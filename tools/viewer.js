/* ==========================================================================
   Doc viewer script — inlined into every generated page by build_docs.mjs.
   Responsibilities: theme toggle, TOC highlighting, Mermaid rendering and the
   zoom / pan / fullscreen controls around each diagram.
   ========================================================================== */
(function () {
  'use strict';

  var MERMAID_URL = 'https://cdn.jsdelivr.net/npm/mermaid@10.9.8/dist/mermaid.esm.min.mjs';
  var MAX_ZOOM = 8;
  var MIN_ZOOM = 0.1;
  var STEP = 1.25;

  function $(sel, root) { return (root || document).querySelector(sel); }
  function $$(sel, root) { return Array.prototype.slice.call((root || document).querySelectorAll(sel)); }
  function clamp(v, lo, hi) { return Math.min(hi, Math.max(lo, v)); }
  function isDark() { return document.documentElement.dataset.theme === 'dark'; }

  /* ------------------------------------------------------------------------
     Theme
     ------------------------------------------------------------------------ */
  var themeListeners = [];
  $('.theme-toggle').addEventListener('click', function () {
    var next = isDark() ? 'light' : 'dark';
    document.documentElement.dataset.theme = next;
    try { localStorage.setItem('doc-theme', next); } catch (e) { /* private mode */ }
    themeListeners.forEach(function (fn) { fn(); });
  });

  /* ------------------------------------------------------------------------
     TOC: mobile toggle + highlight the section being read
     ------------------------------------------------------------------------ */
  var tocBtn = $('.toc-toggle');
  if (tocBtn) {
    tocBtn.addEventListener('click', function () { document.body.classList.toggle('toc-open'); });
  }
  $$('.toc a').forEach(function (a) {
    a.addEventListener('click', function () { document.body.classList.remove('toc-open'); });
  });

  (function initToc() {
    var links = {};
    $$('.toc a[data-id]').forEach(function (a) { links[a.dataset.id] = a; });
    var heads = $$('.content h1[id], .content h2[id]').filter(function (h) { return links[h.id]; });
    if (!heads.length || !('IntersectionObserver' in window)) return;

    var current = null;
    function setActive(id) {
      if (current === id) return;
      current = id;
      $$('.toc a.active').forEach(function (a) { a.classList.remove('active'); });
      $$('.toc li.grp.open').forEach(function (li) { li.classList.remove('open'); });
      var a = links[id];
      a.classList.add('active');
      var grp = a.closest('li.grp');
      if (grp) grp.classList.add('open');
      var nav = $('.toc');
      var top = a.offsetTop, bottom = top + a.offsetHeight;
      if (top < nav.scrollTop + 40 || bottom > nav.scrollTop + nav.clientHeight - 40) {
        nav.scrollTop = Math.max(0, top - nav.clientHeight / 3);
      }
    }
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) { if (e.isIntersecting) setActive(e.target.id); });
    }, { rootMargin: '-8% 0px -85% 0px' });
    heads.forEach(function (h) { io.observe(h); });
    setActive(heads[0].id);
  })();

  /* ------------------------------------------------------------------------
     Diagram: zoom / pan / fullscreen around one rendered <svg>
     ------------------------------------------------------------------------ */
  function Diagram(fig) {
    var self = this;
    var slot = fig.parentElement;
    var view = $('.dg-view', fig);
    var stage = $('.dg-stage', fig);
    var zoomOut = $('.dg-zoom', fig);
    var S = { k: 1, x: 0, y: 0, w: 0, h: 0, mode: 'width' }; // mode: width | fit | free

    function isFull() { return fig.classList.contains('is-full'); }

    function apply() {
      stage.style.transform = 'translate(' + S.x + 'px,' + S.y + 'px) scale(' + S.k + ')';
      zoomOut.textContent = Math.round(S.k * 100) + '%';
    }

    function limitPan() {
      // Never let the diagram be dragged completely out of view.
      var vw = view.clientWidth, vh = view.clientHeight, keep = 48;
      S.x = clamp(S.x, keep - S.w * S.k, vw - keep);
      S.y = clamp(S.y, keep - S.h * S.k, vh - keep);
    }

    function setViewHeight(px) {
      if (isFull()) return;
      var h = Math.round(clamp(px, 200, window.innerHeight * 0.82));
      if (view.style.height !== h + 'px') view.style.height = h + 'px';
    }

    function fitWidth() {
      var vw = view.clientWidth;
      var k = Math.min(isFull() ? 1.6 : 1, (vw - 24) / S.w);
      S.k = clamp(k, MIN_ZOOM, MAX_ZOOM);
      setViewHeight(S.h * S.k + 24);
      var vh = view.clientHeight;
      S.x = (vw - S.w * S.k) / 2;
      S.y = S.h * S.k + 24 <= vh ? (vh - S.h * S.k) / 2 : 12;
    }

    function fitAll() {
      if (!isFull()) setViewHeight(window.innerHeight * 0.7);
      var vw = view.clientWidth, vh = view.clientHeight;
      var k = Math.min((vw - 24) / S.w, (vh - 24) / S.h, isFull() ? 1.6 : 1);
      S.k = clamp(k, MIN_ZOOM, MAX_ZOOM);
      S.x = (vw - S.w * S.k) / 2;
      S.y = (vh - S.h * S.k) / 2;
    }

    function refit() {
      if (!S.w || S.mode === 'free') return;
      if (S.mode === 'width') fitWidth(); else fitAll();
      apply();
    }

    function setMode(mode) { S.mode = mode; refit(); }

    function zoomAt(clientX, clientY, factor) {
      var rect = view.getBoundingClientRect();
      var px = clientX - rect.left, py = clientY - rect.top;
      var nk = clamp(S.k * factor, MIN_ZOOM, MAX_ZOOM);
      var f = nk / S.k;
      S.x = px - (px - S.x) * f;
      S.y = py - (py - S.y) * f;
      S.k = nk;
      S.mode = 'free';
      limitPan();
      apply();
    }

    function zoomCenter(factor) {
      var r = view.getBoundingClientRect();
      zoomAt(r.left + r.width / 2, r.top + r.height / 2, factor);
    }

    function actualSize() {
      var r = view.getBoundingClientRect();
      zoomAt(r.left + r.width / 2, r.top + r.height / 2, 1 / S.k);
    }

    function panBy(dx, dy) {
      S.x += dx; S.y += dy; S.mode = 'free';
      limitPan(); apply();
    }

    function setFull(on) {
      if (on === isFull()) return;
      if (on) slot.style.height = fig.offsetHeight + 'px';   // keep page layout from jumping
      fig.classList.toggle('is-full', on);
      if (!on) slot.style.height = '';
      document.documentElement.classList.toggle('dg-lock', !!$('.diagram.is-full'));
      $('[data-act="full"]', fig).textContent = on ? '✕' : '⤢';
      $('[data-act="full"]', fig).title = on ? 'Thoát toàn màn hình (Esc)' : 'Toàn màn hình (F)';
      S.mode = on ? 'fit' : 'width';
      requestAnimationFrame(function () { refit(); view.focus({ preventScroll: true }); });
    }

    /* --- content --- */
    this.setSvg = function (markup) {
      stage.innerHTML = markup;
      var svg = $('svg', stage);
      if (!svg) return false;
      var vb = svg.viewBox && svg.viewBox.baseVal;
      var w = vb && vb.width, h = vb && vb.height;
      if (!w || !h) {
        var bb = svg.getBBox();
        w = bb.width; h = bb.height;
      }
      S.w = Math.ceil(w); S.h = Math.ceil(h);
      svg.setAttribute('width', S.w);
      svg.setAttribute('height', S.h);
      svg.style.maxWidth = 'none';
      stage.style.width = S.w + 'px';
      stage.style.height = S.h + 'px';
      S.mode = isFull() ? 'fit' : 'width';
      refit();
      return true;
    };
    this.setFull = setFull;
    this.showMessage = function (html) {
      S.w = 0;
      stage.style.cssText = '';
      stage.style.transform = 'none';
      stage.innerHTML = '<div class="dg-msg">' + html + '</div>';
    };

    /* --- buttons --- */
    fig.addEventListener('click', function (e) {
      var b = e.target.closest('button[data-act]');
      if (!b) return;
      switch (b.dataset.act) {
        case 'in':   zoomCenter(STEP); break;
        case 'out':  zoomCenter(1 / STEP); break;
        case 'one':  actualSize(); break;
        case 'width': setMode('width'); break;
        case 'fit':  setMode('fit'); break;
        case 'full': setFull(!isFull()); break;
      }
    });

    /* --- wheel: Ctrl/⌘ + wheel (or pinch) inline; plain wheel in fullscreen --- */
    view.addEventListener('wheel', function (e) {
      if (!(isFull() || e.ctrlKey || e.metaKey) || !S.w) return;
      e.preventDefault();
      var dy = e.deltaMode === 1 ? e.deltaY * 33 : e.deltaY;
      var sens = (e.ctrlKey && Math.abs(dy) < 50) ? 0.01 : 0.0015;   // trackpad pinch vs mouse wheel
      zoomAt(e.clientX, e.clientY, Math.exp(-dy * sens));
    }, { passive: false });

    /* --- drag to pan, two fingers to pinch --- */
    var ptrs = new Map();
    var pinch = null;
    function pinchState() {
      var p = Array.from(ptrs.values());
      return {
        d: Math.hypot(p[0].x - p[1].x, p[0].y - p[1].y) || 1,
        cx: (p[0].x + p[1].x) / 2, cy: (p[0].y + p[1].y) / 2
      };
    }
    view.addEventListener('pointerdown', function (e) {
      if (!S.w || (e.pointerType === 'mouse' && e.button !== 0)) return;
      view.setPointerCapture(e.pointerId);
      ptrs.set(e.pointerId, { x: e.clientX, y: e.clientY });
      view.classList.add('grabbing');
      pinch = ptrs.size === 2 ? pinchState() : null;
    });
    view.addEventListener('pointermove', function (e) {
      var p = ptrs.get(e.pointerId);
      if (!p) return;
      if (ptrs.size === 1) {
        panBy(e.clientX - p.x, e.clientY - p.y);
        p.x = e.clientX; p.y = e.clientY;
      } else if (ptrs.size === 2 && pinch) {
        p.x = e.clientX; p.y = e.clientY;
        var now = pinchState();
        panBy(now.cx - pinch.cx, now.cy - pinch.cy);
        zoomAt(now.cx, now.cy, now.d / pinch.d);
        pinch = now;
      }
    });
    function endPointer(e) {
      ptrs.delete(e.pointerId);
      pinch = null;
      if (!ptrs.size) view.classList.remove('grabbing');
    }
    view.addEventListener('pointerup', endPointer);
    view.addEventListener('pointercancel', endPointer);

    view.addEventListener('dblclick', function (e) {
      if (!S.w) return;
      zoomAt(e.clientX, e.clientY, e.shiftKey ? 1 / 1.8 : 1.8);
    });

    /* --- keyboard (when the viewport has focus) --- */
    view.addEventListener('keydown', function (e) {
      if (e.ctrlKey || e.metaKey || e.altKey) return;
      var handled = true;
      switch (e.key) {
        case '+': case '=': zoomCenter(STEP); break;
        case '-': case '_': zoomCenter(1 / STEP); break;
        case '0': setMode('fit'); break;
        case '1': actualSize(); break;
        case 'w': case 'W': setMode('width'); break;
        case 'f': case 'F': setFull(!isFull()); break;
        case 'ArrowLeft':  panBy(80, 0); break;
        case 'ArrowRight': panBy(-80, 0); break;
        case 'ArrowUp':    panBy(0, 80); break;
        case 'ArrowDown':  panBy(0, -80); break;
        default: handled = false;
      }
      if (handled) e.preventDefault();
    });

    /* --- keep fitted diagrams fitted when the box resizes --- */
    if ('ResizeObserver' in window) {
      var raf = 0;
      new ResizeObserver(function () {
        cancelAnimationFrame(raf);
        raf = requestAnimationFrame(refit);
      }).observe(view);
    }

    fig._dg = self;
  }

  document.addEventListener('keydown', function (e) {
    if (e.key !== 'Escape') return;
    var f = $('.diagram.is-full');
    if (f && f._dg) f._dg.setFull(false);
  });

  /* ------------------------------------------------------------------------
     Build every diagram frame, then render Mermaid into it
     ------------------------------------------------------------------------ */
  function titleFor(fig) {
    var el = fig.parentElement;
    while ((el = el.previousElementSibling)) {
      if (/^H[1-6]$/.test(el.tagName)) {
        var clone = el.cloneNode(true);
        var a = $('.anchor', clone);
        if (a) a.remove();
        return clone.textContent.trim();
      }
    }
    return 'Sơ đồ';
  }

  function escapeHtml(s) {
    return s.replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; });
  }

  var figs = $$('figure.diagram');
  figs.forEach(function (fig) {
    var title = escapeHtml(titleFor(fig));
    fig.insertAdjacentHTML('afterbegin',
      '<div class="dg-bar">' +
        '<span class="dg-title" title="' + title + '">' + title + '</span>' +
        '<span class="dg-hint">Kéo để di chuyển · Ctrl + cuộn để zoom · nhấp đúp để phóng to</span>' +
        '<span class="dg-btns">' +
          '<button type="button" data-act="out" title="Thu nhỏ (−)" aria-label="Thu nhỏ">−</button>' +
          '<span class="dg-zoom" aria-live="polite">100%</span>' +
          '<button type="button" data-act="in" title="Phóng to (+)" aria-label="Phóng to">+</button>' +
          '<button type="button" data-act="one" title="Kích thước thật (1)">1:1</button>' +
          '<button type="button" data-act="width" title="Vừa chiều rộng (W)">Vừa rộng</button>' +
          '<button type="button" data-act="fit" title="Thấy toàn bộ sơ đồ (0)">Vừa khung</button>' +
          '<button type="button" data-act="full" title="Toàn màn hình (F)" aria-label="Toàn màn hình">⤢</button>' +
        '</span>' +
      '</div>' +
      '<div class="dg-view" tabindex="0" role="img" aria-label="' + title + '">' +
        '<div class="dg-stage"><div class="dg-msg">Đang vẽ sơ đồ…</div></div>' +
      '</div>');
    new Diagram(fig);
  });

  if (!figs.length) return;

  var mermaid = null;
  var renderSeq = 0;

  function mermaidConfig() {
    return {
      startOnLoad: false,
      securityLevel: 'loose',
      theme: isDark() ? 'dark' : 'default',
      fontFamily: '"Segoe UI", system-ui, sans-serif',
      flowchart: { useMaxWidth: false },
      sequence: { useMaxWidth: false },
      state: { useMaxWidth: false },
      class: { useMaxWidth: false }
    };
  }

  function showSource(fig, reason) {
    fig._dg.showMessage(
      '<div class="dg-err"><strong>Không vẽ được sơ đồ.</strong> ' + escapeHtml(reason) +
      '<pre>' + escapeHtml(fig.dataset.src) + '</pre></div>');
  }

  async function renderAll() {
    mermaid.initialize(mermaidConfig());
    for (var i = 0; i < figs.length; i++) {
      var fig = figs[i];
      try {
        var out = await mermaid.render('dg-svg-' + (++renderSeq), fig.dataset.src);
        if (!fig._dg.setSvg(out.svg)) showSource(fig, 'Mermaid không trả về SVG.');
      } catch (err) {
        showSource(fig, String((err && err.message) || err));
      }
    }
  }

  import(MERMAID_URL).then(function (mod) {
    mermaid = mod.default;
    themeListeners.push(renderAll);
    return renderAll();
  }).catch(function () {
    figs.forEach(function (fig) {
      showSource(fig, 'Cần internet để tải thư viện Mermaid (' + MERMAID_URL + ').');
    });
  });
})();
