# Q&A Learning Log

Ghi lại toàn bộ câu hỏi và câu trả lời trong quá trình học FreeRTOS.
Cập nhật sau mỗi phiên Q&A trước khi bắt đầu viết code.

---

## Session 17 — 2026-10-10 — Van cấp nước dừng theo cảm biến, không theo giờ

**Yêu cầu:** "van xả nước vào lồng giặt lúc bấm giặt … xả đến lúc cảm biến báo đầy thì dừng
chứ không xả theo thời gian nữa".

**Hiểu nhầm lần đầu:** tôi tưởng là van xả (`DRAIN`) và đã sửa `DRAIN` kết thúc bằng bit
`EVT_WATER_EMPTY`. Sai đối tượng — đã hoàn tác toàn bộ. Người dùng nói về **van cấp nước**.

**Thực tế code:** `FILL_WATER` đã chờ `EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR`; van cấp
đóng khi cảm biến báo đầy. 10 s chỉ là timeout an toàn (hết giờ → `ERROR`). Chính tài liệu
ghi "Bơm nước: 10 giây" khiến nó giống chạy theo giờ → đã sửa chữ trong START_HERE, SPEC.

**Lỗi thật tìm thấy:** `EVT_WATER_FULL` không bao giờ bị xoá → bit cũ làm `FILL_WATER`
ở chu trình sau kết thúc oan. Nay SensorTask xoá bit khi cảm biến báo cạn.

---

## Session 16 — 2026-10-10 — Một biến, một chủ: sửa lỗi `s_statusCache`

**Chủ đề:** Khi viết Tầng 3 phát hiện UITask hiển thị trạng thái `IDLE` cũ và giữ
một con trỏ menu riêng lệch với WashingManager. Sửa bằng cách dồn quyền sở hữu về
WashingManager, rồi đồng bộ toàn bộ tài liệu.

---

### Q1: Sửa theo hướng nào?

UITask có `s_statusCache` không bao giờ được ghi (luôn `IDLE`), và con trỏ menu
tồn tại ở hai nơi. Ba phương án; chọn **hướng 2**: WashingManager giữ con trỏ và là
bên duy nhất dựng `DisplayMsg_t`; UITask thành bộ dịch không trạng thái
(`BTN_EVENT_NAVIGATE → CMD_NAVIGATE`, `CONFIRM → CMD_CONFIRM`). Hệ quả có chủ ý: nút
bị bỏ qua lúc đang chạy chu trình.

### Q2: `sizeof` của message là bao nhiêu?

Tôi từng "sửa" comment thành 12/36 byte theo giả định enum = 4 byte — **sai**.
Đo bằng probe `char (*p)[sizeof(T)] = 1;` cho ra 1/1/27 vì arm-none-eabi-gcc dùng
`-fshort-enums`. Bài học: đo, đừng đoán.

### Q3: Vì sao file có byte NUL?

Một lớp xử lý chuỗi nuốt một nửa dấu `\` trong tham số Bash/node nên `\0` thành NUL.
Sửa bằng mã ký tự; về sau dùng Edit/Write cho văn bản có backslash.

### Q4: Kiểm chứng thế nào khi chưa nạp được board?

Biên dịch sạch 0 cảnh báo; chạy mã thật của manager + SM trên host (37/37) và thử
"đột biến" — đưa lại từng lỗi cũ, bộ test đều bắt được. **Chưa** kiểm chứng trên phần cứng.

### Q5: Có nên bỏ UITask?

Sau bản sửa nó chỉ đổi tên enum, và phản đối "manager phải chờ nhiều nguồn" gần như
hết (nút không dùng lúc đang chạy). Về kỹ thuật bỏ được; chưa quyết định.

### Ghi chú tài liệu

SYSTEM_DIAGRAMS §1 tách thành 1a/1b/1c (mỗi sơ đồ ≤ ~800 px). Sơ đồ tuần tự §5 gãy vì
dấu `;` trong `Note` (Mermaid coi là ngăn lệnh) — đã đổi thành dấu phẩy.

---

## Session 15 — 2026-10-09 — Thiết kế lại HTML: sơ đồ zoom được

**Chủ đề:** Sơ đồ kiến trúc trong `SYSTEM_DIAGRAMS.html` bị co cho vừa chiều rộng
trang nên chữ không đọc được và không phóng to được. Dựng lại toàn bộ bộ HTML.

---

### Q1: Vì sao không zoom được, và nên sửa ở đâu?

**Chẩn đoán:** Mermaid mặc định đặt `useMaxWidth: true` — tự co SVG cho vừa khung
chứa nó. Sơ đồ kiến trúc tổng thể thực chất rộng **2757 × 2157 px**; nhét vào cột
960 px thì còn ~35 %, chữ ~5 px. Trình duyệt không zoom riêng từng sơ đồ được,
và Ctrl + cuộn zoom cả trang (làm vỡ bố cục chứ không giúp đọc sơ đồ).

**Hai phát hiện kèm theo khi khảo sát 3 file HTML:**

| File | Vấn đề |
| --- | --- |
| `PROJECT_SPEC.html` | Nạp `mermaid.min.js` và `katex.css` từ `file:///c:\Users\Dell\.vscode\extensions\...` — chỉ mở được trên đúng máy này, và gãy khi extension đổi phiên bản |
| `SYSTEM_DIAGRAMS.html` | Viết tay, ghi chú "regenerate when diagrams change" nhưng không có công cụ nào để regenerate → sẽ lệch khỏi `.md` |
| `START_HERE.html` | Xuất từ Markdown Preview Enhanced; cũ hơn `.md` 16 phút ở phiên trước |

Cả ba do ba cách khác nhau tạo ra. Đây lại là vấn đề "một sự thật nằm ở hai chỗ".

**Quyết định:** một script sinh cả ba từ `.md`, chạy lại là đồng bộ.

---

### Thiết kế

**Công cụ** (`tools/`): `build_docs.mjs` + `template.html` + `viewer.css` +
`viewer.js`. Phụ thuộc: `markdown-it` (render), `highlight.js` (tô màu C/bash).
Mermaid vẫn nạp từ CDN, **ghim phiên bản 10.9.8** (trước đó `@10` trôi theo bản mới).
Không dùng timestamp trong output → build lại khi `.md` không đổi thì **không sinh
git diff** (đã kiểm chứng bằng sha1).

**Bộ điều khiển sơ đồ:**

| Thao tác | Cách làm |
| --- | --- |
| Di chuyển | Kéo chuột |
| Zoom tại con trỏ | **Ctrl + cuộn** (hoặc pinch trackpad); trong toàn màn hình thì cuộn thường |
| Phóng nhanh | Nhấp đúp (Shift + nhấp đúp để thu) |
| Nút | `−`  `+`  `1:1`  `Vừa rộng`  `Vừa khung`  `⤢` |
| Phím (khi khung có focus) | `+ −` zoom · `0` vừa khung · `1` 100 % · `W` vừa rộng · `F` toàn màn hình · mũi tên di chuyển · `Esc` thoát |

**Quyết định thiết kế đáng ghi lại:**

1. **Inline chỉ zoom bằng Ctrl + cuộn, không phải cuộn thường.** Nếu cuộn thường
   cũng zoom thì sơ đồ "nuốt" thao tác cuộn trang — người đọc rê chuột qua sơ đồ
   là bị kẹt. Chỉ trong toàn màn hình (không còn trang để cuộn) mới cho cuộn thường
   = zoom. Đã kiểm chứng: cuộn thường trên sơ đồ làm `scrollY` tăng, zoom đứng yên.
2. **Mặc định "vừa chiều rộng", không phải "vừa khung".** Sơ đồ cao như sequence
   diagram mà ép vừa khung sẽ co rất nhỏ; vừa chiều rộng đọc được ngay và kéo dọc
   là xem tiếp. "Vừa khung" vẫn có nút riêng cho ai muốn nhìn toàn cảnh.
3. **Giữ kích thước gốc của SVG** (`useMaxWidth: false`) và scale bằng CSS
   `transform` → chữ vector, zoom 800 % vẫn sắc nét, không bị vỡ như ảnh bitmap.
4. **Cảm ứng:** trong trang, cảm ứng vẫn cuộn trang bình thường
   (`touch-action: pan-x pan-y`); pinch/kéo chỉ bật trong toàn màn hình. Nếu bật
   luôn thì ngón tay chạm vào sơ đồ là không cuộn trang được.
5. **Mất mạng không vỡ trang:** nếu không tải được Mermaid, mỗi khung hiện thông
   báo và **mã nguồn sơ đồ dạng text** thay vì để trống.
6. Đổi giao diện sáng/tối **vẽ lại** sơ đồ với theme tương ứng (Mermaid cố định
   màu lúc vẽ, đổi CSS thôi là không đủ).

**Thêm cho cả 3 trang:** mục lục bên trái (tự mở chương đang đọc), tô màu cú pháp C,
bảng rộng cuộn ngang, neo `#` ở mỗi tiêu đề, link `.md` → `.html` khi đích cũng
được build, thanh chuyển giữa các tài liệu.

---

### Kiểm chứng: điều khiển Chrome thật, không chỉ "build xong"

Dùng `puppeteer-core` + Chrome (cài ở thư mục tạm, **không** thêm vào project).
**25/25 kiểm tra đạt**: 9/9 sơ đồ vẽ ra SVG · giữ kích thước gốc (2757 × 2157) ·
nút `+` / `1:1` · Ctrl + cuộn zoom · cuộn thường **không** zoom · kéo chuột dịch
đúng 120 × 60 px · nhấp đúp · toàn màn hình phủ đủ 1400 × 900 · cuộn zoom trong
toàn màn hình · Esc thoát · đổi sang tối vẽ lại đủ 9 sơ đồ · mục lục tô đúng chương
· không lỗi JS · `PROJECT_SPEC.html` không còn đường dẫn extension.
Tĩnh: **0** liên kết `#` gãy (215 liên kết), **0** placeholder sót.

**Bài học từ chính quá trình test:** lần chạy đầu có 2 FAIL, và **cả hai đều do
bài test sai, không phải trang sai** — nhưng không thể biết điều đó nếu không tách
ra điều tra:

- *Nhấp đúp:* Puppeteer luôn gửi `clickCount: 1` nên trình duyệt không sinh
  `dblclick`. Gửi sự kiện giả `dblclick` → zoom đúng ×1.8 (0.367 → 0.661); gửi
  chuỗi `clickCount` 1 rồi 2 như chuột thật → cũng đúng. Kết luận: trang đúng.
- *Mục lục:* `scroll-behavior: smooth` khiến test đọc trạng thái giữa chừng lúc cuộn.

Quy tắc rút ra: **test fail chưa phải bug — phải cô lập xem lỗi ở đối tượng được
test hay ở công cụ test, trước khi sửa bất cứ thứ gì.**

---

### Còn tồn tại: bản thân sơ đồ kiến trúc quá lớn

Zoom giải quyết việc đọc, nhưng **sơ đồ tổng thể rộng 2757 × 2157 px là dấu hiệu
sơ đồ làm quá nhiều việc**: 4 task + 3 queue + 1 event group + 4 timer + 4 GPIO +
2 ISR trong một hình, với rất nhiều đường chéo nhau. Ngay cả toàn màn hình vẫn chỉ
vừa ở ~38 %. Hướng cải thiện (chưa làm, chờ quyết định): tách thành 3 sơ đồ —
luồng nút bấm / luồng cảm biến + event group / luồng timer — mỗi cái dưới ~1200 px.

---

### Q2: Minh hoạ mục "7. RTOS Architecture" của PROJECT_SPEC bằng sơ đồ dễ nhìn

**Hiện trạng:** mục 7 là chuỗi chữ xuống dòng với mũi tên `↓ ↑` ở giữa — 12 đoạn
văn rời, người đọc phải tự ghép lại. Nó còn **thiếu Software Timer** dù mục 9 của
chính đặc tả khai timer là nguồn của `EVT_TIMEOUT`.

**Đã làm:** thay bằng một sơ đồ Mermaid trong `PROJECT_SPEC.md` (nguồn), kèm bảng
chú giải ký hiệu và 3 dòng "Cách đọc". HTML tự cập nhật qua `npm run build`.

**Quyết định thiết kế — vẽ qua 4 vòng lặp, mỗi vòng sửa đúng một lỗi nhìn thấy:**

| Vòng | Bố cục | Lỗi nhìn thấy | Sửa |
| --- | --- | --- | --- |
| 1 | Ngang (`LR`) | Rộng **1668 px** → bị co còn 61 %, chữ nhỏ; nền vàng mặc định lệch tông | Đổi sang dọc (`TB`) → 807 px, đọc được ở 100 %; tô nền khung bằng màu xám **có độ trong suốt** để dùng được cả giao diện sáng lẫn tối |
| 2 | Dọc | Mũi tên đi **xuyên qua tiêu đề** "③ Luồng hiển thị"; cao 990 px bị cắt | Nối `WM --> DSP` (vào **khung**, không vào node bên trong) → mũi tên dừng ở viền; đồng thời Mermaid tôn trọng `direction LR` bên trong khung ③ nên DQ → OLED nằm ngang, bớt một tầng |
| 3 | + nén khoảng cách | Tham số `padding: 10` làm node **đè lên tiêu đề** ① ② | Bỏ `padding`; dùng `subGraphTitleMargin` chừa riêng chỗ cho tiêu đề |
| 4 | Hoàn chỉnh | Cao 840 px, màn 900 px chỉ hiện ~740 → cắt đáy khung ③ | Sửa **bộ xem** (xem dưới) |

**Bài học Mermaid đáng nhớ:** khi một khung con (`subgraph`) có node bị nối ra
ngoài, `direction` bên trong bị **bỏ qua** và kế thừa hướng toàn cục. Nối vào
**chính khung** (`WM --> DSP`) thay vì node bên trong thì hướng riêng được giữ.
Mẹo này vừa tránh va chạm mũi tên–tiêu đề, vừa làm sơ đồ gọn hơn.

**Cải tiến bộ xem rút ra từ sơ đồ này:** sơ đồ chỉ cao hơn khung một chút thì
đọc trọn vẹn dễ hơn là phải cuộn. `viewer.js` giờ tự thu nhỏ **tối đa 30 %** để
sơ đồ vừa khung; sơ đồ cao hơn nhiều (sequence diagram) vẫn giữ "vừa chiều rộng"
rồi kéo dọc. Trên màn 900 px, sơ đồ này hiện trọn ở 85 %.

**Kiểm chứng:** chụp ảnh Chrome thật ở sáng + tối sau mỗi vòng; chạy lại bộ test
**25/25 đạt**. Một kiểm tra lúc đầu FAIL (`PROJECT_SPEC.html` kỳ vọng 1 sơ đồ,
giờ có 2) — đây là FAIL **đúng**, vì nội dung đã thay đổi chủ ý; cập nhật con số
kỳ vọng trong test chứ không "sửa cho test qua".

**Khớp với code thật:** các bit event (`DOOR_CLOSED · WATER_FULL · TIMEOUT ·
FINISHED · ERROR`), priority (UI 2, Sensor 2, Manager 3, OLED 1), chu kỳ Sensor
100 ms đối chiếu với `app_events.h` / `app_config.h`. Sơ đồ vẫn theo **đặc tả**
(có UI Task) — nếu sau này gộp UI Task vào Manager thì phải sửa cả mục 8, 9 chứ
không riêng sơ đồ.

---

## Session 14 — 2026-08-31 — Viết Tầng 3 & phát hiện bug `s_statusCache`

**Chủ đề:** Viết Tầng 3 (Queue) của `START_HERE.md`. Trong lúc đọc `ui_task.c`
để trích code, phát hiện một lỗi thiết kế kênh liên lạc có triệu chứng nhìn thấy
được trên màn hình.

---

### Q1: "ổn rồi viết tiếp tầng 2" — nhưng Tầng 2 đã viết xong

**Nguyên nhân hiểu nhầm:** bạn đang đọc `docs/START_HERE.html` — bản xuất bằng
Markdown Preview Enhanced lúc **22:03**, trong khi Tầng 2 được ghi vào `.md` lúc
**22:19**. HTML cũ hơn 16 phút nên vẫn hiện "Tầng 2–6 — 🚧 Chưa viết".

**Rút ra cho quy trình:** `START_HERE.html` là **file dẫn xuất**, không tự cập
nhật. Mỗi lần sửa `.md` phải xuất lại (chuột phải → Markdown Preview Enhanced →
Export → HTML), nếu không sẽ đọc nhầm bản cũ. Đây đúng là vấn đề "một sự thật
nằm ở hai chỗ" mà §0.1 của chính tài liệu đó cảnh báo — lần này áp vào chính bộ
tài liệu.

---

### Q2: Bug phát hiện khi viết Tầng 3 — `s_statusCache` không bao giờ được ghi

**Triệu chứng:** đang giặt, bấm PA0 để di chuyển con trỏ menu → dòng trạng thái
OLED đổi từ `WASHING` thành `IDLE`, dù máy vẫn đang giặt (đèn LED vẫn đúng).

**Truy nguyên:**

```c
/* ui_task.c:44-46 */
/* Cached status text from last WashingManager display update.
 * UITask piggybacks this when it sends its own cursor-update messages. */
static char s_statusCache[24] = "IDLE";
```

Grep cả file: `s_statusCache` xuất hiện **đúng hai lần** — dòng khai báo và dòng
đọc (`ui_task.c:65`). **Không có dòng nào ghi vào nó.** Comment mô tả một cơ chế
cache không tồn tại, và cũng **không thể tồn tại**: `xDisplayQueue` là kênh một
chiều `WashingManager → OLEDTask`, UITask không đọc nó nên không có đường nào
biết được statusText hiện tại.

**Chẩn đoán — đây là lỗi thiết kế kênh, không phải lỗi đánh máy:**

`DisplayMsg_t` chở **cả cụm** (menuIndex + statusText + state + selectedMode),
nhưng có **hai producer** cùng gửi vào `xDisplayQueue`, và mỗi bên chỉ biết một
phần:

| Producer | Biết | Phải bịa |
| --- | --- | --- |
| WashingManager | state, statusText, selectedMode | `menuIndex` (giữ bản sao riêng `s_menuIndex`) |
| UITask | menuIndex | `statusText` (bịa bằng hằng `"IDLE"`) |

Vi phạm đúng tiêu chí ownership đã dạy ở Tầng 2: **nội dung màn hình đang có hai
chủ.**

**Chứng cứ phụ củng cố chẩn đoán:** `ui_task.c:63` khai `DisplayMsg_t disp;` rồi
chỉ gán `menuIndex` và `statusText`, **để nguyên `state` và `selectedMode` chưa
khởi tạo** — queue vẫn trung thành copy đủ 36 byte kể cả hai trường rác. Chưa
gây triệu chứng vì `OLED_Render()` không dùng tới chúng, nhưng nó cho thấy chính
người viết cũng cảm nhận được UITask không đủ thông tin để dựng một `DisplayMsg_t`
hoàn chỉnh.

**Ba hướng sửa đã nêu trong tài liệu (§3.6), chưa áp dụng — chờ quyết định:**

| Hướng | Cách làm | Đánh đổi |
| --- | --- | --- |
| 1. OLEDTask nhớ trạng thái | Thêm `updateMask` vào `DisplayMsg_t`; OLEDTask giữ khung hình gần nhất, chỉ ghi đè phần được đánh dấu | Sửa ít nhất; OLEDTask không còn là pure renderer |
| 2. UITask không gửi thẳng OLED | UITask báo "cursor moved" cho WashingManager; bên duy nhất biết đủ mọi thứ dựng `DisplayMsg_t` | Đúng nguyên tắc một chủ nhất; thêm một chặng |
| 3. Tách hai queue | Một cho trạng thái, một cho con trỏ; OLEDTask ghép | Ranh giới sạch nhất; tốn RAM và một chỗ chờ |

Nghiêng về hướng 2 vì hợp nguyên tắc "WashingManager là chủ duy nhất của trạng
thái", nhưng cả ba đều hợp lệ.

**Đã sửa ngay trong tài liệu:** Thí nghiệm A của Tầng 1 bảo người học bấm PA0 lúc
đang giặt, nhưng không nhắc gì tới việc dòng trạng thái nhảy về `IDLE`. Đã thêm
ghi chú cảnh báo đây là lỗi thật, kèm câu hỏi gợi mở dẫn sang Tầng 3 ("đèn vẫn
đúng nhưng chữ thì sai — hai thứ đó đi bằng hai đường khác nhau, đường nào?").

---

### Nội dung Tầng 3

**Cách ngây thơ được chọn để bác bỏ:** biến toàn cục + `volatile`. Bốn lỗ hổng
xếp theo độ khó phát hiện tăng dần: mất sự kiện → không block được → race trên
đọc-sửa-ghi (`LDR`/`ADD`/`STR`) → **struct bị đọc nửa vời**. Nhấn mạnh
`volatile` **không** cho tính nguyên tử — hiểu nhầm phổ biến nhất về từ khoá này.

**Bốn tính chất của Queue**, mỗi tính chất khớp một lỗ hổng: có chiều sâu · chặn
được hai chiều · thao tác nguyên tử · **sao chép theo giá trị**.

**Thí nghiệm A (quyết định):** thêm `disp.menuIndex = 99u;` **ngay sau**
`xQueueSend()` trong `ui_task.c:68`. Con trỏ vẫn nhảy đúng → chứng minh queue đã
chụp ảnh 36 byte trong lúc hàm còn chạy, không giữ con trỏ. Lập luận sắc hơn đi
kèm: `disp` nằm trên stack UITask và khung stack đó bị thu hồi rất lâu trước khi
OLEDTask (prio 1) được chạy — nếu queue giữ con trỏ thì đây là dangling pointer
100 % lần nào cũng dính.

**Thí nghiệm B:** bấm PD2 giữa lúc đang giặt → lệnh nằm đệm trong `xCommandQueue`
→ tới `FINISH` bị `xQueueReset()` dọn → máy đứng yên. Bỏ dòng reset đó đi chính
là **Bug #10**.

**Điểm thiết kế được nhấn mạnh:** vì sao **mọi** chỗ gửi đều dùng timeout `0`.
Nếu `Manager_PostDisplay()` dùng `portMAX_DELAY`, WashingManager (prio 3) sẽ bị
OLEDTask (prio 1) chặn — đảo ưu tiên thủ công. Quy tắc mang đi được: *bên gửi ở
priority cao không bao giờ được block chờ bên nhận ở priority thấp.*

Câu 2 mục Tự kiểm tra mở rộng thêm một bước suy luận đáng tập thành phản xạ:
chặn UITask không chỉ làm chậm UITask — UITask là bên tiêu thụ duy nhất của
`xButtonQueue`, nó dừng thì queue nút bấm đầy dần. **Chặn một task luôn kéo theo
hậu quả lan ngược lên thượng nguồn của nó.**

**Giữ kỷ luật một tầng một khái niệm:** `xQueueSendFromISR` và
`portYIELD_FROM_ISR` được trích và giải thích ở mức "chuyện gì xảy ra theo trình
tự", nhưng PendSV/BASEPRI/NVIC để nguyên cho Tầng 6.

---

### Sửa kèm trong code

`freertos_objects.c` — comment ghi `sizeof(CommandMsg_t) = 4 + 4 = 8 bytes`, sai
từ khi trường `menuIndex` được thêm vào struct. Thực tế là `4 + 4 + 1 + 3 padding
= 12`. Đã sửa cả dòng đó lẫn bảng "Approximate RAM cost" (`~116` → `~136`,
tổng `~544` → `~564`). Build lại: sạch.

Lần thứ ba trong project phát hiện **comment mô tả sai code thật** (trước đó:
`app_events.h` ở Session 09, `bsp_button.h` ở Session 13). Củng cố nguyên tắc đã
ghi ở "Bài học phương pháp": comment không được compiler kiểm tra, chỉ phát hiện
sai khi chủ động đối chiếu.

---

## Session 13 — 2026-08-29 — Dọn tàn dư short/long press trong bsp_button.c

**Chủ đề:** Người dùng phát hiện `BSP_Button_IRQHandler()` vẫn còn bộ máy đo
thời lượng nhấn, dù thiết kế short/long press đã bỏ từ lâu. Truy vết, thiết kế
lại bộ lọc nhiễu, áp dụng cho cả hai nút.

---

### Q1: "code chỗ này chưa sửa thì phải làm gì còn long short press gì nữa"

**Quan sát của bạn:** đúng. Kiểm tra lại tìm ra ba tàn dư:

| # | Tàn dư | Vị trí |
| --- | --- | --- |
| 1 | Macro chết `BUTTON_LONG_PRESS_MS` — grep toàn bộ `App/` + `Core/Src/`: không `.c` nào dùng | `app_config.h:35` |
| 2 | Bộ máy đo thời lượng: `s_pressStartMs`, `s_pressed`, biến `duration` | `bsp_button.c:28-29, 44-52` |
| 3 | Comment tự mâu thuẫn: "check duration for debounce, then fire short press" rồi ngay dưới phải đính chính "Long-press is no longer handled" | `bsp_button.c:47, 53-54` |

**Hệ quả thật, không chỉ là thẩm mỹ:** cấu trúc `if (rising) ghi giờ / else
(falling) bắn sự kiện` khiến sự kiện **chỉ bắn khi THẢ TAY**. Đó là ràng buộc
bắt buộc của thiết kế cũ (phải đợi buông mới đo được thời lượng), nhưng vô nghĩa
sau khi long-press bị bỏ. Giữ nút PA0 3 giây thì con trỏ menu đứng im tới lúc
buông. Nút PD2 ngay bên dưới thì bắn ngay lúc bấm — **hai nút cùng project, hai
triết lý khác nhau, và khác biệt đó không do yêu cầu nào cả.**

Đây cũng chính là điều kiện kích hoạt **Bug #8** ngày trước.

---

### Q2: Phát hiện thêm — PD2 hoàn toàn không có bộ lọc nhiễu

`BSP_StartButton_IRQHandler()` bắn `BTN_EVENT_CONFIRM` trên **mọi** cạnh xuống.
Một lần bấm dội 3-4 cạnh = 3-4 sự kiện CONFIRM.

Triệu chứng đang bị che đi chứ không phải không có: `Manager_OnEnterIdle/Finish/
Error` đều gọi `xQueueReset(xCommandQueue)` nên lệnh thừa bị dọn ở hạ nguồn.
**Hạ nguồn dọn rác cho thượng nguồn** — thượng nguồn vẫn đang sinh sự kiện giả.

**Bạn quyết:** cải tiến cả PD2.

---

### Thiết kế đã chọn: lọc theo khoảng lặng (*quiet window*)

```c
uint32_t now   = HAL_GetTick();
uint8_t  quiet = ((now - s_lastEdgeMs) >= BUTTON_DEBOUNCE_MS) ? 1u : 0u;

s_lastEdgeMs = now;        /* MỌI cạnh đều nạp lại — kể cả cạnh bị từ chối */

if (pressed && quiet && (s_callback != NULL)) { s_callback(...); }
```

Điểm mấu chốt là `s_lastEdgeMs = now;` nằm **ngoài** mọi điều kiện.

**Phương án ngây thơ đã loại:** "bỏ qua cạnh cách *sự kiện đã chấp nhận* gần nhất
dưới 50 ms". Hỏng khi giữ nút lâu: chùm dội lúc nhả cách sự kiện trước 2 giây →
lọt thành sự kiện giả. Phải so với **cạnh gần nhất**, không phải **sự kiện gần
nhất**.

**Phương án thứ hai đã loại:** đọc thêm mức chân cho PD2 để xác nhận nút đang bị
đè. Lúc dội khi bấm, ISR có thể đọc trúng mức nhả và **từ chối cả lần bấm thật**
— đổi một lỗi hiếm lấy một lỗi thường xuyên. Không đáng.

**Giới hạn đã biết, ghi thẳng vào comment thay vì lờ đi:** `.ioc` cấu hình EXTI2
chỉ cạnh xuống, nên ISR của PD2 không nhìn thấy cạnh nhả. Dội trong một lần bấm
bị lọc sạch, nhưng cạnh đầu của chùm dội lúc *nhả* vẫn có thể lọt thành một
CONFIRM thừa. Muốn triệt để phải bật cả hai cạnh cho PD2 trong CubeMX — đã thống
nhất **không đụng `.ioc`** để tránh rủi ro CubeMX reset GPIOD (tiền lệ ghi trong
`GPIO_CONFIG.md`).

---

### Files đã sửa

| File | Sửa gì |
| --- | --- |
| `App/BSP/Src/bsp_button.c` | Viết lại cả hai ISR theo quiet-window; xóa `s_pressStartMs`/`s_pressed`; mỗi nút một biến timestamp riêng (dùng chung sẽ khiến bấm nút này khoá nút kia 50 ms) |
| `App/BSP/Inc/bsp_button.h` | Sửa "on release" → "on press"; **khôi phục dòng tiêu đề "Button 2 — Start / Confirm (PD2)"** vốn bị mất, khiến khối comment đọc như thể vẫn đang mô tả PA0 |
| `App/Common/Inc/app_config.h` | Xóa `BUTTON_LONG_PRESS_MS`; viết lại comment của `BUTTON_DEBOUNCE_MS` cho khớp cơ chế mới |
| `App/Common/Inc/app_types.h` | Comment "the BSP measures duration" — sai từ khi bỏ long press |
| `docs/LEARNING_NOTES.md` | Viết lại mục "Design: Debounce"; đánh dấu mục "macro chết" là đã dọn |

**Build:** `make all` thành công, `bsp_button.c` **không một warning**.
`text 37880 / data 44 / bss 85812`.

---

### Bài học phương pháp

**Macro chết chỉ là phần dễ thấy của một refactor bỏ dở.** Phần nguy hiểm hơn là
*cấu trúc điều khiển* mà tính năng cũ để lại — ở đây là việc sự kiện bắn ở cạnh
nhả tay. Macro không dùng thì vô hại; cấu trúc thừa thì vẫn đang **quy định hành
vi thật của hệ thống**, và không ai chất vấn nó vì "code đang chạy được".

Khi bỏ một tính năng, phải truy ngược: **tính năng đó đã áp đặt những ràng buộc
nào lên phần còn lại?** Rồi gỡ luôn các ràng buộc đó.

---

### Q3: Dọn nốt warning phát sinh lúc build — ✅ đã xong

Build cảnh báo `implicit declaration of function 'BSP_Sensor_Poll'`
(`sensor_task.c:21`). Nguyên nhân: `bsp_sensor.h` **thiếu prototype** cho cả
`BSP_Sensor_Init()` lẫn `BSP_Sensor_Poll()` — chỉ khai báo hai hàm `Is*()`.

**Vì sao nguy hiểm dù code vẫn chạy:** thiếu prototype thì trình biên dịch tự
giả định `int f()`. Ở đây đúng được là **nhờ may** — hàm `void f(void)` trên ARM
EABI tình cờ khớp cách truyền tham số/giá trị trả về. Nếu hàm có tham số, hoặc
trả về kiểu khác `int`, chương trình sẽ hỏng theo kiểu rất khó truy. Từ C23 đây
là **lỗi cứng**, không còn là cảnh báo.

**Đã sửa, ba chỗ:**

| Chỗ | Sửa gì |
| --- | --- |
| `bsp_sensor.h` | Thêm prototype `BSP_Sensor_Init()` và `BSP_Sensor_Poll()` kèm doc |
| `main.c` | Thêm `#include "bsp_sensor.h"` (trong `USER CODE BEGIN Includes`) — header có prototype nhưng `main.c` chưa hề include, nên `BSP_Sensor_Init()` ở dòng 110 vẫn implicit |
| `oled_task.c` | Xóa biến `char line[22]` không dùng tới (`-Wunused-variable`) |

Bài học nhỏ: thêm prototype vào header **chưa đủ** — phải kiểm tra mọi nơi gọi
đã include header đó chưa. Warning thứ nhất biến mất ở `sensor_task.c` nhưng
vẫn còn nguyên ở `main.c`, chỉ lộ ra khi **build sạch** (`make clean`) chứ
build tăng dần không biên dịch lại `main.c`.

**Kết quả:** `make clean && make all` → **0 warning, 0 error** trên toàn project.
`text 37880 / data 44 / bss 85812`.

---

## Session 07 — 2026-07-10 — Test thủ công WashingManager & Bug #8/#9

**Chủ đề:** Soạn kịch bản test tay cho state machine, phát hiện và fix race
condition PA0 (button vs door sensor), giải thích LED/OLED expected behavior.

---

### Q1: Kịch bản test WashingManager nên chạy theo cách nào — unit test PC hay test tay trên board?

**Trả lời của bạn:** Test thủ công trên board thật.

**Kết quả:** Soạn bảng test case bám theo `kTransitions[]` trong `washing_sm.c`,
map Logic Analyzer CH0-CH3 vào PD12-PD15, dùng debugger Live Expressions cho
`s_waterFull`. Phát hiện trước khi test: `bsp_sensor.c` đọc door qua PA0 —
trùng chân với `bsp_button.c` — dự đoán sẽ gây lỗi ngay khi long-press.

---

### Q2: Test thật trên board — bấm Wash, LED cam+xanh dương sáng, SM vào FILL_WATER rồi lập tức ERROR khi thả tay

**Đánh giá:** Đúng như dự đoán ở Q1 — xác nhận bug PA0 dùng chung cho
button (cần thả tay để xác nhận long-press) và door sensor (thả tay = đọc
thành "cửa mở"). Ghi lại thành **Bug #8** trong `HARDWARE_BUGS.md`.

**Quyết định fix:** Phương án A — biến `s_doorClosed` giả lập qua debugger
(giống `s_waterFull`), bỏ hẳn việc đọc GPIO PA0 trong `bsp_sensor.c`. Build
thành công, không lỗi. Xem TD-11 trong `PROJECT_PROGRESS.md`.

---

### Q3: LED hiển thị đúng như thế nào khi chọn Wash?

**Trả lời:** Bảng trạng thái LD3/LD4/LD5/LD6 theo từng state, dựa trên các
entry-action callback trong `washing_manager.c`. LD6 (status) sáng liên tục
từ `FILL_WATER` đến hết `SPIN`, chỉ tắt ở `FINISH`/`ERROR`/`IDLE` — không
phải đèn theo pha mà là đèn "máy đang hoạt động".

---

### Q4: OLED hiển thị đúng như thế nào khi chọn Wash?

**Trả lời:** 4 dòng cố định (tiêu đề, Wash, Spin, statusText), cursor giữ
nguyên vị trí đã chọn suốt chu trình vì `s_menuIndex` chỉ đổi khi có lệnh
mới. Phát hiện thêm **Bug #9** (chưa fix): short-press giữa chu trình làm
OLED tạm hiện sai `"IDLE"` do `s_statusCache` trong `ui_task.c` không bao
giờ được cập nhật sau lần khởi tạo.

---

### Q5: Sau khi vào FILL_WATER, vài giây sau ERROR — có phải bug không?

**Trả lời:** Không — đây là hành vi đúng thiết kế. `WASH_FILL_DURATION_MS`
= 10000ms; vì `s_waterFull` mặc định = 0 và không ai set qua debugger,
`xFillTimeoutTimer` tự bắn `EVT_TIMEOUT` sau đúng 10s → SM chuyển
`FILL_WATER → ERROR` theo transition table. Xác nhận bug PA0 (Bug #8) đã
được fix thành công — đường lỗi còn lại là do chưa mô phỏng nước đầy, không
phải do cửa.

---

## Session 08 — 2026-07-10 — Bug #10: Không thoát được FINISH/ERROR

**Chủ đề:** Test tiếp sau khi fix Bug #8 — chạy hết chu trình Wash bằng cách
set `s_waterFull = 1u`, phát hiện máy đứng yên ở `FINISHED` không phản hồi
nút bấm nữa.

---

### Q1: Set `s_waterFull = 1u`, chạy đến FINISHED, nhưng sau đó chọn Wash/Spin thì hệ thống không hoạt động nữa — vì sao?

**Trả lời:** `UITask` không bao giờ gửi `CMD_RESET` — long-press handler chỉ
build `CMD_START_WASH`/`CMD_START_SPIN` ([ui_task.c:73-90](../App/UITask/Src/ui_task.c#L73-L90)).
Trong khi đó, transition table chỉ cho phép thoát `FINISH`/`ERROR` bằng
`WASH_EVT_RESET` — mọi event khác bị âm thầm bỏ qua theo đúng spec của SM.
Ghi lại thành **Bug #10** trong `HARDWARE_BUGS.md`.

**Hai phương án đã thảo luận:**
- Phương án A (khuyến nghị): `Manager_HandleCommand()` tự kiểm tra
  `WashingSM_GetState()` — nếu đang FINISH/ERROR, mọi command đến đều hiểu
  là RESET. Không cần IPC mới, giữ đúng nguyên tắc UITask không cần biết
  state máy.
- Phương án B: thêm kênh phản hồi ngược để UITask tự biết state — phức tạp
  hơn, phá vỡ tách biệt kiến trúc sẵn có.

**Trạng thái:** Đã ghi lại phân tích đầy đủ vào `HARDWARE_BUGS.md` (Bug #10)
và `PROJECT_PROGRESS.md`. Chưa implement — đang chờ xác nhận chọn phương án
trước khi sửa code, theo đúng quy trình review của CLAUDE.md.

---

## Session 09 — 2026-08-23 — Mentoring: Event Group, Guard Condition, Level vs Edge

**Chủ đề:** Phiên dạy có hệ thống theo yêu cầu "làm chủ project". Đánh giá
kiến thức hiện tại qua câu hỏi, dạy sâu Event Group (cả kiến thức tổng quát
ngoài project), phát hiện lỗ hổng thiết kế guard condition.

**Bối cảnh:** Bạn cho biết mới chỉ nắm chắc Task và Queue. Đã thống nhất lộ
trình 10 module, kết thúc bằng bài kiểm tra tự thiết kế một project mới.
Bạn cũng yêu cầu: khi dạy khái niệm nào thì phải mở rộng ra kiến thức tổng
quát của khái niệm đó, không bó hẹp trong cách project này dùng.

---

### Q1: Kể lại từng bước từ lúc nhấn PD2 đến khi motor bật

**Trả lời của bạn:** ISR tạo data queue → đánh thức UITask → UITask gửi lệnh
qua queue sang Manager → Manager kiểm tra cửa và mức nước, nếu cửa đóng thì
mở van, đợi đầy thì bật motor, đồng thời gửi data sang OLED.

**Đánh giá:** Phần Task + Queue **đúng** — nắm chắc cơ chế queue vừa truyền
data vừa đánh thức task Blocked.

Hai chỗ sai:

1. Nhầm tên van: nạp nước dùng `BSP_VALVE_INLET` (PD13), không phải van xả
   `BSP_VALVE_DRAIN` (PD14) — van xả chỉ dùng ở state `DRAIN`.
2. **Sai kiến trúc (quan trọng):** hiểu thành "kiểm tra cửa **rồi mới** mở
   van" (if-else tuần tự). Thực tế `Manager_OnEnterFillWater()` mở van **vô
   điều kiện**, còn cửa được giám sát **song song** qua `EVT_ERROR` trong
   `xEventGroupWaitBits`. Đây là mô hình *"cứ hành động, đồng thời canh
   chừng sự cố"*, khác bản chất với if-else.

---

### Q2: `xEventGroupWaitBits` khác `xQueueReceive` thế nào? Dùng Queue để chờ 3 điều kiện thì bất tiện gì?

**Trả lời của bạn:** Một event thì dùng queue được nhưng tốn tài nguyên hơn;
nhiều event thì không dùng queue được.

**Đánh giá:** Trực giác đúng hướng, nhưng lý do chưa trúng. Queue **vẫn dùng
được** cho nhiều loại sự kiện. Khác biệt thật nằm ở bản chất:

- Queue = dòng sự kiện **rời rạc, có thứ tự, đọc 1 lần là mất** (hộp thư).
- Event Group = **trạng thái true/false hiện tại** của nhiều cờ độc lập
  (bảng đèn báo trên taplo).

Điểm chốt: SensorTask poll mỗi 100 ms và set lại bit liên tục — với Event
Group là **idempotent** (`1 OR 1 = 1`, vô hại), với Queue sẽ **chồng item**
gây tràn hàng đợi.

---

### Q3: Set cùng một bit 2 lần liên tiếp vs `xQueueSend` 2 lần — khác gì?

**Trả lời của bạn:** Event group thì action đã trigger ở lệnh đầu; queue
depth ≥ 2 thì lưu 2 lần và consumer xử lý cả 2. Event group "có thể gây lỗi
hoặc không phản ứng với event thứ 2".

**Đánh giá:** Ý chính đúng. Cần làm rõ: **không có lỗi nào cả** — "không
phản ứng với lần thứ 2" chính là **thiết kế đúng**, không phải tác dụng phụ.
Event Group chỉ biểu diễn **level** (đang đúng/sai), **không đếm được**
(không có edge/count). Muốn đếm phải dùng counting semaphore hoặc biến
counter + critical section.

Bổ sung: Queue giữ đủ 2 item **không tự động an toàn** — ở project này an
toàn là nhờ state machine bỏ qua im lặng cặp `(state, event)` không khớp,
tức nhờ thiết kế phòng thủ của FSM chứ không nhờ bản thân Queue.

---

### Q4 (bạn tự đặt): `EVT_TIMEOUT` của state cũ bắn nhầm sang state sau thì có gây lỗi không?

**Đánh giá:** Câu hỏi rất tốt — bạn tự phát hiện một rủi ro thật trong code.
`EVT_TIMEOUT` là **một bit dùng chung cho cả 4 timer** (TD-10), nhưng ý
nghĩa phụ thuộc state đang đọc: ở `FILL_WATER` nghĩa là *lỗi hết giờ*, ở
`WASHING` nghĩa là *giặt xong bình thường*.

Code đã có phòng thủ bằng `xTimerStop()` gọi **trước** khi đổi state. Nhưng
`xTimerStop()` không tức thời — nó chỉ gửi lệnh vào timer command queue, xử
lý bất đồng bộ. Vậy vẫn còn khe hở cực hẹp, **giảm thiểu chứ không loại bỏ**.

**Giải pháp bạn tự đề xuất:** tách thành nhiều bit timeout riêng. Đúng
hướng. Tinh chỉnh: đặt tên theo ngữ nghĩa phase thay vì `TIMEOUT0/1/2`:

```c
#define EVT_FILL_TIMEOUT   (1 << 5)   // FILL_WATER: lỗi
#define EVT_WASH_DONE      (1 << 6)   // WASHING: xong bình thường
#define EVT_DRAIN_DONE     (1 << 7)
#define EVT_SPIN_DONE      (1 << 8)
```

Còn một điểm tinh vi: bit "rác" của state cũ vẫn nằm lại (vì `pdTRUE` chỉ
xóa bit *được chờ và khớp*) → cần `xEventGroupClearBits()` khi vào state
mới, theo đúng pattern `Manager_OnEnterIdle()` đang làm.

**Trạng thái:** Chưa implement — mới là thiết kế trên giấy.

---

### Q5: Thêm điều kiện "cửa đóng VÀ nắp lọc OK" mới cho khởi động — đặt ở đâu, `xWaitForAllBits` là gì?

**Trả lời của bạn:** `pdTRUE`, đặt trong `case WASH_STATE_FILL_WATER` vì đó
là case bắt đầu chu trình giặt.

**Đánh giá:** `pdTRUE` (AND) **đúng**. Vị trí **sai**, 3 lý do:

1. **Quá muộn:** khi vào được `case FILL_WATER` thì
   `Manager_OnEnterFillWater()` đã chạy — van đã mở, nước đang chảy.
2. **Bỏ sót nhánh SPIN:** `IDLE → SPIN` không đi qua `FILL_WATER`, nên chế
   độ vắt sẽ **hoàn toàn không bị kiểm tra**. Lỗ hổng an toàn thật.
   → Nguyên tắc: guard phải đặt ở chỗ **mọi đường đi đều bắt buộc qua**.
3. **Sai công cụ:** dùng `WaitBits` (blocking) làm guard.

**Khái niệm mới — Guard Condition (kiến thức FSM tổng quát):**

Bảng transition hiện tại chỉ có `(state, event) → next_state`. Dạng đầy đủ
theo UML statechart có 4 thành phần:

```
(state, event, [guard]) → next_state / action
```

Guard sai → transition **không nổ**, state giữ nguyên, event bị từ chối.

**Phân biệt sống còn — `WaitBits` vs `GetBits`:**

| | `xEventGroupWaitBits()` | `xEventGroupGetBits()` |
| --- | --- | --- |
| Ý nghĩa | "Tôi **đợi cho tới khi** đúng" | "**Ngay lúc này** đúng hay sai?" |
| Blocking | Có | Không |
| Dùng cho | Chờ sự kiện tương lai | **Guard / precondition** |

Nếu block trong Manager: máy không hiển thị được lý do từ chối, user tưởng
máy hỏng. Nguyên tắc UX embedded: **fail fast with feedback**.

Vị trí đúng là `Manager_HandleCommand()`, dùng `GetBits`:

```c
const EventBits_t required = EVT_DOOR_CLOSED | EVT_FILTER_OK;
EventBits_t bits = xEventGroupGetBits(xSystemEvents);
if ((bits & required) != required) {
    Manager_PostDisplay(currentState, "CLOSE DOOR");
    return;                       // từ chối, KHÔNG đổi state
}
```

Idiom cần nhớ: `(bits & mask) != 0` là **OR**, `(bits & mask) != mask` là **AND**.

**Ba loại điều kiện — đừng nhầm:**

| Loại | Đúng khi nào | Ví dụ |
| --- | --- | --- |
| Precondition (guard) | **Trước khi** bắt đầu | Cửa đóng + nắp lọc OK trước khi chạy |
| Invariant | **Suốt** thời gian ở state | Cửa luôn đóng khi motor quay |
| Postcondition | **Sau khi** hoàn thành | Xả hết nước trước khi vắt |

→ Chính chỗ này giải thích hiểu nhầm ở Q1: bạn nghĩ cửa là *precondition
tuần tự*, thực tế code đang xử lý nó như *invariant*. Hệ thống tốt cần **cả
hai** — project hiện đang có invariant nhưng **thiếu guard**.

**Level flag vs Edge event:** chỉ **level flag** mới làm guard được.
`EVT_DOOR_CLOSED` được SensorTask set/clear liên tục theo trạng thái vật lý
→ là level → hợp lệ. `EVT_FILTER_OK` mới cũng phải thiết kế theo kiểu này.

**Trạng thái:** Guard chưa implement — mới là thiết kế. Chỉ mới sửa 3 lỗi
tài liệu phát sinh khi rà `app_events.h` (xem dưới).

---

### Phát sinh: 3 lỗi tài liệu trong `app_events.h` (đã sửa)

Phát hiện khi đọc file để kiểm chứng trước lúc dạy:

1. **Comment `EVT_TIMEOUT`** ghi timer callback set bit *"from ISR context
   via `xEventGroupSetBitsFromISR()`"* — sai. `Timer_PhaseCallback()` chạy
   trong **Timer Service Task** (task thật, priority `configTIMER_TASK_PRIORITY`)
   và dùng `xEventGroupSetBits()` bản thường. Cùng loại lỗi đã sửa trong
   `SYSTEM_DIAGRAMS.md` trước đây nhưng file này còn sót.
2. **"Usage example"** gợi ý dùng `WaitBits(EVT_DOOR_CLOSED, portMAX_DELAY)`
   làm guard — chính là thiết kế blocking-guard vừa phân tích là sai. Đã
   thay bằng ví dụ phân biệt rõ 2 cách đọc (WaitBits vs GetBits). Ví dụ cũ
   còn ghi sai tên handle (`hSystemEvents` thay vì `xSystemEvents`).
3. **Comment `EVT_ERROR`** ghi *"clears only on CMD_RESET"* — sai kép:
   `case CMD_RESET` đã bị xóa khỏi `Manager_HandleCommand()` (dead code), và
   bit này thực tế được clear ở `Manager_OnEnterIdle()` cùng tự động bởi
   `WaitBits(..., pdTRUE, ...)`.

**Bài học chung:** comment không được compiler kiểm tra — nó **trôi khỏi
code** một cách âm thầm. Đã 4 lần trong project này comment mô tả sai hành
vi thật (Bug #12, và 3 chỗ vừa rồi). Khi đọc code lạ, luôn xác minh comment
bằng code thật trước khi tin.

---

### Q6: Trace kịch bản "mở cửa bỏ đồ → đóng cửa → bấm Start" — chuyện gì xảy ra?

**Trả lời của bạn:** Manager nhận command wash, nếu không có lỗi gì thì máy
hoạt động bình thường, nếu có `EVT_ERROR` thì dừng.

**Đánh giá:** Mới mô tả logic chung, chưa trace giá trị bit thật — mà đó
chính là mấu chốt. Đáp án: máy **KHÔNG** chạy bình thường. Tại bước bấm
Start, `EVT_ERROR` **vẫn đang bằng 1**: cửa đã đóng lại nhưng cái bit thì
không tự đóng theo, vì nhánh `if` trong `sensor_task.c` chỉ set
`EVT_DOOR_CLOSED` mà **không clear `EVT_ERROR`**.

Hệ quả: `xEventGroupWaitBits()` trong `case FILL_WATER` thấy bit đã set sẵn
→ trả về ngay, không block → `FILL_WATER → ERROR` trong vài chục micro giây.

Đã ghi lại đầy đủ thành **Bug #17** trong `HARDWARE_BUGS.md`, kèm 3 phương
án fix (A/B/C) và phân tích ownership.

**Khái niệm tổng quát rút ra — Sticky / Stale Level Flag:**

Cờ level chỉ có đường lên mà không có đường xuống sẽ mô tả **quá khứ** thay
vì hiện tại. Đặc điểm khiến nó khó bắt: không crash, không warning, phụ
thuộc *lịch sử thao tác*, và triệu chứng xuất hiện ở module cách xa nguyên
nhân. Nguyên tắc phòng tránh: **cờ level phải được duy trì đối xứng ở mọi
nhánh của producer**, và **mỗi bit nên có đúng một chủ sở hữu** chịu trách
nhiệm clear.

**Trạng thái:** Chưa fix — người dùng quyết định ưu tiên học để làm chủ
project trước, sửa sau. Người dùng cũng cho biết hệ thống hiện còn một số
lỗi khác chưa liệt kê; sẽ xử lý ở phiên riêng.

---

## Session 10 — 2026-08-23 — Mentoring buổi 2: Tick/SysTick & Context Switch

**Chủ đề:** Nguồn thời gian vật lý của FreeRTOS (SysTick, HSI vs HSE), cơ chế
đánh thức Timer Service Task, phân tích đầy đủ một chuỗi context switch thật,
và tách bạch tầng scheduler với tầng logic ứng dụng.

**Kết quả:** Ghi thành **DD-05** trong `LEARNING_NOTES.md` Part II. Không có
thay đổi code nào trong buổi này.

---

### Q1: Software Timer — ai quản lý nó, nguồn của nó từ đâu (clock nội hay ngoại)?

**Bối cảnh:** DD-04 (buổi 1) mới trả lời tầng **logic** — Timer Service Task
quản lý danh sách timer. Câu hỏi này chỉ đúng vào lỗ hổng còn lại: tầng **vật
lý**, cái gì thực sự tạo ra khái niệm "thời gian".

**Trả lời:**

Có **hai tầng quản lý** khác nhau, không được gộp:

| Tầng | Ai quản lý |
| --- | --- |
| Thời gian (vật lý) | **SysTick** — bộ đếm 24-bit nằm trong lõi Cortex-M, thuộc chuẩn ARM chứ không phải ngoại vi của ST |
| Logic | **Timer Service Task** — dựa vào nhịp SysTick để biết timer nào tới hạn |

Chuỗi đầy đủ: `HSI 16 MHz → SystemCoreClock → SysTick (nạp 15999, tràn mỗi
1 ms) → xPortSysTickHandler() → xTaskIncrementTick() → daemon chuyển Ready →
Timer_PhaseCallback()`.

**Nguồn clock của project này = NỘI.** Kiểm chứng bằng `SystemClock_Config()`
trong `main.c`: `OscillatorType = RCC_OSCILLATORTYPE_HSI`, `PLL.PLLState =
RCC_PLL_NONE`, `SYSCLKSource = RCC_SYSCLKSOURCE_HSI` → HSI (RC nội ~16 MHz),
không dùng thạch anh ngoài (HSE), không nhân tần bằng PLL.

**Ba bằng chứng source đã trích:**

- `port.c:826-828` — công thức nạp thanh ghi SysTick.
- `FreeRTOSConfig.h:130` — `#define xPortSysTickHandler SysTick_Handler`
  (chính là pattern weak-symbol của Bug #4; thiếu dòng này → tick không tăng →
  treo toàn hệ thống).
- `timers.c:817` — `vQueueWaitForMessageRestricted(...)`: daemon **không
  polling**, nó tự tính thời gian ngủ và dùng đúng cơ chế block-with-timeout
  chung của kernel.

**Kết luận quan trọng nhất:** Timer Service Task **không có phần cứng riêng,
không có đặc quyền lịch trình nào** — chỉ là một task bình thường biết tự tính
thời gian ngủ.

---

### Q2: Bài toán context switch — `xWashTimer` hết hạn khi UITask đang chạy

**Đề bài:** `xWashTimer` (20 s) vừa hết hạn. WashingManager (prio 3) Blocked
trong `WaitBits`. UITask (prio 2) Running. Daemon (prio 2) Blocked. Hỏi thứ tự
chạy và số lần context switch tới khi `Manager_OnEnterDrain()` thực thi.

**Trả lời của bạn:** UITask dừng để Timer Service Task chạy → callback set
event → WashingManager chạy → sau khi WashingManager block lại thì các task
prio 2 chạy tiếp. **2 lần context switch.**

**Đánh giá:** Chuỗi logic **đúng**. Con số **thiếu** — thực tế tối thiểu **4**.

**Hai điểm bổ sung:**

**(a) UITask nhường CPU KHÔNG phải vì daemon ưu tiên cao hơn.** Daemon = prio
2, bằng UITask. `tasks.c:4787` yêu cầu `pxTCB->uxPriority > pxCurrentTCB->
uxPriority` → `2 > 2` là FALSE, nhánh này không kích hoạt. Cái làm nó switch là
khối **time-slicing** ở `tasks.c:4810-4813` (`configUSE_TIME_SLICING` mặc định
`1`). → Nếu ai tắt time slicing, daemon sẽ nằm Ready chờ tới khi UITask tự
block, callback trễ tuỳ ý. Muốn tất định phải đặt `configTIMER_TASK_PRIORITY`
cao hơn mọi task dùng timer.

**(b) Daemon bị cướp CPU GIỮA CHỪNG callback** — đây là chỗ bị bỏ sót.
`xEventGroupSetBits()` bọc trong `vTaskSuspendAll()`/`xTaskResumeAll()`
(`event_groups.c:567,639`). WashingManager được gỡ block bên trong vùng suspend
nên chỉ đặt cờ `xYieldPendings[0] = pdTRUE` (`tasks.c:4632-4636`), rồi
`xTaskResumeAll()` thực hiện yield → switch xảy ra **trước khi
`Timer_PhaseCallback()` kịp return**. Daemon vì thế **bắt buộc phải được chạy
lại sau** để hoàn tất callback và dọn dẹp.

| # | Từ → Đến | Nguyên nhân |
| --- | --- | --- |
| 1 | UITask → daemon | tick + time-slicing (2 == 2) |
| 2 | daemon → WashingManager | `xTaskResumeAll()` giữa callback |
| 3 | WashingManager → task prio 2 | `WaitBits` block lại |
| 4 | … → daemon | round-robin |

Thứ tự giữa UITask và daemon ở bước 3 phụ thuộc con trỏ `pxIndex` của
ready-list (`tasks.c:189-191`) → **không tất định theo góc nhìn thiết kế, đừng
bao giờ phụ thuộc vào nó**.

---

### Q3: Sao UITask lại chạy được khi máy đang giặt? Nhấn nút lúc đó phải vô tác dụng chứ?

**Đánh giá:** Câu hỏi hay nhất buổi — nó lộ ra chỗ đang **trộn lẫn hai tầng**.

| Tầng | Câu hỏi | Ai quyết định |
| --- | --- | --- |
| Scheduler | UITask có được cấp CPU không? | Priority + Blocked/Ready. **Không biết gì về "đang giặt"** |
| Ứng dụng | Lệnh đó có tác dụng không? | State machine của WashingManager |

Scheduler hoàn toàn không có khái niệm "máy đang giặt". Nó chỉ thấy UITask prio
2 vừa có message → Ready → tới lượt thì chạy.

**"Vô tác dụng" thực ra được cài đặt bằng 3 mảnh trong code:**

1. UITask **vẫn chạy đủ**, vẫn `xQueueSend(xCommandQueue, ...)`
   (`ui_task.c:69-81`).
2. WashingManager ở state WASHING **không đọc `xCommandQueue`**
   (`washing_manager.c:291-297`) — chỉ đọc ở IDLE/FINISH/ERROR (`:249-254`).
   Lệnh nằm im trong queue.
3. Vì nằm im nên sẽ "nổ chậm" khi về IDLE → code chặn bằng
   `(void)xQueueReset(xCommandQueue)` trong `Manager_OnEnterIdle()` /
   `OnEnterFinish()` / `OnEnterError()` (`washing_manager.c:66-67`).

**Pattern tổng quát rút ra:** *lọc sự kiện ở phía consumer (state machine),
không phải ở phía producer.* Producer cứ đơn giản; toàn bộ tri thức về ngữ cảnh
nằm gọn một chỗ — đúng Design Rule #1. Làm ngược lại (cho UITask tự hỏi "máy
đang giặt à?") thì logic trạng thái bị rải ra 2 nơi và sẽ có lúc bất đồng.

**Hệ quả kiểm chứng được trên board:** nhấn NAVIGATE (PA0) lúc đang giặt thì
con trỏ menu trên OLED **vẫn di chuyển**, vì UITask gửi thẳng `xDisplayQueue`
không qua WashingManager (`ui_task.c:60-67`). Feature hay bug là quyết định
thiết kế, không phải tai nạn.

---

### Q4: "Software Timer chưa xong mà" — phản hồi về nhịp dạy

**Bối cảnh:** Sau khi viết DD-05 và đánh dấu ✅ cho Software Timer trong bảng
lộ trình, người dùng phản hồi hai lần liên tiếp:

1. *"Software Timer chưa xong mà"*
2. *"ý mình là kiến thức về Software Timer còn nhiều mà, sao chuyển sang bài
   học khác sớm thế"*

**Đánh giá — phản hồi đúng.** Rà lại DD-04 (mục 1–14) thì thấy mới dạy phần
**dùng API**, thiếu hẳn phần **nội tại** và **thực chiến**. Đã bổ sung mục
15–26, gồm cả **một đính chính**.

**Nội dung đã bổ sung:**

| Mục | Nội dung |
| --- | --- |
| 15 | Vòng đời timer — trạng thái **Dormant/Active** (thuật ngữ chính thức, `timers.h:111`), 3 bit `ucStatus` |
| 16 | **Vì sao có HAI danh sách timer** — xử lý tick overflow, `prvSwitchTimerLists()` đổi con trỏ O(1) |
| 17 | Vòng lặp thật của daemon chỉ 3 dòng; thứ tự "xử lý timer hết hạn TRƯỚC lệnh trong queue" |
| 18 | Command queue đầy; gọi `xTimerStart()` trước scheduler thì `xTicksToWait` **bị ép về 0** |
| 19 | Static allocation — `configSUPPORT_STATIC_ALLOCATION` mặc định 0 → project này **không dùng được** |
| 20 | ⚠ **Đính chính mục 9** |
| 21 | 6 API còn thiếu ở bảng mục 6 |
| 22 | **Độ phức tạp O(n)** khi chèn; so sánh sorted list / min-heap / timer wheel / RB-tree |
| 23 | Callback chạy trên **stack của daemon** (1040 B) — bug tràn stack ở task mình chưa từng viết |
| 24 | **Reentrancy** — callback↔callback không cần mutex, callback↔task thì cần |
| 25 | 8 pattern thực chiến (debounce, watchdog mềm, end-of-frame, backoff, throttle...) |
| 26 | 5 trace hook timer; đo độ trễ daemon bằng cặp `COMMAND_SEND`→`COMMAND_RECEIVED` |

**⚠ Đính chính quan trọng (mục 20):** buổi 1 dạy
`xTimerPendFunctionCallFromISR()` như API sẵn dùng, nhưng
`INCLUDE_xTimerPendFunctionCall` mặc định `0` (`FreeRTOS.h:283-284`) và project
không khai → **hàm không được biên dịch**, gọi sẽ lỗi linker. Đây là **cùng
một lỗi** đã mắc ở DD-01 mục 5 với `xEventGroupSetBitsFromISR` mà vẫn lặp lại.
→ Nguyên tắc: *"API tồn tại trong FreeRTOS" ≠ "API dùng được trong build
này"* — luôn kiểm tra macro `INCLUDE_*` / `configUSE_*` trước khi dạy.

**Phát hiện thêm về code project:** cả **10 lời gọi**
`xTimerStart()`/`xTimerStop()` trong `washing_manager.c` đều bỏ qua giá trị trả
về — đúng lỗi kinh điển số 4 trong checklist DD-04 mục 12. Chưa gây hậu quả
(`configTIMER_QUEUE_LENGTH = 10`, chỉ 4 timer) nhưng đã ghi nhận.

**Bài học phương pháp (đã lưu vào memory):** không tự đánh dấu một chủ đề là
✅ hoàn thành rồi chuyển sang chủ đề khác — **liệt kê thẳng những gì còn thiếu
cho người học thấy và để họ quyết định khi nào chủ đề xong**. Trạng thái trong
bảng lộ trình mặc định để 🔶.

**Còn lại của DD-04 lúc đó:** (F) test logic phụ thuộc thời gian, (G) so sánh
với timer của Linux / Zephyr / bare-metal. Đã dạy tiếp ở Q5, Q6 dưới đây —
DD-04 khép lại trong buổi này.

---

### Q5: Tại sao mình đọc source FreeRTOS nhiều vậy?

**Trả lời:** Hai lý do. (1) Chuẩn "sách giáo khoa, không bịa" mà người dùng
đặt ra — bắt nguồn từ 7 lỗi thật đã mắc khi viết theo trí nhớ (sai tên macro,
thiếu điều kiện biên dịch, trích sai số liệu). (2) Sâu hơn: ba loại thông tin
chỉ đọc được, không suy luận được — cấu hình riêng của project (macro
`INCLUDE_*`/`configUSE_*` không khai thì lấy default ẩn, ví dụ
`INCLUDE_xTimerPendFunctionCall=0` khiến một API "không tồn tại" trong build
này), chi tiết phụ thuộc phiên bản kernel (V11.1.0), và số liệu chính xác
(44 byte `Timer_t`, 1040 byte stack daemon...) mà tài liệu sẽ dùng để tính
toán thật về sau.

Thừa nhận có phần dư: không phải lần tra cứu nào cũng cần — chỉ bắt buộc cho
cấu hình/API compile được và cơ chế nội tại; khái niệm chung, pattern, câu
hỏi phỏng vấn thì không cần đọc source mỗi lần.

---

### Q6: Test unit cho `washing_sm.c` — có loại bug nào không bắt được?

**Trả lời của bạn:** "không biết" — hỏi ngược lại thay vì đoán.

**Đáp án:** **Bug #17** (sticky `EVT_ERROR` trong `sensor_task.c:23-28`).
`washing_sm.c` xử lý `WASH_EVT_ERROR` hoàn toàn đúng khi nhận được nó — test
sẽ xanh. Nhưng bug nằm ở chỗ **sự kiện đó bị sinh ra sai** (bit không được
clear khi đóng cửa lại), và việc sinh sự kiện nằm ở `sensor_task.c`, ngoài
seam mà `washing_sm.c` test qua (đầu vào `WashEvent_t`, đầu ra callback).

**Nguyên tắc rút ra:** seam dùng để tiêm phụ thuộc test cũng chính là ranh
giới vùng mù của test — trả lời được "xử lý đúng sự kiện chưa" nhưng không
trả lời được "có sinh đúng sự kiện không". Phát hiện thêm cùng hình dạng lỗi
ở `EVT_WATER_FULL` (`sensor_task.c:30-32`, cũng thiếu nhánh `else` để clear).
Đã ghi thành DD-04 mục 29 kèm bảng phân loại bug theo tầng và đề xuất tầng
test "Component" (cảm biến giả) để bắt được loại này.

---

## Session 11 — 2026-08-24 — Task Notification & phát hiện tài liệu lỗi thời

**Chủ đề:** Bắt đầu module Task Notification (mentoring roadmap #4); trên
đường học phát hiện và sửa một phần Phase 3 của `LEARNING_NOTES.md` đã lỗi
thời so với code thật.

---

### Q1: Đường dẫn ISR PA0 → `xButtonQueue` → UITask trong project — có nên thay bằng Task Notification không?

**Trả lời của bạn:** Không nên, vì "button có thể gửi nhiều tín hiệu mà task
notify chỉ bắt được 1 cái, sẽ gây ra miss command".

**Đánh giá:** Kết luận đúng, cơ chế nêu ra chưa chính xác. Task Notification
không "rớt" sự kiện thứ 2 như queue tràn — `eSetBits`/`eIncrement` đều
**tích luỹ** (`ulNotifiedValue |= ulValue` hoặc `++`). Vấn đề thật là **mất
danh tính và thứ tự**: nếu `NAVIGATE=bit0`, `CONFIRM=bit1` và cả hai đến
trước khi task đọc, giá trị cuối chỉ còn `0b11` — không biết đã bấm NAVIGATE
mấy lần, không biết thứ tự. Ngược lại, nếu chỉ có **một loại tín hiệu duy
nhất lặp lại** thì `eIncrement` (`xTaskNotifyGive`/`ulTaskNotifyTake`) không
mất gì cả — đây chính là mẫu "counting semaphore nhanh hơn" chính thức của
FreeRTOS. Quy tắc chọn: nhiều loại cần phân biệt + giữ thứ tự → Queue; một
loại lặp lại, chỉ cần đếm → Task Notification.

---

### Q2: Đường dẫn UITask → `xCommandQueue` → WashingManager — ứng viên tốt hơn?

**Đánh giá câu hỏi mở — chưa có câu trả lời trực tiếp từ người dùng, phân
tích được đưa ra kèm câu hỏi tiếp theo:** Đây là ứng viên **tốt hơn** case
Q1 vì WashingManager chỉ đọc queue lúc IDLE/FINISH/ERROR, xử lý đúng 1 lệnh
rồi rời state ngay, và thiết kế hiện tại **đã tự vứt bỏ backlog** qua
`xQueueReset()` ở `OnEnterIdle/OnEnterFinish/OnEnterError` — tức là chỉ quan
tâm lệnh mới nhất, đúng ngữ nghĩa `eSetValueWithOverwrite`. Có thể đóng gói
`command|mode|menuIndex` vào một `uint32_t` và bỏ hẳn 3 lời gọi
`xQueueReset()` rải rác. Đánh đổi: Task Notification cần UITask biết handle
cụ thể của WashingManager (khớp nối chặt hơn queue trung lập).

---

### Q3: "Từ giờ không cần trace lại source FreeRTOS sau mỗi câu hỏi, dựa vào kiến thức trong các file này"

**Ghi nhận:** Đã lưu thành memory `feedback_reuse_verified_knowledge` — chỉ
tra lại `OS/FreeRTOS/` khi gặp khẳng định thật sự mới hoặc nghi ngờ điều đã
ghi trước đó sai; kiến thức đã trích dẫn trong `LEARNING_NOTES.md`/`QA_LOG.md`
thì dùng lại trực tiếp. Đọc code của project (`App/`, `Core/`) không thuộc
phạm vi hạn chế này — vẫn đọc bình thường mỗi khi cần.

---

### Q4: "Sao file learning vẫn còn phần này" (ảnh chụp `Design: Short Press vs Long Press`)

**Phát hiện:** Đối chiếu `bsp_button.c` thật thì mục "Design: Short Press vs
Long Press" ở Phase 3 (`LEARNING_NOTES.md`) mô tả kiến trúc **đã không còn
tồn tại** — một nút PA0 duy nhất phân biệt short/long press bằng thời lượng
nhấn giữ (`duration >= BUTTON_LONG_PRESS_MS`). Code thật hiện dùng **hai nút
vật lý riêng biệt** (PA0 = navigate sau debounce, PD2 = confirm ngay khi có
falling edge), không còn đo thời lượng. Đây là hệ quả của Bug #8 (PA0 dùng
chung với door sensor gây race condition) và hành trình đổi chân
PE1→PD4→PD2 (Bug #14/#15/#16) — Phase 3 được viết **trước** các thay đổi đó
và không ai quay lại đối chiếu.

**Phát hiện thêm:** sơ đồ gốc "ISR → Queue → Task" cũng bỏ sót một tầng thật
— `bsp_button.c` không hề gọi `xQueueSendFromISR()` trực tiếp; nó gọi qua
con trỏ hàm callback (đăng ký bởi `UITask`), và `xQueueSendFromISR()` +
`portYIELD_FROM_ISR()` thực chất nằm trong `Button_EventCallback()` ở
`ui_task.c`. Đây chính là cơ chế giữ Design Rule #3 ("Drivers must not
depend on FreeRTOS") — bị thiếu hoàn toàn trong tài liệu gốc.

**Phát hiện phụ:** `BUTTON_LONG_PRESS_MS` (`app_config.h:35`, giá trị
`1500U`) vẫn còn định nghĩa nhưng **không còn nơi nào trong code sử dụng** —
dead code sau refactor.

**Đã sửa:** Viết lại toàn bộ mục "Design: ISR → Queue → Task Pattern" (đổi
tên thành "ISR → Callback → Queue → Task Pattern"), "Design: Short Press vs
Long Press" (đổi tên thành "Navigate vs Confirm", có bảng so sánh thiết kế
cũ/mới), và "What I Should Learn" trong `LEARNING_NOTES.md` Phase 3, kèm ghi
chú lịch sử trỏ về Bug #8 và #14/#15/#16 để không mất giá trị tham chiếu.

**Bài học:** đây là lần thứ hai phát hiện Part I (ghi theo phase xây dựng)
lỗi thời so với code thật — lần trước là 3 comment trong `app_events.h`
(Session 09). Xác nhận lại nguyên tắc đã ghi ở "Bài học phương pháp" mục 2:
tài liệu không được compiler kiểm tra, chỉ phát hiện được khi chủ động đối
chiếu lại với code.

---

## Session 12 — 2026-08-29 — Thiết kế tài liệu nhập môn START_HERE.md

**Chủ đề:** Không phải kiến thức FreeRTOS mới, mà là **thiết kế tài liệu học**:
viết file dẫn đường thế nào để người chưa từng dùng RTOS hiểu được project.

---

### Q1: Người đọc mục tiêu của file learning là ai?

**Ba phương án đưa ra:** (1) chính bạn — ôn lại, (2) người mới hoàn toàn, (3)
người phỏng vấn / đồng nghiệp cần nắm kiến trúc trong 30 phút.

**Bạn chọn:** (2) — người mới hoàn toàn: biết C, biết HAL cơ bản, chưa từng
dùng RTOS.

**Ràng buộc kéo theo:**

| Ràng buộc | Hệ quả lên cách viết |
| --- | --- |
| Chưa từng dùng RTOS | Không dùng *block / preempt / context switch / yield / ISR-safe* trước khi định nghĩa. Mỗi thuật ngữ có **một** điểm định nghĩa duy nhất |
| Biết HAL, quen superloop | HAL là điểm tựa bắc cầu: mỗi khái niệm mới mở đầu bằng "trong superloop bạn làm thế này… nó hỏng ở đây" |
| Đọc để hiểu project này | Không ví dụ bịa — mọi đoạn code trích thật từ `App/`, kèm `file:dòng` |

---

### Q2: Người học có board STM32F411E-DISCO trong tay khi đọc không?

**Bạn chọn:** có board, nạp được firmware.

**Hệ quả:** mục "Thí nghiệm" của mỗi tầng viết dạng **mệnh lệnh** (nạp code,
bấm nút này, quan sát LED kia), không cần kèm sẵn số liệu đo trước. File mỏng
hơn và học sâu hơn, đổi lại người không có board sẽ đọc bị hụt.

---

### Chẩn đoán trước khi viết

Đối chiếu bộ tài liệu hiện có: `LEARNING_NOTES.md` (3352 dòng) là **reference**
— sắp theo primitive (Phần A) và theo thứ tự lịch sử xây dựng (Phần B). Cả hai
thứ tự đều là thứ tự của người đã biết. Người mới mở ra sẽ gặp
`struct EventGroupDef_t` ngay §1.1 — kiến thức trang 200 đặt ở trang 1.

Thiếu sót thật: **không có cửa vào**. 6 file trong `docs/` + 2 file gốc, không
file nào nói "bắt đầu ở đây, đọc theo thứ tự này".

**Quyết định:** không sửa `LEARNING_NOTES.md`. Viết thêm một file mỏng làm
xương sống dẫn đường (`docs/START_HERE.md`), **link** sang các file kia thay vì
chép lại. Quy tắc chống trùng lặp: *một sự thật chỉ nằm ở một chỗ.*

---

### Bảy nguyên tắc đã chốt cho START_HERE.md

1. **Vấn đề trước, primitive sau** — mở chương bằng nhu cầu cảm nhận được, không
   bằng định nghĩa API.
2. **Cho xem cách làm sai trước** — mỗi primitive sinh ra để thay một cách ngây
   thơ; phải thấy nỗi đau mới nhớ được giá trị của công cụ.
3. **Neo vào code thật của repo**, kèm `file:dòng` — không pseudo-code.
4. **Mỗi tầng phải quan sát được bằng mắt** — chỉ bằng LED, OLED và nút bấm.
5. **Dự đoán trước, đo sau** — chỗ dự đoán sai là chỗ mô hình tư duy sai.
6. **Một tầng = một khái niệm mới**, không dùng thuật ngữ chưa định nghĩa.
7. **Nông ở đây, sâu ở LEARNING_NOTES** — cắt độ sâu ở mức đủ hiểu code project.

---

### Khung 7 tầng đã chốt

| Tầng | Tên | Câu hỏi trung tâm |
| --- | --- | --- |
| 0 | Sờ vào hệ thống | Cái máy này làm gì? |
| 1 | Vì sao cần RTOS | Ba việc cùng lúc — superloop hỏng ở đâu? |
| 2 | Bốn task của project | Ai làm gì, ai quan trọng hơn ai? |
| 3 | Task nói chuyện với nhau | Dữ liệu đi từ task này sang task kia bằng gì? |
| 4 | Chờ nhiều điều kiện | Chờ "cửa đóng VÀ nước đầy" kiểu gì? |
| 5 | Trái tim: State Machine | Máy giặt "đang ở đâu" lưu ở chỗ nào? |
| 6 | Từ chân MCU đến task | Nút bấm đi qua những bước nào? |

Khuôn cố định 7 mục cho mọi tầng: Hiện tượng → Cách ngây thơ → Ý tưởng → Code
thật → Thí nghiệm → Tự kiểm tra → Đào sâu.

---

### Q3: Có đưa phần công cụ debug (SystemView / RTT / logic analyzer) vào lộ trình không?

**Bạn quyết:** chưa cần, để sau.

**Đã sửa:** bỏ tầng "Nhìn thấy hệ thống chạy" khỏi lộ trình (còn 7 tầng: 0–6),
thêm ghi chú "Để sau" ở `START_HERE.md` §0.3, và đổi comment hai dòng
`SEGGER_SYSVIEW_*` trong trích đoạn `main.c` thành "chưa dùng tới → bỏ qua".

**Hệ quả về mặt sư phạm — đáng ghi lại:** ràng buộc này *tốt cho tài liệu*.
Toàn bộ 7 tầng giờ chỉ cần board + LED + nút bấm, không phụ thuộc công cụ nào
ngoài STM32CubeIDE. Người học không bị chặn ở Tầng 0 vì chưa cài được
SystemView. Nguyên tắc số 4 ("mỗi tầng phải quan sát được bằng mắt") vẫn giữ
nguyên — chỉ đổi phương tiện quan sát từ trace sang đèn LED, mà đèn LED thì
trực quan hơn với người mới.

Khi nào bổ sung tầng debug thì đánh số tiếp là Tầng 7 — kiến thức nền đã có sẵn
ở `LEARNING_NOTES.md` §B.8 và Session 06 của file này.

---

### Đã viết: Tầng 0 và Tầng 1

Viết dần từng tầng theo quy tắc "không sinh cả project một lượt" trong
`CLAUDE.md` — chỉnh khuôn ở 2 tầng đầu rẻ hơn chỉnh ở tầng thứ 8.

**Thí nghiệm chủ lực của Tầng 1** (đóng góp mới của phiên này): đổi
`vTaskDelay()` → `HAL_Delay()` trong `sensor_task.c:34`, dự đoán rồi đo. Suy
luận từ bảng priority (WashMgr 3, UITask 2, Sensor 2, OLED 1) và
`configUSE_TIME_SLICING = 1`:

- WashMgr (prio 3) — vẫn chạy, priority cao hơn nên chen vào được.
- UITask (prio 2) — vẫn chạy, **bằng** priority nên được chia CPU theo vòng.
- OLEDTask (prio 1) — **starvation**, không bao giờ được chọn → màn hình đứng.

Giá trị sư phạm: chương trình **không crash**. Đèn vẫn chạy, nút vẫn ăn, chỉ
màn hình chết — đúng loại bug mà người mới sẽ đi tìm nhầm trong driver OLED.
Cho thấy `HAL_Delay()` trong task không phải lỗi chức năng mà là lỗi thiết kế,
và lỗi thiết kế thì im lặng.

Ghi chú: project chuyển HAL timebase sang TIM riêng
(`Core/Src/stm32f4xx_hal_timebase_tim.c`) nên `HAL_Delay()` **vẫn chạy đúng
chức năng** — chính điều đó làm bug khó nhận ra hơn.

---
### Q4: Duyệt Tầng 0–1 — nhịp độ đạt, nhưng cách trình bày đáp án phải sửa

**Phản hồi của bạn:** "nhịp độ tầng 0 1 ổn rồi, tuy nhiên các câu hỏi cần có
trả lời và giải thích rõ ngay ở bên dưới".

**Chẩn đoán:** bản đầu để đáp án trong khối `<details><summary>` gấp lại ở cuối
mỗi tầng. Hai vấn đề:

1. **Mất mạch đọc** — người học phải cuộn xuống cuối tầng tìm đáp án rồi cuộn
   ngược lại.
2. **`<details>` không phải markdown chuẩn** — nó là HTML nhúng. GitHub render
   được, nhưng bản preview của STM32CubeIDE và một số editor thì không; khi đó
   đáp án **biến mất hoàn toàn** hoặc hiện ra dạng thẻ HTML thô.

**Đã sửa:**

- Bỏ toàn bộ khối `<details>` trong `START_HERE.md`.
- Cấu trúc mới cho mỗi câu: `**Câu N. <câu hỏi>**` → `**Trả lời:** ...` →
  giải thích dày → `---` ngăn cách.
- **Viết lại giải thích dày hơn hẳn bản cũ**, không chỉ chuyển vị trí:

| Câu | Bản cũ | Bản mới |
| --- | --- | --- |
| 0.6 Q1 (LED thay motor) | 3 dòng | 3 lợi ích cụ thể, trong đó lợi ích thứ 3 dẫn thẳng sang lý do tồn tại của `App/BSP/` (bắc cầu sang Tầng 2) |
| 0.6 Q2 (ai điều khiển) | 2 dòng | Bảng so sánh bare-metal vs RTOS + giải thích `vTaskStartScheduler()` chỉ return khi thiếu heap tạo Idle task |
| 0.6 Q3 (cái gì đếm 10 s) | 3 dòng | Nêu rõ **mâu thuẫn** timer giải quyết: không thể vừa đếm giờ vừa chờ sự kiện trên một luồng |
| 1.6 Q1 (priority ≠ CPU) | 5 dòng | Bảng "priority cao nghĩa là / không nghĩa là" + quy tắc gán priority thực tế (theo độ gấp, không theo độ quan trọng) |
| 1.6 Q2 (`vTaskDelay` 100 ms) | 4 dòng | Sơ đồ 3 mốc `Blocked → Ready → Running`, phân biệt điều RTOS đảm bảo và không đảm bảo, nối sang drift và `vTaskDelayUntil` |
| 1.6 Q3 (stack 512 bytes) | 6 dòng | Thêm: vì sao crash lệch chỗ, vì sao compiler bó tay (`sprintf` ngốn stack), bảng 2 công cụ phòng ngừa, và cách làm thực tế: cấp dư → đo high-water-mark → cắt |

- Kết quả Thí nghiệm B cũng bỏ `<details>`, thay bằng tiêu đề
  **"⛔ Dừng lại — viết dự đoán ra giấy xong mới đọc tiếp"** rồi trình bày công
  khai. Giữ được kỷ luật dự-đoán-trước mà không phụ thuộc HTML.

**Nguyên tắc bổ sung vào bộ quy tắc viết tài liệu (thành nguyên tắc số 8):**
đáp án và giải thích luôn nằm ngay dưới câu hỏi. Tài liệu học không được dùng
cơ chế "giấu đi rồi mở ra" — nó đánh đổi tính chắc chắn của việc đọc lấy một
chút gọn gàng hình thức, và đó là đánh đổi sai.

---

### Q5: Viết Tầng 2 — "Bốn task của project"

**Yêu cầu:** viết tiếp theo khuôn đã duyệt.

**Câu hỏi trung tâm được chọn cho tầng này:** không phải "bốn task này làm gì"
(cái đó tra `SYSTEM_DIAGRAMS.md` là ra), mà **"chia hệ thống thành bao nhiêu
task, theo tiêu chí gì?"** — đây mới là chỗ người mới thực sự mắc kẹt, và là
kiến thức mang được sang project khác.

**Cách ngây thơ được chọn để bác bỏ:** chia task **theo đồ vật**
(`MotorTask`, `ValveTask`, `LedTask`...). Đây là sai lầm tự nhiên ngay sau khi
học xong Tầng 1 — biết RTOS có nhiều task thì lao sang thái cực chia quá nhỏ.
Ba lý do bác bỏ, xếp tăng dần: tốn RAM → task rỗng không có việc → **trạng thái
hệ thống bị xé nát, không ai sở hữu, buộc phải dùng biến toàn cục**.

**Bốn tiêu chí chia task (nội dung chính của tầng):**

| # | Tiêu chí | Câu hỏi |
| --- | --- | --- |
| 1 | Nguồn chờ (blocking source) | Việc này nằm chờ cái gì? |
| 2 | Nhịp (rate) | Định kỳ hay theo sự kiện? |
| 3 | Độ gấp (latency) | Chậm 100 ms có sao không? |
| 4 | Quyền sở hữu (ownership) | Tài nguyên chung này ai được đụng? |

Tiêu chí 1 đặt lên đầu vì nó giải thích *chính xác* vì sao project này có 4 task:
bốn task nằm chờ bốn nguồn khác nhau (`xButtonQueue` / `xDisplayQueue` /
`xCommandQueue` / `vTaskDelay`).

**Hai thí nghiệm:**

- **A (không sửa code)** — bấm PD6 ở `IDLE` (không có gì xảy ra) rồi bấm PD6 ở
  `FILL_WATER` (chuyển trạng thái ngay). Cùng một nút, cùng đoạn code trong
  `sensor_task.c`, phản ứng khác nhau vì **WashingManager đổi nguồn chờ theo
  trạng thái**. Dạy được ý cốt lõi: *lọc sự kiện bằng lựa chọn nguồn chờ, không
  bằng chuỗi `if`.*
- **B (sửa 1 dòng)** — đổi `MOTOR_PIN` từ `GPIO_PIN_12` sang `GPIO_PIN_13` trong
  `bsp_motor.c:40`. Kết quả kiểm chứng được bằng mắt: đèn xanh lá **không sáng
  lần nào** trong cả chu trình, đèn cam gánh vai trò motor. Điểm dạy: **0 dòng
  phải sửa trong `washing_manager.c`** — giá trị lớp BSP được người học tự tay
  chứng minh, không phải nghe kể.

  Thí nghiệm B còn có tác dụng phụ đắt giá: PD13 giờ có **hai chủ** (van + motor),
  và kết quả chỉ đúng vì thứ tự lệnh trong callback tình cờ hợp lý
  (`Valve_Close()` chạy trước `Motor_Start()`). Đây là tiêu chí 4 (ownership)
  áp dụng ở mức chân MCU thay vì mức task — nối liền hai ý mà không cần thêm
  khái niệm mới.

**Câu 3 của mục Tự kiểm tra nối vào lịch sử thật của project:** "vì sao
WashingManager không tự đọc PD5?" → dẫn tới **Bug #8** (`bsp_sensor.c` từng đọc
cửa qua PA0, trùng chân nút menu). Tài liệu học dùng được bug thật của chính
project làm ví dụ là thứ sách vở không có.

**Giữ kỷ luật "một tầng một khái niệm":** code trích ở §2.4 có
`xQueueReceive()` và `xEventGroupWaitBits()`, nhưng tầng này **không giải thích
cơ chế** của chúng — chỉ dùng chúng làm bằng chứng cho "mỗi task chờ một nguồn
khác nhau", kèm ghi chú trỏ sang Tầng 3 và Tầng 4.

---


## Session 06 — 2026-06-26 — Phase 8: SEGGER SystemView

**Chủ đề:** RTT transport, queue registry, vQueueAddToRegistry, SEGGER_SYSVIEW_Conf/Start.

*Lưu ý: Session này tự hỏi và tự trả lời vì người học chưa có kiến thức nền về SystemView.*

---

### Q1: SEGGER SystemView truyền data từ MCU về PC qua cơ chế gì? Tại sao không dùng UART?

**Trả lời:** SystemView dùng SEGGER RTT (Real-Time Transfer). RTT ghi event vào ring buffer trong RAM. ST-Link đọc trực tiếp RAM qua SWD — CPU không tham gia. UART mất ~1ms cho 10 ký tự (làm chậm task), RTT ghi RAM chỉ mất vài chục ns (không ảnh hưởng timing).

---

### Q2: `vQueueAddToRegistry(xButtonQueue, "ButtonQ")` làm gì? Nếu không gọi thì sao?

**Trả lời:** Gán tên cho queue handle để SystemView hiển thị. Nếu không gọi, SystemView chỉ thấy địa chỉ RAM thô như `0x20003A40` thay vì `"ButtonQ"` — rất khó đọc khi debug. Hàm này không ảnh hưởng hoạt động queue, chỉ phục vụ debug tool.

---

### Q3: `configQUEUE_REGISTRY_SIZE` là gì và cần set bao nhiêu?

**Trả lời:** Số lượng queue/semaphore/mutex tối đa có thể đăng ký tên. Project có 3 queues cần đăng ký → cần ≥ 3. Hiện tại set = 8 trong FreeRTOSConfig.h — đủ dùng.

---

### Q4: Tại sao `SEGGER_SYSVIEW_Start()` phải gọi trước `vTaskStartScheduler()`?

**Trả lời:** Để capture event từ t=0 (scheduler start, task creation, first context switch). Nếu gọi sau `vTaskStartScheduler()`, code đó không bao giờ chạy vì scheduler không return. Nếu gọi từ trong task thì miss toàn bộ events từ đầu.

---

### Q5: Timer Service Task có hiện trên SystemView không? Dùng để làm gì?

**Trả lời:** Có. Hiện tên `"Tmr Svc"`. Trên timeline sẽ thấy nó chạy rất ngắn mỗi khi timer fire — đây là cách verify timer callback hoạt động đúng thời điểm và không chiếm CPU quá lâu.

---

### Q6: `SEGGER_SYSVIEW_Conf()` khác gì `SEGGER_SYSVIEW_Start()`?

**Trả lời:** `Conf()` khởi tạo một lần — setup RTT buffer, đăng ký interrupt names, thông tin MCU. `Start()` bắt đầu record events — có thể gọi/stop nhiều lần để capture khoảng thời gian cụ thể.

---

## Session 05 — 2026-06-26 — Phase 7: Software Timers

**Chủ đề:** vTaskDelay vs event-driven, Timer Service Task, thiết kế timer, one-shot vs auto-reload.

---

### Q1: Vấn đề gì xảy ra khi WashingManager đang `vTaskDelay(20000ms)` mà cửa máy giặt bị mở?

**Trả lời của bạn:** EVT_ERROR được ưu tiên hơn nên WashingManager sẽ thoát khỏi block.

**Đánh giá:** Sai.

**Giải thích:** `vTaskDelay()` không quan tâm đến event group — chỉ ngủ đủ ms rồi thức. WashingManager không hay biết EVT_ERROR trong suốt 20 giây, motor tiếp tục quay dù cửa đã mở. Đây là lỗi safety nghiêm trọng. Fix: dùng `xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)` — thức ngay khi có bất kỳ signal nào đến trước.

---

### Q2: Software timer callback chạy trong context của task nào? Tại sao không được dùng blocking API?

**Trả lời của bạn:** Chạy trong SensorTask.

**Đánh giá:** Sai.

**Giải thích:** Callback chạy trong **Timer Service Task** — task đặc biệt do FreeRTOS tự tạo khi enable software timers. Task này phục vụ tất cả timers trong hệ thống. Nếu một callback block (timeout > 0), Timer Service Task bị treo — mọi timer khác đều không được gọi callback. Phải dùng `FromISR` variants: `xEventGroupSetBitsFromISR()`, `xQueueSendFromISR()`.

---

### Q3: Nên tạo 4 timer riêng hay 1 timer dùng lại?

**Trả lời của bạn:** 4 timer riêng, nhưng tốn tài nguyên hơn.

**Đánh giá:** Đúng lựa chọn, nhận xét đúng về trade-off.

**Giải thích:** 4 timer tốn thêm ~150 bytes RAM nhưng code rõ ràng, dễ debug trên SystemView, không có risk nhầm timer. Với 88KB RAM còn trống, 150 bytes không đáng kể. Nguyên tắc: chỉ tối ưu khi đo được vấn đề.

---

### Q4: `xTimerStart()` cho wash timer nên được gọi ở đâu và bởi ai?

**Trả lời của bạn:** Gọi khi bắt đầu quá trình washing bởi WashingManager task.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích:** Gọi trong `Manager_OnEnterWashing()` — entry action callback của SM. Ngay khi vào state WASHING, motor bật và timer bắt đầu đếm cùng lúc. Fill-timeout timer cần được stop thủ công nếu nước đầy trước khi timer fire.

---

### Q5: Làm sao WashingManager biết `EVT_TIMEOUT` đến từ timer nào?

**Trả lời của bạn:** Kiểm tra xem SM đang ở state nào.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích:** Mỗi state chỉ có đúng một timer đang chạy — không cần phân biệt timer ID. EVT_TIMEOUT trong WASHING chỉ có thể từ wash timer, trong DRAIN chỉ từ drain timer. SM context thay thế hoàn toàn cho việc phân biệt.

---

### Q6: `uxAutoReload` nên là `pdTRUE` hay `pdFALSE`?

**Trả lời của bạn:** Chỉ fire 1 lần cho mỗi lần giặt.

**Đánh giá:** Đúng — `pdFALSE` (one-shot).

**Giải thích:** Mỗi giai đoạn chỉ cần một tín hiệu "hết giờ" duy nhất. Auto-reload sẽ fire liên tục, gây EVT_TIMEOUT thừa ở các state sau. One-shot là lựa chọn tự nhiên cho mọi deadline hay timeout.

---

## Case Study 01 — 2026-06-25 — Multiple Definition: SVC_Handler / PendSV_Handler / SysTick_Handler

---

### Triệu chứng

Build thất bại với lỗi linker:

```
multiple definition of `SVC_Handler'
multiple definition of `PendSV_Handler'
multiple definition of `SysTick_Handler'
```

---

### Nguyên nhân gốc rễ

Dự án tích hợp FreeRTOS thủ công (không qua CubeMX RTOS middleware).
Khi CubeMX tái tạo code (sau khi thêm I2C1), nó sinh lại `stm32f4xx_it.c`
với đầy đủ các interrupt handler tiêu chuẩn, bao gồm:

- `SVC_Handler`
- `PendSV_Handler`
- `SysTick_Handler`

Đồng thời, `FreeRTOSConfig.h` đang có các macro:

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

Các macro này khiến `port.c` của FreeRTOS compile ra các hàm trùng tên.
Kết quả: linker thấy hai định nghĩa cho cùng một symbol → lỗi.

---

### Tại sao xảy ra mỗi lần CubeMX tái tạo?

CubeMX không biết FreeRTOS đã được tích hợp thủ công. Nó luôn sinh ra
`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler` trong `stm32f4xx_it.c`
vì đây là các handler tiêu chuẩn của Cortex-M4. Đây là conflict cấu trúc,
không phải lỗi code.

---

### Giải pháp (permanent — sống sót qua CubeMX tái tạo)

**Bước 1 — Đổi tên FreeRTOS handlers trong `FreeRTOSConfig.h`:**

```c
// Trước
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

// Sau
#define vPortSVCHandler     vFreeRTOS_SVC_Handler
#define xPortPendSVHandler  vFreeRTOS_PendSV_Handler
#define xPortSysTickHandler vFreeRTOS_SysTick_Handler
```

`port.c` giờ compile ra các hàm có tên khác — không còn conflict.

**Bước 2 — Forward calls trong `stm32f4xx_it.c` (trong USER CODE sections):**

```c
/* USER CODE BEGIN EV */
extern void vFreeRTOS_SVC_Handler(void);
extern void vFreeRTOS_PendSV_Handler(void);
extern void vFreeRTOS_SysTick_Handler(void);
/* USER CODE END EV */
```

```c
void SVC_Handler(void) {
  /* USER CODE BEGIN SVCall_IRQn 0 */
  vFreeRTOS_SVC_Handler();
  /* USER CODE END SVCall_IRQn 0 */
}

void PendSV_Handler(void) {
  /* USER CODE BEGIN PendSV_IRQn 0 */
  vFreeRTOS_PendSV_Handler();
  /* USER CODE END PendSV_IRQn 0 */
}

void SysTick_Handler(void) {
  /* USER CODE BEGIN SysTick_IRQn 0 */
  vFreeRTOS_SysTick_Handler();
  /* USER CODE END SysTick_IRQn 0 */
}
```

---

### Tại sao giải pháp này là permanent?

CubeMX tái tạo `stm32f4xx_it.c` nhưng **chỉ xóa code ngoài USER CODE sections**.
Các lệnh `extern` và lời gọi `vFreeRTOS_*` nằm trong USER CODE sections →
được giữ nguyên sau mỗi lần regenerate.

---

### Phân tích luồng sau khi fix

```
Cortex-M4 hardware trigger
        │
        ▼
SVC_Handler()          ← định nghĩa bởi stm32f4xx_it.c (CubeMX)
        │
        ▼
vFreeRTOS_SVC_Handler()  ← định nghĩa bởi port.c (FreeRTOS)
        │
        ▼
FreeRTOS scheduler / context switch logic
```

---

### Bài học

- Khi tích hợp FreeRTOS thủ công với CubeMX, luôn phải giải quyết conflict
  interrupt handler này.
- Giải pháp đúng không phải là xóa handler trong `stm32f4xx_it.c` (sẽ mất
  sau mỗi lần regenerate), mà là đổi tên FreeRTOS symbols và forward từ
  USER CODE sections.
- USER CODE sections là ranh giới an toàn duy nhất giữa code của bạn và
  code do CubeMX quản lý.

---

## Session 04 — 2026-06-25 — Phase 6: OLED Integration

**Chủ đề:** OLEDTask priority, display queue timeout, Blocked vs Idle, preemption, self-contained message, BSP layer.

---

### Q1: Tại sao OLEDTask có priority thấp nhất (1)?

**Trả lời của bạn:** Màn hình chỉ cần update từng thời điểm, không yêu cầu nghiêm ngặt về mặt thời gian đến mức ms hay us.

**Đánh giá:** Đúng.

**Giải thích thêm:** OLEDTask không được phép block WashingManager. Nếu OLEDTask có priority cao hơn, WashingManager có thể bị preempt giữa chừng khi đang điều khiển motor và valve — nguy hiểm. Priority thấp đảm bảo OLEDTask chỉ chạy khi mọi task quan trọng hơn đã Blocked.

---

### Q2: Tại sao WashingManager gửi display queue với timeout 0, còn UITask gửi command queue không dùng timeout 0?

**Trả lời của bạn:** Không biết.

**Giải thích:**
- **WashingManager → display queue (timeout 0):** WashingManager là task priority cao nhất. Nếu block chờ OLEDTask, toàn hệ thống đứng lại. Display message bị drop không phải thảm họa — lần transition tiếp theo gửi message mới.
- **UITask → command queue (có timeout):** Command mang ý định người dùng. Nếu bị drop, người dùng nhấn nút nhưng máy không phản hồi — lỗi chức năng nghiêm trọng.

Nguyên tắc: producer priority cao hơn consumer → timeout 0. Producer priority thấp hơn consumer → có thể block ngắn.

---

### Q3: Khi queue rỗng và OLEDTask đang chờ, nó ở trạng thái nào? Có tiêu thụ CPU không?

**Trả lời của bạn:** Trạng thái idle, không tiêu thụ CPU.

**Đánh giá:** Đúng về CPU, sai về tên trạng thái.

**Giải thích:** OLEDTask ở trạng thái **Blocked**, không phải Idle. Idle là trạng thái của Idle Task — task đặc biệt FreeRTOS tạo ra, chạy khi không có task nào khác Ready. Task thông thường khi chờ tín hiệu → Blocked. Blocked không tiêu thụ CPU.

---

### Q4: Trong khi OLEDTask đang ghi I2C (mất vài ms), nó có thể bị preempt không?

**Trả lời của bạn:** Bị preempt.

**Đánh giá:** Đúng.

**Giải thích:** FreeRTOS dùng preemptive scheduling — SysTick mỗi 1ms kiểm tra có task priority cao hơn đang Ready không. Nếu có, OLEDTask bị preempt ngay giữa I2C write. Đây là lý do OLEDTask phải là task duy nhất truy cập OLED — tránh corrupt data.

---

### Q5: Tại sao `DisplayMsg_t` chứa toàn bộ thông tin (self-contained) thay vì chỉ gửi phần thay đổi (partial update)?

**Trả lời của bạn:** Struct đầy đủ thì display dễ hơn.

**Đánh giá:** Đúng hướng.

**Giải thích đầy đủ:** Nếu dùng partial update, OLEDTask phải tự lưu state — giữ bản copy màn hình hiện tại, merge từng field. OLEDTask trở thành stateful, vi phạm nguyên tắc "pure renderer". Với self-contained message: nhận → render ngay → không cần nhớ gì. `DisplayMsg_t` chỉ 32 bytes — chi phí không đáng kể trên 128KB RAM.

Nguyên tắc: mỗi message phải tự đủ nghĩa — người nhận không cần context bên ngoài.

---

## Session 03 — 2026-06-25 — Phase 4: SensorTask

**Chủ đề:** vTaskDelay vs HAL_Delay, priority preemption, polling vs interrupt.

---

### Q1: Tại sao dùng `vTaskDelay()` thay vì `HAL_Delay()` cho chu kỳ 100ms?

**Trả lời của bạn:** HAL_Delay do CPU kiểm soát nên tốn tài nguyên CPU; vTaskDelay do RTOS kiểm soát.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
- `HAL_Delay()` là busy-wait — CPU chạy vòng lặp rỗng, không nhường thời gian cho task khác.
- `vTaskDelay()` chuyển task sang trạng thái Blocked — CPU rảnh để chạy task khác. Sau đúng 100ms scheduler đưa task về Ready. Đây là cách đúng trong RTOS.

---

### Q2: Sau khi SensorTask set `EVT_WATER_FULL`, WashingManager có chạy ngay không?

**Trả lời của bạn:** Phải chờ cả event của door nữa.

**Đánh giá:** Chưa chính xác.

**Giải thích:**
Trong FILL_WATER state, WashingManager chỉ chờ `EVT_WATER_FULL | EVT_TIMEOUT` — không chờ door.

Điều quan trọng hơn là **priority preemption**:
- SensorTask: priority 2
- WashingManager: priority 3 (cao hơn)

Khi SensorTask gọi `xEventGroupSetBits(EVT_WATER_FULL)`, WashingManager unblock. Vì priority cao hơn, scheduler preempt SensorTask **ngay bên trong** lời gọi `xEventGroupSetBits()` — trước khi SensorTask kịp chạy thêm dòng nào. Priority quyết định ai chạy, không phải ai set bit trước.

---

### Q3: Tại sao SensorTask dùng polling thay vì interrupt?

**Trả lời của bạn:** Trong thực tế nên dùng ngắt để notify; đây là mô phỏng và không có cảm biến thật nên dùng tạm poll.

**Đánh giá:** Đúng hoàn toàn.

**Tổng kết:**
- **Interrupt:** tín hiệu thay đổi bất kỳ lúc nào, cần phản ứng ngay (vài micro giây).
- **Polling:** không có phần cứng thật, tín hiệu thay đổi chậm, latency 100ms chấp nhận được.

SensorTask trong dự án này là stub. Khi có phần cứng thật, thay bằng ISR + `xEventGroupSetBitsFromISR()` — phần còn lại không đổi.

---

## Session 02 — 2026-06-25 — Phase 3: BSP Button + UITask

**Chủ đề:** Tại sao tách ISR khỏi UITask, tại sao tách BSP khỏi application layer, debounce.

---

### Q1: Tại sao ISR không xử lý menu logic trực tiếp mà phải post event vào `xButtonQueue`?

**Trả lời của bạn:** Vì khi người dùng bấm nhiều lần context rất dễ bị miss nếu xử lý menu trực tiếp.

**Đánh giá:** Đúng — đây là lý do chính.

**Giải thích đầy đủ:**
1. **Queue có depth — buffering nhiều lần nhấn:** Queue tích lũy event, UITask xử lý khi được schedule. Không mất event dù nhấn nhanh.
2. **ISR phải chạy cực ngắn:** ISR chạy ở elevated priority, block interrupt khác. Menu logic trong ISR làm tăng latency toàn hệ thống.
3. **FreeRTOS API trong ISR bị giới hạn:** Không được gọi `xQueueSend()` từ ISR — phải dùng `xQueueSendFromISR()`. Không được gọi blocking API (vTaskDelay, xQueueReceive). Tách ISR và UITask giúp ranh giới này rõ ràng.

---

### Q2: Tại sao không để UITask đọc GPIO trực tiếp thay vì đi qua BSP?

**Trả lời của bạn:** Tách layer giúp làm việc rõ ràng hơn; nhiều task có thể dùng BSP nên phải tách ra.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
- **Portability:** Nếu đổi board, GPIO pin thay đổi, chỉ sửa `bsp_button.c` — UITask không đụng đến.
- **Rule trong dự án:** *"BSP hides HAL from application code"* — UITask không được biết PA0 là pin nào, chỉ biết "có button event".

---

### Q3: Debounce là gì? Tại sao cần debounce cho nút nhấn cơ học?

**Trả lời của bạn:** Thuật ngữ mới, đoán liên quan đến nhiễu.

**Đánh giá:** Đúng hướng.

**Giải thích:**
Nút nhấn cơ học có tiếp điểm kim loại — khi nhấn, tiếp điểm **nảy (bounce)** nhiều lần trong ~50ms trước khi ổn định. MCU chạy ở MHz, thấy mỗi lần nảy là một lần nhấn riêng. Người dùng nhấn 1 lần nhưng EXTI có thể trigger 5–10 lần.

```
PA0:  ____    _ __________
          |  | |
__________|  |_|
          ^bounce^ (~50ms)
```

**Giải pháp:** Sau EXTI đầu tiên, bỏ qua mọi trigger trong vòng `BUTTON_DEBOUNCE_MS` (50ms). Chỉ lần trigger đầu tiên được post vào `xButtonQueue`.

---

## Session 01 — 2026-06-25 — Phase 5: Wiring main.c

**Chủ đề:** Thứ tự khởi tạo FreeRTOS, stack size, stack overflow, test không có UITask.

---

### Q1: Tại sao `App_FreeRTOSObjectsInit()` phải gọi trước `vTaskStartScheduler()`? Điều gì xảy ra nếu task được tạo trước khi queue tồn tại?

**Trả lời của bạn:** Không biết.

**Giải thích:**
Khi `WashingManagerTask` chạy, nó gọi `xQueueReceive(xCommandQueue, ...)`.
Handle `xCommandQueue` là một con trỏ tới queue object trong RAM.
Nếu `App_FreeRTOSObjectsInit()` chưa chạy, handle đó vẫn là `NULL`.
Truyền `NULL` vào bất kỳ FreeRTOS API nào là undefined behavior — crash hoặc corrupt memory.

**Quy tắc đúng:**
```
Tạo RTOS objects → Tạo tasks → vTaskStartScheduler()
```
Task được tạo ở trạng thái suspended — chúng không chạy cho đến khi scheduler bắt đầu.
Vì vậy, chỉ cần tạo queue/event group trước `vTaskStartScheduler()` là đủ.

---

### Q2: Tại sao không thể gọi `App_FreeRTOSObjectsInit()` sau `vTaskStartScheduler()`?

**Trả lời của bạn:** Vì `vTaskStartScheduler()` không bao giờ trả về.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
Khi `vTaskStartScheduler()` được gọi, FreeRTOS:
1. Tạo Idle Task và Timer Task nội bộ.
2. Cấu hình SysTick interrupt.
3. Chuyển quyền điều khiển sang task có priority cao nhất ở trạng thái Ready.

CPU không bao giờ quay lại dòng code sau `vTaskStartScheduler()`.
Trường hợp duy nhất nó trả về là khi không đủ RAM để tạo Idle Task — và lúc đó hệ thống crash ngay.

---

### Q3: Stack size 256 words trên STM32F411 = bao nhiêu bytes?

**Trả lời của bạn:** 4 × 256 = 1024 bytes.

**Đánh giá:** Đúng.

**Giải thích thêm:**
FreeRTOS dùng đơn vị "words" để portable:
- Kiến trúc 8-bit: 1 word = 1 byte
- Kiến trúc 32-bit (STM32): 1 word = 4 bytes

Cùng con số `256` nhưng ý nghĩa thực tế khác nhau tùy nền tảng.
`xTaskCreate(..., 256, ...)` trên STM32 = cấp 1 KB stack cho task đó.

---

### Q4: Tại sao stack overflow gây hard fault thay vì lỗi rõ ràng hơn? Điều gì xảy ra ở mức phần cứng?

**Trả lời của bạn:**
- Stack lưu các execution frame của task (đúng).
- Nếu stack không đủ thì đã fail ở bước compile (sai).
- Khi chạy gây stack overflow thì crash ngay (gần đúng).

**Điểm cần sửa — compile-time detection là sai:**
Compiler không thể biết call stack sẽ sâu bao nhiêu lúc runtime.
`xTaskCreate(..., 256, ...)` chỉ là con số nguyên — compiler không phân tích được
toàn bộ call chain sẽ dùng bao nhiêu stack.
Đây là vấn đề **runtime**, không phải compile-time.

**Điều thực sự xảy ra lúc runtime:**
Khi stack tràn, task ghi đè vào vùng RAM của task khác hoặc kernel.
Không có cơ chế bảo vệ phần cứng nào ngăn điều này (trừ khi bật MPU).
Crash xảy ra **muộn hơn** và **ở chỗ khác** — rất khó debug.

FreeRTOS có `configCHECK_FOR_STACK_OVERFLOW` để phát hiện sớm hơn,
nhưng vẫn là runtime check, không phải compile-time.

---

### Q5: Khi chưa có UITask, làm sao test `WashingManagerTask`?

**Trả lời của bạn:** Truyền trực tiếp command vào queue.

**Đánh giá:** Đúng — đây chính xác là cách embedded engineer kinh nghiệm làm.

**Hai cách phổ biến:**

**Cách 1 — Hardcode tạm trong `main.c`** (đơn giản hơn):
```c
CommandMsg_t cmd = { .command = CMD_START_WASH };
xQueueSend(xCommandQueue, &cmd, 0);
// Đặt sau App_FreeRTOSObjectsInit(), trước vTaskStartScheduler()
```

**Cách 2 — Debugger Expression** (không cần recompile):
Pause debugger, gọi trong Expressions window:
```c
xQueueSend(xCommandQueue, &cmd, 0)
```
