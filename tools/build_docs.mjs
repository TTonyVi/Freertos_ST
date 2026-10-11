#!/usr/bin/env node
/**
 * build_docs.mjs — turns the project's Markdown docs into self-contained HTML
 * pages with zoomable diagrams, a table of contents and a dark mode.
 *
 *   cd tools
 *   npm install
 *   npm run build
 *
 * The .md files stay the source of truth. Never edit the generated .html by hand.
 * Output is deterministic (no timestamps), so re-running without changes to the
 * .md files produces no git diff.
 */

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import MarkdownIt from 'markdown-it';
import hljs from 'highlight.js/lib/core';
import c from 'highlight.js/lib/languages/c';
import bash from 'highlight.js/lib/languages/bash';
import powershell from 'highlight.js/lib/languages/powershell';
import json from 'highlight.js/lib/languages/json';
import diff from 'highlight.js/lib/languages/diff';
import ini from 'highlight.js/lib/languages/ini';
import xml from 'highlight.js/lib/languages/xml';
import makefile from 'highlight.js/lib/languages/makefile';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');

/** Pages to build. `src` and `out` are relative to the repo root. */
const DOCS = [
  { src: 'docs/START_HERE.md',      out: 'docs/START_HERE.html',      label: 'Học FreeRTOS' },
  { src: 'docs/SYSTEM_DIAGRAMS.md', out: 'docs/SYSTEM_DIAGRAMS.html', label: 'Sơ đồ hệ thống' },
  { src: 'PROJECT_SPEC.md',         out: 'PROJECT_SPEC.html',         label: 'Đặc tả' },
  { src: 'docs/LEARNING_NOTES.md',  out: 'docs/LEARNING_NOTES.html',  label: 'Ghi chú học tập' },
];

const BRAND = 'Washing Machine Simulator';

/* ------------------------------------------------------------------------ */
/* Syntax highlighting                                                       */
/* ------------------------------------------------------------------------ */

for (const [name, lang] of Object.entries({ c, bash, powershell, json, diff, ini, xml, makefile })) {
  hljs.registerLanguage(name, lang);
}
const LANG_ALIAS = { h: 'c', cpp: 'c', sh: 'bash', shell: 'bash', ps1: 'powershell', html: 'xml', make: 'makefile' };

/* ------------------------------------------------------------------------ */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------ */

const toPosix = (p) => p.split(path.sep).join('/');
const read = (p) => fs.readFileSync(p, 'utf8').replace(/^﻿/, '');

/** GitHub-style heading slug, keeping Unicode letters (matches existing #anchors). */
function slugify(text) {
  return text
    .trim()
    .toLowerCase()
    .replace(/[^\p{L}\p{N}\p{M}\s_-]/gu, '')
    .replace(/\s/g, '-');
}

/** Visible text of an inline token's children (no markdown markers). */
function inlineText(token) {
  return (token.children || [])
    .filter((t) => t.type === 'text' || t.type === 'code_inline')
    .map((t) => t.content)
    .join('');
}

/** Replace `{{KEY}}` placeholders. Uses a function so `$` in values is never special. */
function fill(template, values) {
  return template.replace(/\{\{([A-Z]+)\}\}/g, (_, key) => (key in values ? values[key] : `{{${key}}}`));
}

/* ------------------------------------------------------------------------ */
/* Markdown → HTML                                                           */
/* ------------------------------------------------------------------------ */

const builtOutputs = new Map(DOCS.map((d) => [path.resolve(root, d.src), path.resolve(root, d.out)]));

function createRenderer(doc) {
  const md = new MarkdownIt({
    html: true,
    linkify: false,
    typographer: false,
    highlight(code, langRaw) {
      const lang = LANG_ALIAS[langRaw] || langRaw;
      let inner;
      if (lang && hljs.getLanguage(lang)) {
        inner = hljs.highlight(code, { language: lang, ignoreIllegals: true }).value;
      } else {
        inner = md.utils.escapeHtml(code);
      }
      const label = md.utils.escapeHtml(langRaw || '');
      return `<pre class="code" data-lang="${label}"><code class="hljs">${inner}</code></pre>\n`;
    },
  });

  // ```mermaid  →  zoomable diagram frame (the script draws into it).
  const defaultFence = md.renderer.rules.fence;
  md.renderer.rules.fence = (tokens, idx, options, env, self) => {
    const token = tokens[idx];
    if (token.info.trim() === 'mermaid') {
      env.diagramCount = (env.diagramCount || 0) + 1;
      return (
        `<div class="dg-slot"><figure class="diagram" data-src="${md.utils.escapeHtml(token.content)}"></figure></div>\n`
      );
    }
    return defaultFence(tokens, idx, options, env, self);
  };

  // Wide tables scroll sideways instead of breaking the layout.
  md.renderer.rules.table_open = () => '<div class="table-wrap"><table>\n';
  md.renderer.rules.table_close = () => '</table></div>\n';

  // Heading ids + permalink anchors, and a heading list for the TOC.
  md.core.ruler.push('heading_ids', (state) => {
    const used = new Map();
    state.env.headings = [];
    state.tokens.forEach((token, i) => {
      if (token.type !== 'heading_open') return;
      const level = Number(token.tag.slice(1));
      const text = inlineText(state.tokens[i + 1]);
      let id = slugify(text) || 'section';
      const n = used.get(id) || 0;
      used.set(id, n + 1);
      if (n > 0) id = `${id}-${n}`;
      token.attrSet('id', id);
      state.env.headings.push({ level, text, id });
    });
  });
  md.renderer.rules.heading_close = (tokens, idx) => {
    const id = tokens[idx - 2].attrGet('id');
    return `<a class="anchor" href="#${id}" aria-label="Liên kết tới mục này">#</a></${tokens[idx].tag}>\n`;
  };

  // Links: .md → .html when the target page is built here; external → new tab.
  const srcDir = path.dirname(path.resolve(root, doc.src));
  const outDir = path.dirname(path.resolve(root, doc.out));
  md.core.ruler.push('rewrite_links', (state) => {
    for (const block of state.tokens) {
      for (const token of block.children || []) {
        if (token.type !== 'link_open') continue;
        const href = token.attrGet('href') || '';
        if (/^[a-z][a-z0-9+.-]*:/i.test(href)) {
          token.attrSet('target', '_blank');
          token.attrSet('rel', 'noopener noreferrer');
          continue;
        }
        const m = href.match(/^([^#?]+\.md)(#.*)?$/i);
        if (!m) continue;
        const target = builtOutputs.get(path.resolve(srcDir, decodeURI(m[1])));
        if (target) {
          token.attrSet('href', toPosix(path.relative(outDir, target)) + (m[2] || ''));
        }
      }
    }
  });

  return md;
}

/* ------------------------------------------------------------------------ */
/* Table of contents                                                         */
/* ------------------------------------------------------------------------ */

function buildToc(headings) {
  const esc = (s) => s.replace(/[&<>"]/g, (ch) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[ch]));
  const link = (h) => `<a href="#${h.id}" data-id="${h.id}" title="${esc(h.text)}">${esc(h.text)}</a>`;
  const items = headings.filter((h) => h.level <= 2);
  const h1Count = items.filter((h) => h.level === 1).length;

  if (h1Count <= 1) {
    // One title, sections are h2: a flat list is easier to scan.
    const flat = items.filter((h) => h.level === 2).map((h) => `      <li>${link(h)}</li>`);
    return `    <ul>\n${flat.join('\n')}\n    </ul>`;
  }

  // Several h1 chapters: collapsible groups, the one being read opens itself.
  const groups = [];
  for (const h of items) {
    if (h.level === 1 || !groups.length) groups.push({ head: h.level === 1 ? h : null, kids: [] });
    if (h.level === 2) groups[groups.length - 1].kids.push(h);
  }
  const html = groups.map((g) => {
    const kids = g.kids.length ? `\n        <ul>\n${g.kids.map((k) => `          <li>${link(k)}</li>`).join('\n')}\n        </ul>` : '';
    return g.head
      ? `      <li class="grp">${link(g.head)}${kids}\n      </li>`
      : `      <li class="grp open">${kids}\n      </li>`;
  });
  return `    <ul>\n${html.join('\n')}\n    </ul>`;
}

/* ------------------------------------------------------------------------ */
/* Build                                                                     */
/* ------------------------------------------------------------------------ */

const template = read(path.join(here, 'template.html'));
const css = read(path.join(here, 'viewer.css'));
const js = read(path.join(here, 'viewer.js'));

let failed = false;
for (const doc of DOCS) {
  const srcPath = path.resolve(root, doc.src);
  const outPath = path.resolve(root, doc.out);
  if (!fs.existsSync(srcPath)) {
    console.error(`✗ ${doc.src}: not found`);
    failed = true;
    continue;
  }

  const md = createRenderer(doc);
  const env = {};
  const body = md.render(read(srcPath), env);
  const title = env.headings.find((h) => h.level === 1)?.text || doc.label;

  const outDir = path.dirname(outPath);
  const nav = DOCS.map((d) => {
    const href = toPosix(path.relative(outDir, path.resolve(root, d.out)));
    const current = d === doc ? ' aria-current="page"' : '';
    return `<a href="${href}"${current}>${d.label}</a>`;
  }).join('');

  const html = fill(template, {
    TITLE: title.replace(/[<>&]/g, ''),
    SRC: doc.src,
    BRAND,
    NAV: nav,
    TOC: buildToc(env.headings),
    CONTENT: body,
    CSS: css,
    JS: js,
  });

  fs.writeFileSync(outPath, html, 'utf8');
  const kb = (Buffer.byteLength(html) / 1024).toFixed(0);
  console.log(`✓ ${doc.out}  (${env.headings.length} headings, ${env.diagramCount || 0} diagrams, ${kb} KB)`);
}

process.exit(failed ? 1 : 0);
