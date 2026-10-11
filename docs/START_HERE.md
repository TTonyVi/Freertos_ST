# START HERE — Học FreeRTOS qua máy giặt mô phỏng

**Dành cho:** người biết C, đã từng viết STM32 kiểu `HAL_GPIO_WritePin()` +
`HAL_Delay()` trong `while(1)`, **chưa từng dùng RTOS**.

**Sau khi đọc xong bạn sẽ:** đọc hiểu toàn bộ ~2300 dòng code trong [`App/`](../App/),
giải thích được vì sao mỗi dòng nằm ở đó, và tự sửa/mở rộng được hệ thống.

**Bạn cần có:** board STM32F411E-DISCO, cáp USB, STM32CubeIDE. Mọi thí nghiệm
trong tài liệu này đều chạy trên board thật.

---

## §0 Cách dùng tài liệu này

### §0.1 Vị trí của file này trong bộ tài liệu

| File | Là gì | Đọc khi nào |
| --- | --- | --- |
| **START_HERE.md** (file này) | **Đường đi** — dẫn bạn từ số 0 tới hiểu toàn bộ project | **Bắt đầu ở đây.** Đọc tuần tự |
| [LEARNING_NOTES.md](LEARNING_NOTES.md) | **Cẩm nang tra cứu** — kiến thức sâu từng primitive, đối chiếu source kernel | Khi tầng nào đó nói "đào sâu →" |
| [SYSTEM_DIAGRAMS.md](SYSTEM_DIAGRAMS.md) | 11 sơ đồ hệ thống | Mở song song từ Tầng 2 |
| [PROJECT_SPEC.md](../PROJECT_SPEC.md) | Đặc tả yêu cầu | Khi cần biết "hệ thống *phải* làm gì" |
| [GPIO_CONFIG.md](GPIO_CONFIG.md) | Cấu hình từng chân, cách đấu dây | Khi lắp mạch ở Tầng 0 |
| [HARDWARE_BUGS.md](HARDWARE_BUGS.md) | Nhật ký các bug phần cứng thật đã gặp | Khi board không chạy như mong đợi |
| [LEARNING_NOTES.md](LEARNING_NOTES.md) Phần D | Hỏi–đáp trong quá trình xây dựng | Tùy chọn |

Quy tắc: **một sự thật chỉ nằm ở một chỗ.** File này cố tình *nông* — hết phần
cần thiết để hiểu code là dừng, phần sâu trỏ sang LEARNING_NOTES.

> **Nên đọc bản HTML:** [START_HERE.html](START_HERE.html) có mục lục bên trái và
> chế độ sáng/tối. [SYSTEM_DIAGRAMS.html](SYSTEM_DIAGRAMS.html) cho phép **zoom,
> kéo, và xem toàn màn hình** từng sơ đồ — cần khi mở sơ đồ kiến trúc tổng thể
> (rộng gần 2800 px, chữ nhỏ nếu chỉ nhìn bản thu nhỏ). Các file `.html` được sinh
> từ `.md` bằng `cd tools && npm run build`; đừng sửa tay.

### §0.2 Khuôn của mỗi tầng

Mọi tầng đều theo đúng 7 mục này:

| Mục | Nội dung |
| --- | --- |
| **1. Hiện tượng** | Một điều quan sát được trên board mà bạn chưa giải thích được |
| **2. Cách ngây thơ** | Cách bạn sẽ làm nếu chỉ biết superloop — và nó hỏng ở đâu |
| **3. Ý tưởng** | Giải pháp của FreeRTOS, một sơ đồ, tối đa một đoạn văn |
| **4. Code thật** | Trích từ repo, kèm `file:dòng` — không có ví dụ bịa |
| **5. Thí nghiệm** | **Dự đoán trước → nạp → đo → giải thích chênh lệch** |
| **6. Tự kiểm tra** | 3 câu — **đáp án và giải thích nằm ngay dưới mỗi câu** |
| **7. Đào sâu** | Trỏ sang mục tương ứng trong LEARNING_NOTES |

**Mục 5 là mục quan trọng nhất.** Đọc mà không đo thì chỉ là thuộc lòng. Bắt
buộc viết dự đoán ra giấy *trước khi* nạp code — chỗ dự đoán sai chính là chỗ
mô hình tư duy của bạn đang sai, và đó là thứ duy nhất đáng học.

### §0.3 Lộ trình

| Tầng | Tên | Câu hỏi trung tâm | Trạng thái |
| --- | --- | --- | --- |
| **0** | Sờ vào hệ thống | Cái máy này làm gì? | ✅ |
| **1** | Vì sao cần RTOS | Ba việc cùng lúc — superloop hỏng ở đâu? | ✅ |
| **2** | Bốn task của project | Ai làm gì, ai quan trọng hơn ai? | ✅ |
| **3** | Task nói chuyện với nhau | Dữ liệu đi từ task này sang task kia bằng gì? | ✅ |
| **4** | Chờ nhiều điều kiện | Chờ "cửa đóng VÀ nước đầy" kiểu gì? | 🚧 |
| **5** | Trái tim: State Machine | Máy giặt "đang ở đâu" được lưu ở chỗ nào? | 🚧 |
| **6** | Từ chân MCU đến task | Nút bấm đi qua những bước nào? | 🚧 |

Mỗi tầng ~30–60 phút. **Không nhảy cóc** — tầng sau dùng thuật ngữ tầng trước
đã định nghĩa. Hết Tầng 6 là đã đọc hiểu được toàn bộ code trong [`App/`](../App/).

> **Để sau:** phần công cụ debug (SEGGER SystemView, RTT, logic analyzer) **chưa
> nằm trong lộ trình này**. Bảy tầng trên chỉ cần board, LED và nút bấm — không
> cần công cụ trace nào. Khi nào cần dựng thêm một tầng về đọc trace thì bổ sung
> sau, đánh số tiếp là Tầng 7.

---
---

# Tầng 0 — Sờ vào hệ thống

> **Mục tiêu:** trước khi đọc một dòng code nào, bạn phải *dùng* được cái máy.
> Không ai học được kiến trúc của thứ mình chưa từng thấy chạy.

## 0.1 Hiện tượng

Nạp firmware. Bạn có một cái bảng mạch với 4 đèn LED và 4 cái nút. Bấm vài
cái, đèn nhấp nháy. **Bạn chưa biết gì cả** — và đó là điểm xuất phát đúng.

Nhiệm vụ của tầng này: biến đống đèn nhấp nháy đó thành **một cái máy giặt bạn
điều khiển được**.

## 0.2 Cách ngây thơ

Cách ngây thơ ở đây là: *mở code ra đọc trước.* Đừng.

Mở [`washing_manager.c`](../App/WashingManager/Src/washing_manager.c) (359 dòng)
lúc chưa biết máy giặt vận hành thế nào thì bạn đang cố hiểu **giải pháp** của
một **bài toán bạn chưa biết**. Đây là sai lầm phổ biến nhất khi tiếp nhận một
codebase lạ.

Thứ tự đúng: **hiện tượng → bài toán → giải pháp.**

## 0.3 Ý tưởng — máy giặt này là gì

Project mô phỏng một máy giặt. Không có nước, không có motor thật. Mọi thứ vật
lý được thay bằng **đèn LED** (đầu ra) và **nút bấm** (đầu vào cảm biến).

### Bản đồ phần cứng

**4 đầu ra — LED có sẵn trên board:**

| Chân | LED trên board | Đóng vai trò | Sáng nghĩa là |
| --- | --- | --- | --- |
| PD12 | LD4 — **xanh lá** | Motor | Lồng giặt đang quay |
| PD13 | LD3 — **cam** | Van cấp nước | Đang bơm nước vào |
| PD14 | LD5 — **đỏ** | Van xả | Đang xả nước ra |
| PD15 | LD6 — **xanh dương** | Đèn báo trạng thái | Máy đang bận |

Đối chiếu code: [bsp_motor.c:40](../App/BSP/Src/bsp_motor.c#L40) (PD12),
[bsp_valve.c:10-11](../App/BSP/Src/bsp_valve.c#L10-L11) (PD13/PD14),
[bsp_led.c:10](../App/BSP/Src/bsp_led.c#L10) (PD15).

> **"BSP" là gì?** Viết tắt của **Board Support Package** — phần code gắn với một
> board cụ thể: chân nào nối với thiết bị nào, bật/tắt thiết bị đó ra sao. Ba file
> trên (`bsp_motor.c`, `bsp_valve.c`, `bsp_led.c`) nằm trong thư mục
> [`App/BSP/`](../App/BSP/). Nghĩa gốc của BSP rộng hơn (cả clock, bộ nhớ, bootloader
> do nhà sản xuất board cung cấp); project này dùng chữ đó cho **lớp code che phần
> cứng khỏi logic máy giặt**. Tầng 2 giải thích vì sao phải có lớp này.

**4 đầu vào — nút bấm:**

| Chân | Nút | Bấm vào thì | Ghi chú |
| --- | --- | --- | --- |
| PA0 | B1 (có sẵn trên board) | Di chuyển con trỏ menu | Nút xanh dương to |
| PD2 | Nút Start (tự đấu) | Xác nhận / bắt đầu chu trình | Cần lắp thêm |
| PD5 | Nút cảm biến cửa (tự đấu) | Bật/tắt "cửa đã đóng" | Mặc định: cửa đóng |
| PD6 | Nút cảm biến nước (tự đấu) | Bật/tắt "nước đã đầy" | Mặc định: chưa đầy |

Ba nút PD2/PD5/PD6 phải tự đấu ra breadboard — sơ đồ đấu dây ở
[GPIO_CONFIG.md](GPIO_CONFIG.md).

> **Vì sao dùng nút để giả lập cảm biến?** Vì cảm biến thật (công tắc cửa, phao
> nước) chưa có. Dùng nút thì bạn tự tạo được mọi tình huống — kể cả tình huống
> hỏng, như "cửa bị mở giữa lúc đang giặt" — mà không cần debugger. Đây là kỹ
> thuật thật trong nghề: **tách phần logic khỏi phần cảm biến để test được sớm**,
> vì phần cứng thường về muộn hơn phần mềm.

### Máy có 7 trạng thái

Từ [app_types.h:34-40](../App/Common/Inc/app_types.h#L34-L40):

| Trạng thái | Nghĩa | Đèn sáng |
| --- | --- | --- |
| `IDLE` | Đứng yên, chờ người dùng | (không) |
| `FILL_WATER` | Đang bơm nước vào | 🟠 cam + 🔵 xanh dương |
| `WASHING` | Đang giặt, lồng quay | 🟢 xanh lá + 🔵 |
| `DRAIN` | Đang xả nước | 🔴 đỏ + 🔵 |
| `SPIN` | Đang vắt | 🟢 + 🔵 |
| `FINISH` | Xong, chờ xác nhận | 🔵 báo hiệu |
| `ERROR` | Có lỗi (ví dụ: cửa mở) | 🔵 báo lỗi |

### Thời lượng từng giai đoạn

Từ [app_config.h:19-28](../App/Common/Inc/app_config.h#L19-L28):

| Giai đoạn | Hằng số | Thời gian |
| --- | --- | --- |
| Bơm nước (`FILL_WATER`) | `WASH_FILL_DURATION_MS` | **Không phải thời gian bơm.** Van cấp mở **đến khi cảm biến báo đầy** (PD6); 10 giây chỉ là *hạn chót an toàn* — hết giờ mà chưa đầy → `ERROR` |
| Giặt (`WASHING`) | `WASH_CYCLE_DURATION_MS` | 20 giây |
| Xả (`DRAIN`) | `WASH_DRAIN_DURATION_MS` | 5 giây |
| Vắt (`SPIN`) | `SPIN_DURATION_MS` | 10 giây |

Chu trình đầy đủ: `IDLE → FILL_WATER → WASHING → DRAIN → SPIN → FINISH`.
`WASHING` + `DRAIN` + `SPIN` cộng lại 35 giây; `FILL_WATER` dài bao lâu tuỳ lúc bạn
bấm PD6 (cảm biến báo đầy), tối đa 10 giây.

## 0.4 Code thật — hệ thống khởi động thế nào

Chỉ cần đọc **một** đoạn ở tầng này, [Core/Src/main.c:85-125](../Core/Src/main.c#L85-L125):

```c
HAL_Init();                     /* 85  — khởi tạo HAL, flash, tick     */
SystemClock_Config();           /* 92  — chạy CPU ở 16 MHz (HSI nội)   */

SEGGER_SYSVIEW_Conf();          /* 99  — công cụ trace, CHƯA DÙNG TỚI  */
SEGGER_SYSVIEW_Start();         /* 100   → bỏ qua hai dòng này          */

MX_GPIO_Init();                 /* 104 — cấu hình chân LED và nút      */
MX_I2C1_Init();                 /* 105 — I2C cho màn OLED              */
BSP_Sensor_Init();              /* 110 — cấu hình PD5/PD6              */

App_FreeRTOSObjectsInit();      /* 112 — tạo hộp thư liên lạc (Tầng 3) */

xTaskCreate(WashingManager_Task, "WashMgr", 256, NULL, 3, NULL);  /* 114 */
xTaskCreate(UITask,              "UITask",  128, NULL, 2, NULL);  /* 116 */
xTaskCreate(SensorTask,          "Sensor",  128, NULL, 2, NULL);  /* 117 */
xTaskCreate(OLEDTask,            "OLED",    256, NULL, 1, NULL);  /* 118 */

vTaskStartScheduler();          /* 120 — bàn giao quyền cho FreeRTOS   */

while (1) { }                   /* 125 — KHÔNG BAO GIỜ CHẠY TỚI ĐÂY    */
```

Ba điều cần nhận ra ngay, chưa cần hiểu sâu:

1. **Phần đầu giống hệt code STM32 bạn từng viết.** HAL init, clock, GPIO —
   không có gì mới. RTOS **không** thay thế HAL.
2. **Có bốn `xTaskCreate()`.** Đây là bốn "chương trình con chạy song song".
   Tầng 2 sẽ mổ xẻ.
3. **`while(1)` cuối cùng là code chết.** `vTaskStartScheduler()` không bao giờ
   return. Đây là khác biệt lớn nhất so với bare-metal: sau dòng 120, **bạn
   không còn điều khiển luồng chương trình nữa** — FreeRTOS điều khiển.

## 0.5 Thí nghiệm — chạy một chu trình đầy đủ

**Chuẩn bị:** đấu 3 nút PD2/PD5/PD6 theo [GPIO_CONFIG.md](GPIO_CONFIG.md), build
và nạp:

```
cd Debug
make all
```

**✍ Viết dự đoán ra giấy trước khi bấm:**

> Nếu tôi bấm Start ngay bây giờ (cửa đang đóng, nước chưa đầy), trong 10 giây
> tiếp theo đèn nào sáng? Nếu tôi **không** bấm nút nước PD6, sau 10 giây điều
> gì xảy ra?

**Kịch bản A — chu trình bình thường:**

| # | Thao tác | Quan sát |
| --- | --- | --- |
| 1 | Reset board | Không đèn nào sáng. OLED hiện menu, con trỏ ở `> Wash` |
| 2 | Bấm **PD2** (Start) | 🟠 cam + 🔵 xanh dương sáng → đang bơm nước |
| 3 | Trong 10 s, bấm **PD6** (nước đầy) | 🟠 tắt, 🟢 xanh lá sáng → chuyển sang giặt |
| 4 | Chờ 20 s | 🟢 tắt, 🔴 đỏ sáng → đang xả |
| 5 | Chờ 5 s | 🔴 tắt, 🟢 sáng lại → đang vắt |
| 6 | Chờ 10 s | Vào `FINISH` |
| 7 | Bấm **PD2** | Về `IDLE`, tắt hết |

**Kịch bản B — cố tình gây lỗi:** lặp lại bước 1–2, rồi **không bấm PD6**. Sau
10 giây: máy vào `ERROR`. Đây là "hết giờ bơm nước mà nước không lên" — tình
huống hỏng thật của máy giặt thật.

**Kịch bản C — mở cửa giữa chừng:** chạy tới `WASHING`, rồi bấm **PD5** (mở
cửa). Máy phải dừng ngay và vào `ERROR`.

**Nếu kết quả khác dự đoán:** đó là dữ liệu tốt. Ghi lại. Tầng 5 sẽ giải thích
chính xác vì sao. Chi tiết từng bước của 4 kịch bản có ở
[SYSTEM_DIAGRAMS.md §3–§6](SYSTEM_DIAGRAMS.md).

**Nếu board không chạy đúng:** đọc [HARDWARE_BUGS.md](HARDWARE_BUGS.md) trước —
các bug phần cứng thật đã gặp đều được ghi lại ở đó, khả năng cao bạn đang gặp
lại một trong số đó.

## 0.6 Tự kiểm tra

Tự trả lời trước, rồi đọc phần giải thích ngay bên dưới.

---

**Câu 1. Vì sao dùng LED thay cho motor thật lại là quyết định *tốt* về mặt kỹ
thuật, chứ không chỉ là "vì không có motor"?**

**Trả lời:** vì nó **tách logic khỏi phần cứng** — và đó là một nguyên tắc thiết
kế, không phải một sự chống chế.

Ba lợi ích cụ thể:

1. **Không phải chờ phần cứng.** Trong project thật, board mạch công suất
   (relay, triac, driver motor) thường về muộn hơn phần mềm vài tuần. Nếu logic
   máy giặt chỉ chạy được khi có motor thật thì cả đội phần mềm ngồi chơi. Dựng
   một lớp giả lập cho phép viết xong và test xong toàn bộ máy trạng thái trước
   khi phần cứng tồn tại.
2. **Tái tạo được tình huống hỏng.** Muốn test "cửa bị mở giữa lúc đang giặt"
   trên máy thật thì phải... mở cửa máy giặt đang quay. Ở đây chỉ cần bấm PD5.
   Tình huống nguy hiểm và tình huống hiếm đều tái tạo được trong một giây, lặp
   lại được vô số lần.
3. **Giao diện không đổi khi thay phần cứng thật.** Nhìn
   [bsp_motor.c:47-55](../App/BSP/Src/bsp_motor.c#L47-L55): tầng ứng dụng chỉ
   gọi `BSP_Motor_Start()` / `BSP_Motor_Stop()`. Ngày nào lắp motor thật, chỉ
   cần sửa **hai hàm trong một file** — bên trong đổi từ bật LED sang xuất PWM.
   Không một dòng nào trong `washing_manager.c` phải sửa.

Điểm 3 chính là lý do phải có thư mục [`App/BSP/`](../App/BSP/) — Tầng 2 sẽ nói kỹ.

---

**Câu 2. Sau `vTaskStartScheduler()` ở dòng 120, ai quyết định đoạn code nào
được chạy?**

**Trả lời: FreeRTOS scheduler**, không còn là bạn nữa.

Đây gọi là **đảo ngược quyền điều khiển** (*inversion of control*), và là khác
biệt lớn nhất giữa bare-metal và RTOS:

| | Bare-metal | RTOS |
| --- | --- | --- |
| Ai gọi ai | `main()` gọi mọi hàm, bạn viết ra thứ tự | Scheduler gọi các task của bạn |
| Thứ tự thực thi | Bạn đọc code là biết | Phụ thuộc priority + sự kiện lúc chạy |
| `main()` sau khi khởi động | Là vòng lặp chính, chạy mãi | **Kết thúc** ở `vTaskStartScheduler()` |

Hệ quả thực tế: `while(1)` ở dòng 125 là **code chết**. Đây là bẫy hay gặp —
người mới đặt thêm lệnh vào đó rồi ngồi thắc mắc vì sao không chạy.
`vTaskStartScheduler()` chỉ return trong đúng một trường hợp: **không đủ heap để
tạo Idle task**. Nếu chương trình của bạn chạy tới `while(1)` đó thì nghĩa là
RTOS đã khởi động thất bại.

---

**Câu 3. Ở kịch bản B, cái gì đếm 10 giây?**

**Trả lời:** một cơ chế đếm giờ **chạy độc lập với luồng logic chính**.

Ở đây là một **software timer** của FreeRTOS, tên `xFillTimeoutTimer`, tạo ở
[freertos_objects.c:152-157](../App/Common/Src/freertos_objects.c#L152-L157) với
chu kỳ `pdMS_TO_TICKS(WASH_FILL_DURATION_MS)` = 10000 ms. Hết giờ, nó gọi
`Timer_PhaseCallback()` để báo "giai đoạn này đã hết thời gian".

Điều đáng chú ý ở câu này **không phải cái tên timer**, mà là vì sao *bắt buộc*
phải có một cơ chế tách rời:

- WashingManager không thể vừa `chờ nước đầy` vừa `tự đếm 10 giây` cùng lúc, nếu
  chỉ có một luồng thực thi.
- Nếu nó ngồi đếm thì không nghe được tin "nước đầy" hay "cửa mở".
- Nếu nó ngồi chờ tin thì không đếm được giờ.

Timer giải quyết đúng mâu thuẫn đó: nó **biến việc hết giờ thành một sự kiện**,
để WashingManager chỉ cần chờ *một loại thứ duy nhất* là sự kiện. Cơ chế đầy đủ
ở Tầng 5.

## 0.7 Đào sâu

- Vì sao clock là 16 MHz HSI chứ không phải thạch anh ngoài → [LEARNING_NOTES §5.4](LEARNING_NOTES.md)
- Toàn bộ sơ đồ hệ thống và 4 kịch bản chi tiết → [SYSTEM_DIAGRAMS.md](SYSTEM_DIAGRAMS.md)

---
---

# Tầng 1 — Vì sao cần RTOS

> **Mục tiêu:** hiểu bài toán mà RTOS sinh ra để giải. Đây là tầng quan trọng
> nhất của tài liệu. Không nắm tầng này thì mọi thứ sau chỉ là học vẹt API.

## 1.1 Hiện tượng

Quay lại Kịch bản A ở Tầng 0. Chạy tới bước 4 — máy **đang giặt**, đèn xanh lá
sáng, đồng hồ đếm 20 giây.

Bây giờ, **trong lúc đang giặt**, mở cửa: bấm nút **PD5**.

Đèn xanh lá (motor) **tắt gần như ngay lập tức** và máy vào `ERROR`. Không phải
chờ hết 20 giây. Vậy mà ngay trước đó, máy chỉ đang lặng lẽ đếm giờ.

**Hai việc đang xảy ra cùng lúc trên một CPU một nhân:** đếm 20 giây giặt, và
canh cánh cửa. Bằng cách nào?

## 1.2 Cách ngây thơ — superloop

Nếu chỉ biết bare-metal, bạn sẽ viết máy giặt thế này:

```c
/* ❌ Cách này KHÔNG dùng được — viết ra để thấy vấn đề */
while (1)
{
    if (start_button_pressed()) {
        open_inlet_valve();
        HAL_Delay(10000);        /* bơm nước 10 giây  */
        close_inlet_valve();

        start_motor();
        HAL_Delay(20000);        /* giặt 20 giây      */
        stop_motor();

        open_drain_valve();
        HAL_Delay(5000);         /* xả 5 giây         */
        close_drain_valve();
    }

    update_oled();
    read_door_sensor();
}
```

Đọc thì rất dễ hiểu. Nhưng **hỏng ở bốn chỗ chí mạng**:

| # | Hỏng ở đâu | Hậu quả cụ thể trên máy giặt |
| --- | --- | --- |
| 1 | `HAL_Delay(20000)` là **busy-wait** — CPU quay vòng rỗng 20 giây | Bấm bất kỳ nút nào lúc đang giặt → **không phản ứng gì trong 20 giây**. Người dùng tưởng máy hỏng |
| 2 | Cảm biến cửa chỉ được đọc **sau khi** delay xong | Mở cửa giữa lúc giặt → lồng vẫn quay tiếp 20 giây rồi máy mới biết. **Nguy hiểm thật sự** |
| 3 | OLED chỉ vẽ lại ở cuối mỗi vòng lặp | Màn hình đứng hình suốt chu trình |
| 4 | Thêm tính năng mới = phải chèn vào đúng chỗ trong vòng lặp | Mỗi lần thêm chức năng lại phải tính lại timing của **toàn bộ** vòng lặp. Đến chức năng thứ 5 thì không ai dám sửa nữa |

Điểm 4 mới là thứ giết chết các project thật. Superloop **chạy tốt** với 2–3
việc. Với 6 việc có ràng buộc thời gian khác nhau, nó thành một khối không sửa
được.

### Cách chữa nửa vời — máy trạng thái phi chặn

Kỹ sư có kinh nghiệm sẽ chữa bằng cách bỏ hết `HAL_Delay`, thay bằng so sánh
timestamp:

```c
/* Đỡ hơn nhiều, nhưng vẫn chưa đủ */
while (1) {
    uint32_t now = HAL_GetTick();
    switch (state) {
        case FILL:  if (now - t_start >= 10000) { state = WASH;  t_start = now; } break;
        case WASH:  if (now - t_start >= 20000) { state = DRAIN; t_start = now; } break;
        /* ... */
    }
    update_oled();
    read_buttons();
}
```

Cách này **chạy được thật** và rất phổ biến trong công nghiệp. Nhưng phải trả giá:

- Mọi việc chậm (vẽ OLED qua I2C mất vài ms) đều **làm trễ toàn bộ phần còn lại**.
- Không có khái niệm "việc này quan trọng hơn việc kia" — mọi thứ bình đẳng
  trong vòng lặp.
- Mọi hàm **buộc phải** viết kiểu phi chặn. Không được phép viết `chờ tới khi X`.
  Điều này ép mọi logic thành máy trạng thái, kể cả logic vốn dĩ tuần tự.
- Khó tái sử dụng: mỗi module phải biết về nhịp của vòng lặp chính.

## 1.3 Ý tưởng — RTOS cho mỗi việc một vòng lặp riêng

FreeRTOS cho phép viết **mỗi việc như thể nó là chương trình duy nhất**, có
`while(1)` riêng, và được phép "chờ" thoải mái:

```
       Superloop                          RTOS
  ┌──────────────────┐        ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐
  │ while(1) {       │        │WashMgr  │ │ UITask  │ │ Sensor  │ │  OLED   │
  │   giặt           │        │while(1) │ │while(1) │ │while(1) │ │while(1) │
  │   đọc nút        │        │ chờ s.k.│ │ chờ nút │ │  đọc    │ │ chờ vẽ  │
  │   vẽ OLED        │        │}        │ │}        │ │  ngủ    │ │}        │
  │   đọc cảm biến   │        └─────────┘ └─────────┘ └─────────┘ └─────────┘
  │ }                │              ▲          ▲          ▲          ▲
  └──────────────────┘              └──────────┴─────┬────┴──────────┘
   Bạn tự sắp thứ tự                          FreeRTOS Scheduler
   và tự canh thời gian                    (tự quyết ai chạy lúc nào)
```

Ba khái niệm mới, định nghĩa một lần và dùng suốt các tầng sau:

**① Task** — một hàm `void f(void *)` chứa `while(1)`, có **stack riêng** và
**priority riêng**. Có thể hình dung là một "chương trình mini". Project này có
4 task.

**② Trạng thái của task** — mỗi task luôn ở đúng một trong các trạng thái sau
(đầy đủ ở [LEARNING_NOTES §B.1](LEARNING_NOTES.md)):

```
xTaskCreate() ──► [Ready]  ← sẵn sàng chạy, đang xếp hàng chờ CPU
                     │  scheduler chọn
                     ▼
                 [Running] ← đang thực sự chạy trên CPU
                     │  gọi vTaskDelay() hoặc chờ dữ liệu
                     ▼
                 [Blocked] ← đang NGỦ, không tốn một chút CPU nào
                     │  hết giờ, hoặc dữ liệu tới
                     └──────► [Ready]
```

Trên CPU một nhân, **đúng một task ở trạng thái Running tại mỗi thời điểm.**
"Chạy song song" thực chất là chuyển qua lại rất nhanh giữa các task.

**③ Priority** — số nguyên, càng lớn càng ưu tiên. Quy tắc của scheduler chỉ
gồm một câu:

> **Luôn chạy task có priority cao nhất đang ở trạng thái Ready.**

Đây là toàn bộ thuật toán lập lịch. Mọi hành vi phức tạp về sau đều suy ra từ
câu này.

### Khác biệt cốt lõi: `Blocked` là thứ superloop không có

Đây là điểm phải nắm bằng được của cả tầng:

| | `HAL_Delay(100)` | `vTaskDelay(pdMS_TO_TICKS(100))` |
| --- | --- | --- |
| Cơ chế | Quay vòng rỗng, đọc tick liên tục | Task chuyển sang `Blocked`, kernel ghi "đánh thức sau 100 tick" |
| CPU trong lúc chờ | **100 %** — không ai dùng được | **0 %** — task khác chạy tự do |
| Task khác | Chỉ task priority **cao hơn** mới chen vào được | **Mọi** task khác đều chạy được |
| Trạng thái | Vẫn `Running` | `Blocked` |

**Đây chính là câu trả lời cho hiện tượng ở §1.1.** Trong 20 giây "đang giặt",
WashingManager không hề bận — nó đang `Blocked`, ngủ, trả CPU về cho hệ thống.
SensorTask nhờ đó vẫn kiểm tra cánh cửa đều đặn, và khi cửa mở, WashingManager
được đánh thức ngay để dừng motor.

Nói cách khác: **RTOS biến thời gian chờ từ thứ bị lãng phí thành tài nguyên
dùng được.**

## 1.4 Code thật — task đơn giản nhất trong project

Task ngắn nhất là SensorTask, 20 dòng
([sensor_task.c:16-35](../App/SensorTask/Src/sensor_task.c#L16-L35)):

```c
void SensorTask(void *pvParameters) {
  (void)pvParameters;                      /* tham số không dùng               */

  while (1) {                              /* ① task KHÔNG BAO GIỜ return      */
    BSP_Sensor_Poll();                     /* ② đọc PD5/PD6                    */

    if (BSP_Sensor_IsDoorClosed()) {       /* ③ báo tin cho task khác          */
      xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
    } else {
      xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
      xEventGroupSetBits(xSystemEvents, EVT_ERROR);
    }

    if (BSP_Sensor_IsWaterFull()) {        /* bit nước theo đúng mức cảm biến  */
      xEventGroupSetBits(xSystemEvents, EVT_WATER_FULL);
    } else {
      xEventGroupClearBits(xSystemEvents, EVT_WATER_FULL);
    }

    vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));  /* ④ ngủ 100 ms         */
  }
}
```

Bốn điểm, theo đúng thứ tự đánh số:

**① `while(1)` là bắt buộc.** Task function **không được return**. Nếu return,
hành vi là *undefined* — thường là crash. Muốn kết thúc task thì phải gọi
`vTaskDelete(NULL)`.

**② `BSP_...`** (Board Support Package, định nghĩa ở §0.3) là lớp che phần cứng — Tầng 2 giải thích vì sao task
không gọi thẳng `HAL_GPIO_ReadPin()`.

**③ `xEventGroupSetBits`** — cách task này báo tin cho task khác. **Cứ bỏ qua ở
tầng này**, Tầng 4 sẽ mổ kỹ.

**④ `vTaskDelay(pdMS_TO_TICKS(100))`** — dòng quan trọng nhất. Task tự chuyển
sang `Blocked` 100 ms. Trong 100 ms đó **SensorTask tiêu thụ 0 % CPU**.

`pdMS_TO_TICKS()` đổi mili-giây sang **tick**. Project này đặt
`configTICK_RATE_HZ = 1000` nên **1 tick = 1 ms** → `pdMS_TO_TICKS(100)` = 100.
Luôn dùng macro này, đừng viết số tick trực tiếp — đổi tick rate là sai hết.

### Còn stack thì sao?

Nhìn lại dòng tạo task ([main.c:117](../Core/Src/main.c#L117)):

```c
xTaskCreate(SensorTask, "Sensor", 128, NULL, TASK_PRIORITY_SENSOR, NULL);
/*          hàm         tên      ^^^   tham số  priority          handle */
/*                            stack depth                                */
```

⚠ **Bẫy kinh điển:** `128` **không phải 128 bytes**. Đơn vị là **words**. Trên
Cortex-M4, 1 word = 4 bytes → `128 × 4 = 512 bytes` RAM thật.

Stack của mỗi task chứa: biến cục bộ, khung gọi hàm, và toàn bộ thanh ghi được
lưu lại mỗi lần chuyển task. Cấp thiếu → task ghi đè lên RAM của task khác →
crash **ở chỗ khác, muộn hơn nhiều** so với chỗ thực sự gây lỗi. Đây là loại
bug khó tìm nhất trong RTOS.

Bốn task của project tiêu tốn: `(256 + 128 + 128 + 256) × 4 = 3072 bytes` trong
tổng 128 KB SRAM.

### Một chi tiết dễ hiểu nhầm: `HAL_Delay()` vẫn hoạt động

Bình thường HAL dùng SysTick để đếm tick, mà FreeRTOS cũng cần SysTick — hai
bên tranh nhau. Project này giải quyết bằng cách **chuyển HAL sang dùng một
TIM riêng**: xem [Core/Src/stm32f4xx_hal_timebase_tim.c](../Core/Src/stm32f4xx_hal_timebase_tim.c).

Nên `HAL_Delay()` ở đây **vẫn chạy đúng chức năng** — nó chỉ *sai về thiết kế*.
Điều này khiến nó nguy hiểm hơn: bug kiểu này không làm chương trình chết, nó
chỉ âm thầm bóp nghẹt các task priority thấp. Thí nghiệm B dưới đây cho bạn
thấy tận mắt.

## 1.5 Thí nghiệm

### Thí nghiệm A — bằng chứng của `Blocked` (không sửa code)

1. Chạy Kịch bản A tới bước 4 (đèn 🟢 sáng, đang giặt).
2. Bấm **PD5** (mở cửa) và nhìn đèn xanh lá.

**✍ Dự đoán trước:** đèn xanh lá tắt sau bao lâu — ngay, hay phải đợi hết 20 giây
của chu kỳ giặt? Máy vào trạng thái nào?

**Kết quả cần thấy:** đèn xanh lá tắt gần như ngay (chưa tới một phần năm giây),
máy vào `ERROR`. Không có chuyện lồng vẫn quay nốt phần giặt còn lại.

**Giải thích:** WashingManager đang `Blocked` — việc nó "đang giặt" không tiêu
tốn CPU chút nào, LED xanh chỉ là một chân GPIO đã được bật rồi để đấy. Cũng
chính vì nó chỉ *ngủ chờ* chứ không *bận đếm*, SensorTask (cứ 100 ms đọc cửa một
lần) mới có CPU để thấy cửa mở, và WashingManager được đánh thức ngay để dừng
motor. Đối chiếu với superloop ở §1.2, dòng 2: *"lồng vẫn quay tiếp 20 giây rồi
máy mới biết"* — đây chính là điều RTOS đã chữa.

> **Thử thêm:** vẫn trong lúc đang giặt, bấm **PA0** vài lần. Không có gì xảy ra —
> con trỏ menu đứng im. Đó là **chủ ý thiết kế**: WashingManager chỉ đọc nút bấm
> khi máy rảnh (`IDLE`, `FINISH`, `ERROR`). Vì sao nó chỉ đọc nút ở những lúc đó,
> và nút bấm đi qua những đâu trước khi tới nó, là nội dung của Tầng 3.

### Thí nghiệm B — phá hệ thống bằng `HAL_Delay()`

Đây là thí nghiệm quan trọng nhất của tầng. Bạn sẽ **cố tình làm hỏng** hệ thống.

**Bước 1 —** mở [sensor_task.c:34](../App/SensorTask/Src/sensor_task.c#L34),
đổi đúng một dòng:

```c
    /* vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS)); */
    HAL_Delay(SENSOR_TASK_PERIOD_MS);   /* ← thí nghiệm, sẽ hoàn tác */
```

(nhớ `#include "stm32f4xx_hal.h"` nếu trình biên dịch báo thiếu)

**Bước 2 — ✍ viết dự đoán ra giấy TRƯỚC KHI nạp.** Dữ kiện để suy luận: bảng
priority ở [app_config.h:60-70](../App/Common/Inc/app_config.h#L60-L70) là
WashMgr = 3, UITask = 2, **SensorTask = 2**, OLEDTask = 1. Và quy tắc duy nhất
của scheduler: *luôn chạy task Ready có priority cao nhất.*

> - SensorTask bây giờ có bao giờ vào trạng thái `Blocked` nữa không?
> - Task nào **vẫn** chạy được? Task nào **không bao giờ** chạy nữa?
> - Trên board, cái gì sẽ đứng hình? LED? OLED? Nút bấm?

**Bước 3 —** build, nạp, quan sát. Bấm nút, chạy thử một chu trình đầy đủ.

**Bước 4 —** so dự đoán với thực tế, rồi giải thích **bằng quy tắc scheduler**,
không phải bằng cảm giác.

### ⛔ Dừng lại — viết dự đoán ra giấy xong mới đọc tiếp

### Kết quả và giải thích

Gốc rễ: `HAL_Delay()` là quay vòng rỗng, nên **SensorTask không bao giờ vào
`Blocked` nữa** — nó luôn ở `Running` hoặc `Ready`. Từ đó áp quy tắc *"luôn chạy
task Ready có priority cao nhất"* cho từng task:

| Task | Prio | Còn chạy được không? | Vì sao |
| --- | --- | --- | --- |
| WashingManager | **3** | ✅ Bình thường | Priority **cao hơn** SensorTask → cứ có việc là chen vào ngay. Đèn LED vẫn chuyển giai đoạn đúng giờ |
| SensorTask | **2** | ✅ Chạy (quá nhiều) | Đang chiếm CPU vô ích 100 ms mỗi vòng |
| UITask | **2** | ✅ Vẫn chạy | **Bằng** priority với SensorTask. Project bật `configUSE_TIME_SLICING = 1` ([LEARNING_NOTES §C.2](LEARNING_NOTES.md)) nên hai task cùng mức chia nhau CPU theo vòng, mỗi tick đổi phiên. Nút bấm vẫn ăn |
| OLEDTask | **1** | ❌ **Không bao giờ chạy** | Priority **thấp hơn**. Mức 2 lúc nào cũng có task Ready → scheduler không bao giờ xét tới mức 1. **Màn hình đứng hình vĩnh viễn** |

Hiện tượng OLEDTask gặp phải có tên riêng: **starvation** (chết đói) — task
không bị lỗi gì cả, nó chỉ đơn giản là **không bao giờ được scheduler chọn**.

Ba điều rút ra, quan trọng theo thứ tự tăng dần:

1. **Priority là quan hệ so sánh, không phải điểm số.** OLEDTask không "chậm
   hơn" — nó *không chạy*. Chênh một bậc priority với một task không bao giờ
   ngủ là đủ để bị loại khỏi cuộc chơi hoàn toàn.
2. **Bug này không làm chương trình crash.** Đèn vẫn chạy, nút vẫn ăn, chu trình
   vẫn đúng giờ — chỉ mỗi màn hình chết. Người mới sẽ đi tìm bug trong driver
   OLED, trong I2C, trong dây nối, và không bao giờ tìm ra, vì nguyên nhân nằm
   ở **một file hoàn toàn khác** (`sensor_task.c`) mà nhìn qua thì chẳng liên
   quan gì tới màn hình.
3. **Task nào cũng phải có lúc `Blocked`.** Đây là quy tắc sống còn của RTOS.
   Một task không bao giờ block sẽ âm thầm giết mọi task có priority thấp hơn
   nó. Nếu buộc phải viết vòng lặp bận, ít nhất phải chèn `vTaskDelay(1)` để
   nhường lượt.

**Bước 5 — hoàn tác.** Trả lại `vTaskDelay()`, build lại, xác nhận OLED sống lại.

> ⚠ Nhớ hoàn tác thật. Đừng commit thí nghiệm này.

## 1.6 Tự kiểm tra

Tự trả lời trước, rồi đọc phần giải thích ngay bên dưới.

---

**Câu 1. WashingManager có priority 3, cao nhất trong 4 task. Vậy nó có dùng
nhiều CPU nhất không?**

**Trả lời: Không.** Đây là hiểu nhầm phổ biến nhất của người mới học RTOS.

Priority trả lời câu hỏi **"khi nhiều task cùng Ready thì ai được chạy trước?"**
— nó **không** trả lời câu hỏi "ai chạy bao lâu".

Thực tế trong project này, WashingManager gần như **luôn ở `Blocked`**: nó nằm
chờ lệnh từ người dùng, chờ tin từ cảm biến, chờ timer hết giờ. Suốt 20 giây
"đang giặt", nó **không hề chạy một chỉ thị nào** — LED xanh chỉ là chân GPIO đã
bật từ trước, và giữ nguyên mức điện áp thì không tốn CPU.

Cách hiểu đúng:

| Priority cao nghĩa là | Priority cao **không** nghĩa là |
| --- | --- |
| Phản ứng **nhanh** khi có việc | Chạy **nhiều** |
| Được ưu tiên khi tranh CPU | Chiếm CPU |
| Độ trễ thấp (*low latency*) | Thông lượng cao (*high throughput*) |

Vì thế quy tắc gán priority trong thực tế là: **task nào cần phản ứng gấp thì
priority cao**, chứ không phải task nào "quan trọng" thì priority cao.
WashingManager được ưu tiên 3 vì khi cửa mở ra giữa lúc lồng đang quay, nó phải
dừng motor **ngay**, không được xếp hàng sau việc vẽ màn hình.

---

**Câu 2. Một task gọi `vTaskDelay(pdMS_TO_TICKS(100))`. Sau đúng 100 ms nó có
chắc chắn đang chạy không?**

**Trả lời: Không chắc.** `vTaskDelay(100)` chỉ đảm bảo task ngủ **ít nhất** 100
tick, không đảm bảo nó chạy lại đúng tại mốc 100 ms.

Chuyện thực sự xảy ra tại thời điểm hết giờ:

```text
t = 0 ms    : task gọi vTaskDelay(100)  →  [Blocked]
t = 100 ms  : kernel đánh thức          →  [Ready]   ← MỚI CHỈ TỚI ĐÂY
t = ? ms    : scheduler chọn tới nó     →  [Running]
```

Bước cuối cùng xảy ra **lúc nào** thì phụ thuộc: nếu ngay lúc đó có task
priority cao hơn đang `Running`, task của bạn phải xếp hàng chờ ở `Ready`. Nếu
có task priority cao hơn chạy dài 5 ms, bạn trễ 5 ms.

Đây là ý nghĩa thật của chữ "Real-Time" trong RTOS — và nó **không** phải như
nhiều người tưởng:

- ✅ RTOS đảm bảo **thứ tự ưu tiên** và **độ trễ có chặn trên tính được**.
- ❌ RTOS **không** đảm bảo thời điểm tuyệt đối chính xác đến từng mili-giây.

Cần chính xác tuyệt đối thì phải dùng phần cứng (timer capture/compare, PWM,
DMA), không dùng task.

Hệ quả kèm theo: `vTaskDelay(100)` gọi trong vòng lặp sẽ **trôi nhịp** (drift),
vì thời gian xử lý của thân vòng lặp cộng dồn vào chu kỳ. Muốn chu kỳ đều tuyệt
đối thì dùng `vTaskDelayUntil()` — so sánh chi tiết ở
[LEARNING_NOTES §B.4](LEARNING_NOTES.md). SensorTask dùng `vTaskDelay` là chấp
nhận được vì nó chỉ cần "khoảng 100 ms", không cần chính xác.

---

**Câu 3. `xTaskCreate(..., 128, ...)` cấp bao nhiêu bytes stack? Cấp thiếu thì
triệu chứng thế nào, và compiler có báo được không?**

**Trả lời: 512 bytes** — `128 words × 4 bytes/word` trên Cortex-M4.

**Triệu chứng khi cấp thiếu:** task ghi vượt quá vùng stack của nó và **đè lên
vùng RAM kế bên** — có thể là stack của task khác, có thể là biến toàn cục, có
thể là cấu trúc nội bộ của kernel. Hậu quả:

- Crash xảy ra **muộn hơn** thời điểm gây lỗi, và **ở một file khác**.
- Triệu chứng thường là HardFault ở một chỗ trông hoàn toàn vô nghĩa, hoặc tệ
  hơn: một biến của task khác tự nhiên đổi giá trị mà không ai ghi vào nó.
- Cùng một bug có thể cho triệu chứng khác nhau ở mỗi lần chạy, tùy thời điểm
  chuyển task.

Đây là loại bug khó tìm nhất trong RTOS, vì **chỗ lỗi biểu hiện không phải chỗ
lỗi phát sinh**.

**Compiler có báo được không? Không.** Compiler không biết được lúc chạy call
stack sẽ sâu bao nhiêu — độ sâu phụ thuộc luồng rẽ nhánh, đệ quy, và cả những
hàm thư viện gọi lồng nhau (`sprintf()` là thủ phạm kinh điển, ngốn hàng trăm
byte stack). `xTaskCreate(..., 128, ...)` với compiler chỉ là một con số nguyên.
Đây là **lỗi runtime, không phải lỗi compile-time** — đã bàn ở
[LEARNING_NOTES Phần D, Session 07 Q4](LEARNING_NOTES.md).

**Cách phòng trong thực tế:**

| Công cụ | Làm gì |
| --- | --- |
| `configCHECK_FOR_STACK_OVERFLOW = 2` | Kernel kiểm tra mẫu byte canh mỗi lần chuyển task, gọi hook khi phát hiện tràn — bắt lỗi **sớm hơn nhiều** |
| `uxTaskGetStackHighWaterMark()` | Trả về lượng stack còn dư ít nhất từ trước tới giờ. Cho số thật để chỉnh, thay vì đoán |

Cách làm của kỹ sư có kinh nghiệm: cấp dư lúc phát triển, cho chạy hết mọi kịch
bản, rồi đo bằng high-water-mark và cắt xuống — chứ **không đoán số ngay từ
đầu**.

## 1.7 Đào sâu

- Scheduler chọn task ở chỗ nào trong source kernel → [LEARNING_NOTES §5.8](LEARNING_NOTES.md)
- SysTick, tick rate, độ chính xác thực tế của `vTaskDelay` → [LEARNING_NOTES §5.1–§5.7](LEARNING_NOTES.md)
- `vTaskDelay` vs `vTaskDelayUntil` (hiện tượng trôi nhịp) → [LEARNING_NOTES §B.4](LEARNING_NOTES.md)
---
---

# Tầng 2 — Bốn task của project

> **Mục tiêu:** trả lời câu hỏi mà mọi người mới học RTOS đều mắc kẹt —
> **chia hệ thống thành bao nhiêu task, và chia theo tiêu chí gì?**
> Tầng 1 nói *vì sao cần task*. Tầng này nói *chia task thế nào cho đúng*.

## 2.1 Hiện tượng

Mở [washing_manager.c](../App/WashingManager/Src/washing_manager.c) — file lớn
nhất project, 359 dòng, nơi chứa toàn bộ logic máy giặt. Tìm trong đó:

| Tìm gì | Kết quả |
| --- | --- |
| `HAL_GPIO_WritePin` | **Không có dòng nào** |
| `GPIOD`, `GPIO_PIN_12` | **Không có** |
| Code vẽ màn hình OLED | **Không có** |
| `ssd1306_...` | **Không có** |

Vậy mà khi vào trạng thái `WASHING`, đèn xanh lá **vẫn sáng** và màn OLED **vẫn
hiện chữ "WASHING"**.

Hai câu hỏi của tầng này:

1. Bằng cách nào file này bật được đèn mà không hề chạm tới GPIO?
2. Vì sao project chia thành **đúng 4** task — không phải 1, cũng không phải 10?

## 2.2 Cách ngây thơ

Tầng 1 đã loại bỏ phương án **một task làm hết**. Nhưng khi người mới biết "RTOS
có nhiều task", họ thường lao sang thái cực ngược lại: **chia task theo đồ vật**.

```c
/* ❌ Cách chia task sai điển hình */
xTaskCreate(MotorTask,      "Motor",   128, NULL, 2, NULL);
xTaskCreate(InletValveTask, "Inlet",   128, NULL, 2, NULL);
xTaskCreate(DrainValveTask, "Drain",   128, NULL, 2, NULL);
xTaskCreate(StatusLedTask,  "Led",     128, NULL, 2, NULL);
xTaskCreate(DoorTask,       "Door",    128, NULL, 2, NULL);
xTaskCreate(WaterTask,      "Water",   128, NULL, 2, NULL);
xTaskCreate(ButtonTask,     "Button",  128, NULL, 2, NULL);
xTaskCreate(OledTask,       "Oled",    256, NULL, 2, NULL);
```

Nhìn có vẻ "rất hướng đối tượng". Nhưng nó hỏng ở ba chỗ:

**① Tốn RAM vô ích.** Mỗi task tốn stack riêng cộng thêm một khối TCB (khoảng
100 byte). Tám task ở trên tốn `7×128 + 256 = 1152 words = 4608 bytes` stack,
gấp rưỡi phương án hiện tại, để làm **cùng một lượng việc**.

**② Task rỗng không có việc gì để làm.** `MotorTask` sẽ viết gì trong `while(1)`
của nó? Bật motor là **một lệnh ghi GPIO**, mất vài chục nano-giây. Không có gì
để "chạy liên tục" cả. Task đó sẽ dành 99.999 % thời gian ngồi chờ ai đó bảo nó
bật motor — tức là bạn vừa dựng thêm một tầng trung gian vô nghĩa.

**③ Nghiêm trọng nhất: trạng thái máy giặt bị xé nát.** Câu hỏi "máy đang giặt
hay đang xả?" giờ không có chủ. `MotorTask` biết motor đang quay, `DrainValveTask`
biết van đang mở, nhưng **không ai biết máy đang ở giai đoạn nào**. Lối thoát duy
nhất là dựng một đống biến toàn cục cho mọi task cùng đọc-ghi — và đó là công
thức chuẩn để tạo ra **race condition**: hai task đọc-sửa-ghi cùng một biến, kết
quả phụ thuộc thời điểm chuyển task, bug tái hiện được một lần trong một trăm.

Bài học: **task không phải là "đồ vật". Task là một luồng công việc.**

## 2.3 Ý tưởng — bốn tiêu chí chia task

Kỹ sư có kinh nghiệm không hỏi "hệ thống có mấy thiết bị?". Họ hỏi bốn câu sau,
và **mỗi câu trả lời khác nhau là một task riêng**:

| # | Tiêu chí | Câu hỏi | Vì sao phải tách |
| --- | --- | --- | --- |
| **1** | **Nguồn chờ** (*blocking source*) | Việc này nằm chờ **cái gì**? | Một task chỉ block trên **một** thứ tại một thời điểm. Hai việc chờ hai nguồn khác nhau thì không thể nhét chung |
| **2** | **Nhịp** (*rate*) | Việc này chạy **định kỳ** hay **theo sự kiện**? | Trộn chung thì việc theo sự kiện bị kẹt sau việc định kỳ |
| **3** | **Độ gấp** (*latency*) | Chậm 100 ms có sao không? | Việc gấp phải được priority cao hơn. Nhét chung một task thì không thể phân biệt |
| **4** | **Quyền sở hữu** (*ownership*) | Tài nguyên dùng chung này ai được đụng vào? | Mỗi tài nguyên có **đúng một** task sở hữu → không cần khóa, không có race |

Tiêu chí 1 là quan trọng nhất và cũng là tiêu chí bị bỏ quên nhiều nhất. Đây là
lý do bốn task này tồn tại — mỗi task **nằm chờ một thứ khác nhau**:

### Bốn task, nhìn một lượt

| Task | Prio | Stack | Nằm chờ gì | Sở hữu gì |
| --- | --- | --- | --- | --- |
| **WashingManager** | **3** | 256 w | Lệnh từ người dùng **hoặc** sự kiện hệ thống — tùy trạng thái | **Máy trạng thái** + **con trỏ menu** + motor, van, đèn |
| **UITask** | **2** | 128 w | Sự kiện nút bấm (do ISR đẩy tới) | Không giữ trạng thái nào — chỉ đổi cử chỉ nút bấm thành lệnh |
| **SensorTask** | **2** | 128 w | **Không chờ ai** — chạy định kỳ 100 ms | Trạng thái cảm biến cửa/nước |
| **OLEDTask** | **1** | 256 w | Yêu cầu vẽ màn hình | **Toàn quyền truy cập OLED / I2C1** |

Đối chiếu: [main.c:114-118](../Core/Src/main.c#L114-L118),
priority ở [app_config.h:60-70](../App/Common/Inc/app_config.h#L60-L70).

Ba trong bốn task **hoàn toàn không tiêu tốn CPU** cho tới khi có việc — chúng
nằm `Blocked`. Chỉ SensorTask là thức dậy đều đặn 10 lần mỗi giây.

### Vì sao priority được gán như vậy

Nhắc lại quy tắc từ Tầng 1: **priority cao = phản ứng gấp**, không phải quan
trọng. Đọc bảng dưới theo đúng tinh thần đó:

| Task | Prio | Lý do |
| --- | --- | --- |
| WashingManager | **3** | Đây là task **an toàn**. Cửa mở ra giữa lúc lồng đang quay thì motor phải dừng **ngay**, không được xếp hàng sau việc vẽ màn hình. Chậm ở đây là nguy hiểm vật lý |
| UITask | **2** | Người dùng cảm nhận được độ trễ trên ~100 ms. Cần nhanh, nhưng không nhanh bằng dừng motor |
| SensorTask | **2** | Ngang UITask: nó là **nguồn sinh ra** tin "cửa mở". Nếu để thấp hơn UITask, tin báo nguy hiểm sẽ bị việc bấm nút làm trễ |
| OLEDTask | **1** | Thấp nhất, và **cố ý**. Vẽ một khung hình OLED tốn hàng chục mili-giây trên I2C. Nếu nó chạy ở priority cao, mỗi lần vẽ sẽ chặn cả hệ thống. Màn hình chậm 50 ms thì không ai chết; motor chậm 50 ms thì có |

Nguyên tắc rút ra, dùng được cho mọi project: **việc chậm và không gấp phải nằm
ở priority thấp nhất.** Đây gọi là task "nền" (*background task*).

### Lớp BSP — lời giải cho câu hỏi 2.1

Còn câu hỏi "sao `washing_manager.c` bật được đèn mà không chạm GPIO?".
Câu trả lời nằm ở **kiến trúc phân tầng**:

```text
 ┌──────────────────────────────────────────────┐
 │  App/WashingManager, UITask, SensorTask...   │  ← "làm gì"
 │  Biết: máy đang giặt, cần bật motor           │
 │  KHÔNG biết: motor nối vào chân nào           │
 └──────────────────┬───────────────────────────┘
                    │  BSP_Motor_Start()
 ┌──────────────────▼───────────────────────────┐
 │  App/BSP  — bsp_motor, bsp_valve, bsp_led...  │  ← "làm thế nào"
 │  Biết: motor = PD12, bật = ghi mức HIGH       │
 │  KHÔNG biết: vì sao lại bật motor lúc này     │
 └──────────────────┬───────────────────────────┘
                    │  HAL_GPIO_WritePin()
 ┌──────────────────▼───────────────────────────┐
 │  Drivers/STM32 HAL                            │  ← thanh ghi
 └───────────────────────────────────────────────┘
```

Quy tắc: **mỗi tầng chỉ được gọi xuống tầng ngay dưới nó, không nhảy cóc.**
`washing_manager.c` gọi `BSP_Motor_Start()`, không bao giờ gọi
`HAL_GPIO_WritePin()`.

Ba lợi ích, xếp theo mức quan trọng tăng dần:

1. **Đọc dễ hơn.** `BSP_Motor_Start()` nói rõ *ý định*.
   `HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)` bắt người đọc phải tra
   xem PD12 là cái gì.
2. **Đổi phần cứng không phải sửa logic.** Đổi chân motor, đổi board, thay LED
   bằng driver công suất thật — chỉ sửa trong `App/BSP/`. Thí nghiệm B ở §2.5
   sẽ chứng minh điều này bằng tay.
3. **Logic test được mà không cần phần cứng.** Vì `washing_manager.c` không
   phụ thuộc HAL, có thể biên dịch nó trên PC với một bộ `BSP_*` giả để chạy
   unit test cho máy trạng thái.

> **Vì sao mỗi thiết bị một file BSP riêng** (`bsp_motor.c`, `bsp_valve.c`,
> `bsp_led.c`) thay vì gộp chung một `bsp_gpio.c`? Vì tiêu chí ownership: khi
> mỗi chân có đúng một file sở hữu, không bao giờ xảy ra chuyện hai chỗ trong
> code cùng điều khiển một chân mà không biết về nhau. Gộp chung thì ranh giới
> đó biến mất.

## 2.4 Code thật

### (a) Mỗi task chờ một nguồn khác nhau

Đây là bằng chứng trực tiếp cho tiêu chí 1. Bốn dòng, lấy từ bốn file khác nhau:

```c
/* UITask — chờ nút bấm            (ui_task.c:49)      */
xQueueReceive(xButtonQueue, &event, portMAX_DELAY);

/* OLEDTask — chờ yêu cầu vẽ       (oled_task.c:55)    */
xQueueReceive(xDisplayQueue, &msg, portMAX_DELAY);

/* WashingManager — chờ lệnh       (washing_manager.c:287) */
xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY);

/* SensorTask — không chờ ai, tự ngủ theo nhịp (sensor_task.c:34) */
vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
```

`portMAX_DELAY` nghĩa là **chờ vô hạn, không timeout**. Task nằm `Blocked` cho
tới khi có dữ liệu, tiêu thụ 0 % CPU.

> **Chưa cần hiểu `xQueueReceive` hoạt động ra sao** — đó là Tầng 3. Ở tầng này
> chỉ cần thấy: bốn task, bốn nguồn chờ khác nhau. Đó chính là ranh giới chia task.

### (b) WashingManager: chờ thứ gì tùy theo đang ở trạng thái nào

Đây là chỗ đặc biệt nhất project. Vòng lặp chính của WashingManager
([washing_manager.c:272-290](../App/WashingManager/Src/washing_manager.c#L272-L290)),
rút gọn:

```c
for (;;)
{
    WashState_t state = WashingSM_GetState();   /* đang ở đâu?          */

    switch (state)
    {
        case WASH_STATE_IDLE:
        case WASH_STATE_FINISH:
        case WASH_STATE_ERROR:
            /* Máy đứng yên → chỉ có người dùng mới đánh thức được */
            xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY);
            Manager_HandleCommand(&cmd);
            break;

        case WASH_STATE_FILL_WATER:
            /* Đang bơm nước → chờ 1 trong 3: nước đầy / hết giờ / lỗi */
            bits = xEventGroupWaitBits(xSystemEvents,
                       EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR,
                       pdTRUE, pdFALSE, portMAX_DELAY);
            /* ... xử lý theo bit nào được set ... */
            break;

        /* WASHING / DRAIN / SPIN: chờ EVT_TIMEOUT | EVT_ERROR */
    }
}
```

Đọc kỹ ý này, vì nó là điểm khác biệt lớn giữa code nghiệp dư và code chuyên
nghiệp: **task này không chờ "mọi thứ có thể xảy ra". Nó chỉ chờ đúng những thứ
có ý nghĩa với trạng thái hiện tại.**

Ở `IDLE`, tin "nước đầy" là **vô nghĩa** — làm gì có nước. Nên ở `IDLE`,
WashingManager thậm chí **không thèm nghe** sự kiện đó; nó nằm chờ trên
`xCommandQueue` và chỉ tỉnh dậy khi người dùng bấm nút.

Hệ quả: không cần một chuỗi `if` dài để lọc bỏ sự kiện không hợp lệ. **Việc lọc
được thực hiện bởi chính chỗ mà task chọn để nằm chờ.**

Máy trạng thái đầy đủ ở Tầng 5. Ở đây chỉ cần nhớ một câu:
*task có thể đổi nguồn chờ theo hoàn cảnh.*

### (c) Ownership — mỗi tài nguyên đúng một chủ

Hai quy tắc sở hữu của project, ghi trong [CLAUDE.md](../CLAUDE.md):

| Tài nguyên | Chủ sở hữu duy nhất | Task khác muốn dùng thì |
| --- | --- | --- |
| Máy trạng thái | WashingManager | Gửi lệnh vào `xCommandQueue` |
| Con trỏ menu | WashingManager | Gửi `CMD_NAVIGATE` vào `xCommandQueue` |
| Màn OLED + bus I2C1 | OLEDTask | Gửi nội dung vào `xDisplayQueue` |

> **Con trỏ menu từng có hai chủ** — UITask giữ một bản, WashingManager giữ một
> bản khác — và hậu quả là một lỗi màn hình có thật. Câu chuyện đó ở Tầng 3 §3.6;
> hiện tại chỉ còn một bản, nằm trong WashingManager.

Vì sao OLED phải có chủ: nếu hai task cùng gọi `ssd1306_UpdateScreen()`, hai
giao dịch I2C sẽ đè lên nhau giữa chừng — màn hình hiện rác, hoặc bus treo. Cách
chữa thông thường là dùng **mutex** để khóa. Nhưng project chọn cách rẻ hơn và
an toàn hơn: **chỉ cho đúng một task chạm vào**. Không có tranh chấp thì không
cần khóa.

Nhìn lại [oled_task.c:53-57](../App/OLEDTask/Src/oled_task.c#L53-L57): OLEDTask
chỉ có hai việc — nhận tin, vẽ. Nó **không chứa một chút logic máy giặt nào**.
Đây gọi là **pure renderer**: nó không biết máy giặt là gì, nó chỉ biết vẽ cái
người ta đưa cho.

## 2.5 Thí nghiệm

### Thí nghiệm A — cùng một nút, hai phản ứng khác nhau (không sửa code)

Chứng minh trực tiếp cho §2.4(b): nguồn chờ thay đổi theo trạng thái.

**✍ Dự đoán trước:**

> Máy đang ở `IDLE` (không đèn nào sáng). Tôi bấm **PD6** (nút "nước đầy").
> Có gì xảy ra không? Rồi tôi bấm PD2 để chạy, vào `FILL_WATER`, và bấm PD6 lần
> nữa — lần này có gì khác không?

**Các bước:**

| # | Thao tác | Quan sát |
| --- | --- | --- |
| 1 | Reset board (đang ở `IDLE`) | Không đèn nào sáng |
| 2 | Bấm **PD6** hai lần (đầy rồi lại cạn) | ? |
| 3 | Bấm **PD2** (Start) | 🟠 + 🔵 sáng — vào `FILL_WATER` |
| 4 | Bấm **PD6** một lần | ? |

**Kết quả và giải thích**

Bước 2: **không có gì xảy ra.** Bước 4: máy chuyển sang `WASHING` ngay lập tức.

Cùng một nút, cùng một đoạn code trong `sensor_task.c` chạy y hệt nhau ở cả hai
lần. Khác biệt nằm ở **phía người nghe**:

| | Bước 2 (`IDLE`) | Bước 4 (`FILL_WATER`) |
| --- | --- | --- |
| SensorTask làm gì | Set `EVT_WATER_FULL` | Set `EVT_WATER_FULL` (y hệt) |
| WashingManager đang nằm chờ ở đâu | `xQueueReceive(xCommandQueue, ...)` | `xEventGroupWaitBits(..., EVT_WATER_FULL \| ...)` |
| Kết quả | Không ai nghe → tin rơi vào hư không | Thoát `Blocked` → chuyển trạng thái |

Đây là bằng chứng cho câu ở §2.4(b): **việc lọc sự kiện được thực hiện bởi lựa
chọn nguồn chờ, không phải bằng câu `if`.**

*(Chi tiết "tin rơi vào hư không" thực ra không hoàn toàn đúng — bit vẫn nằm
trong event group và có thể gây hiệu ứng phụ ở trạng thái sau. Đó gọi là
**sticky flag**, một cái bẫy thật của project này, phân tích đầy đủ ở
[LEARNING_NOTES §3](LEARNING_NOTES.md). Tầng 4 sẽ nói tới. Riêng bit nước hiện
là bit **theo mức**: SensorTask xoá `EVT_WATER_FULL` ngay khi cảm biến báo cạn,
nên bit cũ của chu trình trước không thể làm `FILL_WATER` kết thúc oan.)*

---

### Thí nghiệm B — đổi chân motor, chứng minh lớp BSP

**Bước 1 —** mở [bsp_motor.c:40](../App/BSP/Src/bsp_motor.c#L40), sửa **đúng một
dòng**:

```c
/* #define MOTOR_PIN    GPIO_PIN_12 */
#define MOTOR_PIN    GPIO_PIN_13      /* ← thí nghiệm, sẽ hoàn tác */
```

**Bước 2 — ✍ dự đoán.** Dữ kiện: PD13 vốn là van cấp nước (đèn cam). Thứ tự lệnh
trong callback vào `WASHING` là [washing_manager.c:84-90](../App/WashingManager/Src/washing_manager.c#L84-L90):
đóng van INLET **trước**, rồi mới bật motor.

> - Vào `WASHING`, đèn nào sáng? Đèn xanh lá có sáng lần nào trong cả chu trình không?
> - Tôi phải sửa bao nhiêu dòng trong `washing_manager.c` để chuyện này xảy ra?

**Bước 3 —** build, nạp, chạy trọn một chu trình.

### ⛔ Dừng lại — viết dự đoán ra giấy xong mới đọc tiếp

### Kết quả và giải thích

Lần theo từng giai đoạn, PD13 giờ có hai chỗ điều khiển (van và motor):

| Giai đoạn | Lệnh chạy trong callback | PD13 (cam) | PD12 (xanh lá) |
| --- | --- | --- | --- |
| `FILL_WATER` | `Valve_Open(INLET)` | **Sáng** | tắt |
| `WASHING` | `Valve_Close(INLET)` → rồi `Motor_Start()` | **Vẫn sáng** | tắt |
| `DRAIN` | `Motor_Stop()` → `Valve_Open(DRAIN)` | tắt | tắt |
| `SPIN` | `Valve_Close(DRAIN)` → `Motor_Start()` | **Sáng** | tắt |

**Đèn xanh lá PD12 không sáng một lần nào trong cả chu trình.** Motor đã "chuyển
nhà" sang đèn cam.

Hai điều rút ra:

1. **Số dòng phải sửa trong `washing_manager.c`: 0.** Toàn bộ logic máy giặt —
   359 dòng — không hay biết gì về việc motor vừa đổi chân. Đây chính là giá trị
   thật của lớp BSP, và bạn vừa tự tay kiểm chứng nó. Trong project thật, "đổi
   chân" có thể là "đổi từ LED sang driver công suất", "đổi từ GPIO sang PWM",
   hay "đổi sang board rev B" — và tất cả đều gói gọn trong `App/BSP/`.

2. **PD13 giờ có hai chủ, và đó là một thiết kế tồi.** Kết quả ở bảng trên đúng
   *chỉ vì* thứ tự lệnh trong mỗi callback tình cờ hợp lý. Đảo `Valve_Close()`
   xuống sau `Motor_Start()` là đèn tắt ngay. Một tài nguyên có hai chủ thì hành
   vi phụ thuộc thứ tự thực thi — và thứ tự thực thi là thứ dễ vô tình thay đổi
   nhất khi sửa code. Đây đúng là tiêu chí 4 (ownership) ở §2.3, lần này áp
   dụng ở mức chân MCU chứ không phải mức task.

**Bước 4 — hoàn tác.** Trả `GPIO_PIN_12`, build lại, xác nhận đèn xanh lá sáng
trở lại ở `WASHING`.

## 2.6 Tự kiểm tra

Tự trả lời trước, rồi đọc phần giải thích ngay bên dưới.

---

**Câu 1. OLEDTask có priority thấp nhất (1) nhưng stack lại lớn nhất (256 words,
bằng WashingManager). Có mâu thuẫn không?**

**Trả lời: Không.** Priority và stack size là **hai trục hoàn toàn độc lập**,
không liên quan gì tới nhau.

| | Quyết định bởi | Trả lời câu hỏi |
| --- | --- | --- |
| **Priority** | Việc này gấp tới mức nào | *Khi nào* task được chạy |
| **Stack size** | Call chain sâu bao nhiêu, biến cục bộ lớn cỡ nào | Task cần *bao nhiêu RAM* để chạy |

Vì sao OLEDTask cần nhiều stack dù chẳng gấp gáp gì: nhìn chuỗi gọi hàm của nó
ở [oled_task.c:22-39](../App/OLEDTask/Src/oled_task.c#L22-L39) —
`OLEDTask` → `OLED_Render()` → `BSP_OLED_DrawText()` → `ssd1306_WriteString()` →
`ssd1306_WriteChar()` → ... xuống tới HAL I2C. Mỗi tầng gọi hàm đều đẩy thêm một
khung stack. Cộng thêm biến cục bộ `DisplayMsg_t msg` (27 byte) nằm ngay trên
stack của task ([oled_task.c:51](../App/OLEDTask/Src/oled_task.c#L51)).

Ngược lại, SensorTask ([sensor_task.c](../App/SensorTask/Src/sensor_task.c)) chỉ
đọc hai chân GPIO và set vài bit — call chain nông, gần như không có biến cục
bộ — nên 128 words là dư dùng.

Quy tắc: **cấp stack theo độ sâu call chain, gán priority theo độ gấp.** Trộn
lẫn hai tiêu chí này là lỗi thiết kế phổ biến.

---

**Câu 2. UITask và SensorTask cùng priority 2. Nếu cả hai cùng `Ready` thì task
nào chạy trước?**

**Trả lời: không xác định trước được** — và điều đó **là chủ ý**, không phải
thiếu sót.

Khi nhiều task cùng priority cùng `Ready`, FreeRTOS chạy chúng theo vòng
(*round-robin*), mỗi task được một lát thời gian rồi nhường lượt — cơ chế này
bật bởi `configUSE_TIME_SLICING = 1` ([LEARNING_NOTES §C.2](LEARNING_NOTES.md)).
Task nào đi trước phụ thuộc trạng thái danh sách Ready tại thời điểm đó, tức là
phụ thuộc lịch sử chạy trước đó.

**Điều đó có nguy hiểm không? Không** — vì hai task này **không tranh chấp gì của
nhau**:

- UITask đọc `xButtonQueue`, ghi `xCommandQueue`.
- SensorTask đọc GPIO, ghi `xSystemEvents`.
- Không có biến chung, không có tài nguyên chung.

Đây là lợi ích thầm lặng của việc chia task theo ownership (tiêu chí 4): **khi
các task không dùng chung gì cả, thứ tự chạy giữa chúng không quan trọng.**

Cách hiểu tổng quát: đặt hai task **cùng** priority là lời tuyên bố *"tôi không
quan tâm task nào chạy trước"*. Nếu bạn thực sự quan tâm, thì việc gán cùng
priority ngay từ đầu đã là sai — phải tách bậc ra.

---

**Câu 3. Vì sao WashingManager không tự đọc thẳng chân PD5 để biết cửa đóng hay
mở, mà phải đi vòng qua SensorTask?**

**Trả lời:** vì tiêu chí 4 — **PD5 phải có đúng một chủ**, và chủ đó là
`bsp_sensor.c`, được gọi từ SensorTask.

Bốn lý do, từ dễ thấy tới sâu nhất:

1. **Chống dội nút (debounce).** Nút cơ khí nảy hàng chục lần trong vài mili-giây
   khi bấm. `BSP_Sensor_Poll()` xử lý bằng **phát hiện cạnh xuống**
   ([bsp_sensor.c:91-103](../App/BSP/Src/bsp_sensor.c#L91-L103)): so mức đọc lần
   này với lần trước, mỗi lần bấm chỉ đảo cờ đúng một lần. Nếu WashingManager tự
   đọc chân, nó sẽ phải tự làm lại toàn bộ logic đó.
2. **Đọc mức chân không cho biết "cửa đóng hay mở".** PD5 là **nút toggle**:
   bấm một cái để đổi trạng thái, không phải giữ để báo trạng thái. Trạng thái
   thật nằm trong biến `s_doorClosed`
   ([bsp_sensor.c:44](../App/BSP/Src/bsp_sensor.c#L44)), không nằm ở chân.
   Ai đọc chân trực tiếp sẽ đọc ra giá trị vô nghĩa.
3. **Nếu hai chỗ cùng đọc, hai chỗ sẽ bất đồng.** Cả hai đều dùng cạnh xuống,
   mỗi chỗ giữ một biến "lần đọc trước" riêng. Task nào đọc trúng lúc nào thì
   thấy cạnh, task kia trượt — hai bên có hai ý kiến khác nhau về việc cửa đang
   đóng hay mở. Bug loại này tái hiện được một lần trong một trăm.
4. **Project này đã dính đúng lỗi đó rồi.** Có thời điểm `bsp_sensor.c` đọc cảm
   biến cửa qua **PA0** — trùng chân với nút menu. Kết quả: thả tay khỏi nút menu
   bị hiểu thành "cửa vừa mở", máy nhảy vào `ERROR` giữa chu trình. Ghi lại là
   **Bug #8** trong [HARDWARE_BUGS.md](HARDWARE_BUGS.md).

Đây không phải lý thuyết sách vở — đó là một buổi debug thật của project này.

## 2.7 Đào sâu

- Bản đồ đầy đủ "task nào đang chờ gì" → [SYSTEM_DIAGRAMS.md §9](SYSTEM_DIAGRAMS.md)
- Kiến trúc tổng thể và mọi kênh liên lạc → [SYSTEM_DIAGRAMS.md §1](SYSTEM_DIAGRAMS.md)
- Vì sao chọn Queue cho kênh này, Event Group cho kênh kia → [PROJECT_PROGRESS.md](../PROJECT_PROGRESS.md) mục TD-02
- Bug #8 (hai chỗ cùng đọc PA0) → [HARDWARE_BUGS.md](HARDWARE_BUGS.md)

---
---
---

# Tầng 3 — Task nói chuyện với nhau

> **Mục tiêu:** hiểu **Queue** — đường ống truyền dữ liệu giữa các task. Tầng 2
> nói mỗi task chờ một nguồn khác nhau. Tầng này nói *cái nguồn đó là gì* và
> *vì sao không được thay nó bằng biến toàn cục*.

## 3.1 Hiện tượng

Grep toàn bộ project:

| Tìm gì | Kết quả |
| --- | --- |
| `washing_manager.c` có gọi hàm nào của `oled_task.c` không | **Không** |
| `ui_task.c` có gọi hàm nào của `washing_manager.c` không | **Không** |
| Có biến toàn cục nào chứa "trạng thái máy giặt" cho mọi task đọc không | **Không** |

Bốn task **không hề gọi hàm của nhau, không dùng chung biến nào**. Vậy mà bấm
nút PD2 thì máy chạy, và chữ trên OLED luôn khớp với đèn LED.

Thêm một chi tiết khó hơn: nút bấm được xử lý trong **ISR** — ngữ cảnh ngắt, nơi
tuyệt đối không được block. Nhưng logic menu lại nằm trong UITask. Dữ liệu nhảy
từ ISR sang task bằng cách nào?

## 3.2 Cách ngây thơ — biến toàn cục

Đây là phản xạ đầu tiên của mọi người viết C nhúng:

```c
/* ❌ Cách này sinh ra bug không tái hiện được */
volatile ButtonEvent_t g_lastButton;
volatile uint8_t       g_hasNewButton = 0;

/* Trong ISR */
g_lastButton   = BTN_EVENT_NAVIGATE;
g_hasNewButton = 1;

/* Trong UITask */
if (g_hasNewButton) {
    g_hasNewButton = 0;
    handle(g_lastButton);
}
```

Thêm `volatile` là xong? **Không.** `volatile` chỉ bảo trình biên dịch "đừng
cache biến này vào thanh ghi". Nó **không** cho bạn tính nguyên tử, và cũng
không cho bạn thứ tự.

Bốn lỗ hổng, xếp theo mức khó phát hiện tăng dần:

**① Mất sự kiện.** Bấm nút hai lần nhanh. Lần bấm thứ hai ghi đè `g_lastButton`
trước khi UITask kịp đọc lần thứ nhất. Một sự kiện biến mất không dấu vết. Biến
toàn cục **chỉ nhớ được giá trị cuối cùng** — nó không có khái niệm hàng đợi.

**② Không block được.** UITask phải liên tục hỏi `if (g_hasNewButton)`. Muốn
không đốt CPU thì phải `vTaskDelay()` giữa các lần hỏi — mà như thế thì phản hồi
bị trễ đúng bằng chu kỳ hỏi. Tầng 1 đã chỉ ra đây chính là bệnh của superloop.

**③ Race condition trên chuỗi đọc-sửa-ghi.** Giả sử có bộ đếm
`g_pendingCount++`. Trên Cortex-M4, một dòng `++` biên dịch thành **ba lệnh**:
`LDR` → `ADD` → `STR`. Ngắt xảy ra đúng giữa `LDR` và `STR` thì kết quả của một
bên bị mất trắng. Bug này tái hiện được khoảng một lần trong một nghìn, và biến
mất khi bạn đặt breakpoint.

**④ Struct bị đọc nửa vời — nguy hiểm nhất.** Với dữ liệu nhiều trường:

```c
volatile DisplayMsg_t g_display;   /* 27 byte */
```

Task A ghi xong `menuIndex`, chưa kịp ghi `statusText` thì bị task B chen vào
đọc. Task B nhận được **một nửa dữ liệu mới ghép với một nửa dữ liệu cũ** — một
trạng thái chưa từng tồn tại trong ý định của ai cả. `volatile` không giúp được
gì: nó không làm cho 27 byte trở nên nguyên tử.

Lỗi ④ là loại khiến kỹ sư mất hàng tuần: hệ thống chạy đúng 99.9 % thời gian,
và cái 0.1 % còn lại không tài nào bắt được bằng debugger, vì dừng chương trình
lại là race biến mất.

## 3.3 Ý tưởng — Queue

Queue là **hàng đợi FIFO có khoá sẵn bên trong**, do kernel quản lý. Nó giải
quyết cả bốn lỗ hổng trên bằng bốn tính chất:

| # | Tính chất | Chữa lỗ hổng nào |
| --- | --- | --- |
| **1** | **Có chiều sâu** — chứa được nhiều phần tử xếp hàng | ① không mất sự kiện khi bấm dồn |
| **2** | **Chặn được hai chiều** — task đọc queue rỗng thì `Blocked` cho tới khi có dữ liệu | ② không phải polling, phản hồi tức thì, 0 % CPU khi rảnh |
| **3** | **Thao tác nguyên tử** — kernel tự vào vùng tới hạn khi gửi/nhận | ③ không còn race trên đọc-sửa-ghi |
| **4** | **Sao chép theo giá trị** — copy trọn `uxItemSize` byte | ④ bên nhận luôn có một **ảnh chụp trọn vẹn**, không bao giờ nửa vời |

Tính chất 4 là điểm người mới hiểu sai nhiều nhất, nên nói kỹ:

```c
xQueueSend(xDisplayQueue, &disp, 0);
/*                        ^^^^^                                    */
/*  Truyền ĐỊA CHỈ vào hàm, nhưng FreeRTOS KHÔNG lưu địa chỉ đó.   */
/*  Nó copy 27 byte từ địa chỉ đó vào vùng nhớ của queue NGAY LẬP  */
/*  TỨC, trong lúc hàm còn đang chạy. Sau khi hàm return, biến     */
/*  `disp` của bạn không còn liên quan gì tới queue nữa.           */
```

Hệ quả: **`disp` được phép là biến cục bộ trên stack.** Hàm kết thúc, khung stack
bị thu hồi, dữ liệu vẫn an toàn trong queue. Nếu queue lưu con trỏ thì đây sẽ là
lỗi dangling pointer kinh điển.

Cái giá phải trả: mỗi lần gửi và mỗi lần nhận đều tốn một phép `memcpy`. Với
`DisplayMsg_t` 27 byte trên Cortex-M4 thì không đáng kể. Với dữ liệu **lớn**
(vài KB, ví dụ một buffer ảnh) thì mới cần cân nhắc gửi con trỏ — nhưng lúc đó
bạn phải tự trả lời: ai cấp phát, ai giải phóng, và bên nhận đọc xong chưa?
Queue theo giá trị không đặt ra câu hỏi nào trong ba câu đó.

## 3.4 Code thật — ba queue của project

Tạo tại [freertos_objects.c:114-134](../App/Common/Src/freertos_objects.c#L114-L134):

```c
xButtonQueue  = xQueueCreate(BUTTON_QUEUE_LENGTH,  sizeof(ButtonEvent_t));
xCommandQueue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(CommandMsg_t));
xDisplayQueue = xQueueCreate(DISPLAY_QUEUE_LENGTH, sizeof(DisplayMsg_t));
/*                           ^^^^^^ chiều sâu       ^^^^^^ kích thước 1 phần tử */
```

| Queue | Chở gì | Bên gửi | Bên nhận | Sâu | 1 phần tử |
| --- | --- | --- | --- | --- | --- |
| `xButtonQueue` | `ButtonEvent_t` | **ISR** nút bấm | UITask | 5 | 1 byte |
| `xCommandQueue` | `CommandMsg_t` | UITask | WashingManager | 5 | 1 byte |
| `xDisplayQueue` | `DisplayMsg_t` | WashingManager *(duy nhất — xem §3.6)* | OLEDTask | 5 | 27 byte |

> **Vì sao 1 / 1 / 27 mà không phải 4 / 4 / 36?** Cột cuối là số **đo thật** bằng
> `arm-none-eabi-gcc` với đúng cờ biên dịch của project. Trình biên dịch này mặc
> định dùng `-fshort-enums`: một `enum` chiếm số byte nhỏ nhất đủ chứa giá trị
> (ở đây là 1), không phải 4 byte như một `int`. Nhiều tài liệu và ví dụ trên mạng
> ngầm giả định `sizeof(enum) == 4`; trên toolchain này điều đó **sai**, và cũng
> từng sai trong chính comment của project cho tới khi được đo lại. Quy tắc:
> **kích thước kiểu dữ liệu thì đo, đừng đoán** — một dòng
> `char (*p)[sizeof(T)] = 1;` làm trình biên dịch tự in ra con số.

Ba queue tạo thành một **dây chuyền một chiều**:

```text
  [Nút bấm]                                          [Màn OLED]
      │ ISR                                               ▲
      ▼                                                   │
 xButtonQueue ──► UITask ──► xCommandQueue ──► WashingManager
                                                  │        │
                                                  └────────┘
                                              xDisplayQueue
```

Mỗi mũi tên có **đúng một bên gửi và đúng một bên nhận**. Đây là hệ quả trực
tiếp của tiêu chí ownership ở Tầng 2 — và nó khiến toàn bộ luồng dữ liệu đọc
được chỉ bằng cách nhìn sơ đồ.

### Ba tham số timeout, ba ý nghĩa khác nhau

Cùng một hàm `xQueueSend`/`xQueueReceive`, tham số cuối đổi ý nghĩa hoàn toàn:

| Giá trị | Nghĩa | Dùng ở đâu trong project |
| --- | --- | --- |
| `portMAX_DELAY` | Chờ vô hạn. Task `Blocked` tới khi có dữ liệu | Mọi chỗ **nhận**: [ui_task.c:49](../App/UITask/Src/ui_task.c#L49), [oled_task.c:55](../App/OLEDTask/Src/oled_task.c#L55), [washing_manager.c:287](../App/WashingManager/Src/washing_manager.c#L287) |
| `0` | **Không chờ.** Queue đầy thì trả về `errQUEUE_FULL` ngay | Mọi chỗ **gửi** từ task: [ui_task.c:68](../App/UITask/Src/ui_task.c#L68) (gửi lệnh), [washing_manager.c:184](../App/WashingManager/Src/washing_manager.c#L184) (`Manager_PostDisplay`). Riêng ISR dùng bản `FromISR` ở [ui_task.c:32](../App/UITask/Src/ui_task.c#L32) |
| `pdMS_TO_TICKS(n)` | Chờ tối đa n ms rồi bỏ cuộc | Không dùng trong project này |

**Vì sao mọi chỗ gửi đều dùng timeout 0** — đây là một quyết định thiết kế, không
phải lười:

Nếu `Manager_PostDisplay()` dùng `portMAX_DELAY`, thì khi queue hiển thị đầy,
**WashingManager sẽ bị chặn lại chỉ vì màn hình vẽ chậm**. Task quan trọng nhất
hệ thống — task chịu trách nhiệm dừng motor khi cửa mở — sẽ nằm chờ một cái màn
hình. Đó là **inversion of priority theo kiểu thủ công**, và là lỗi thiết kế
nghiêm trọng.

Với timeout 0, cập nhật hiển thị bị **rơi lặng lẽ** khi quá tải. Đánh đổi này
đúng vì màn hình là dữ liệu *tự làm mới*: bỏ một khung hình thì khung sau vẫn
vẽ đúng. Ghi rõ trong comment ở
[washing_manager.c:165-173](../App/WashingManager/Src/washing_manager.c#L165-L173).

Quy tắc chung mang được sang project khác: **bên gửi ở priority cao không bao
giờ được block chờ bên nhận ở priority thấp.**

### Gửi từ ISR phải dùng hàm khác

[ui_task.c:29-35](../App/UITask/Src/ui_task.c#L29-L35):

```c
static void Button_EventCallback(ButtonEvent_t event) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);
  /*        ^^^^^^^ hậu tố bắt buộc khi ở trong ngữ cảnh ngắt */

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

Ở tầng này chỉ cần nhớ **một câu**: trong ISR phải dùng bản `...FromISR`, vì bản
thường có thể block, mà ISR thì không được phép block. Cơ chế đầy đủ — vì sao
tồn tại `xHigherPriorityTaskWoken`, `portYIELD_FROM_ISR` làm gì — để **Tầng 6**.

## 3.5 Thí nghiệm

### Thí nghiệm A — chứng minh queue sao chép, không giữ con trỏ

Đây là thí nghiệm quyết định của tầng. Nếu bạn tin rằng "truyền `&disp` nghĩa là
queue giữ địa chỉ của `disp`" thì kết quả dưới đây sẽ sửa lại niềm tin đó.

**Bước 1 —** mở [washing_manager.c:184](../App/WashingManager/Src/washing_manager.c#L184),
trong hàm `Manager_PostDisplay()`, thêm **một dòng ngay sau** lệnh gửi:

```c
    xQueueSend(xDisplayQueue, &msg, 0);
    msg.menuIndex = 99u;      /* ← thí nghiệm, sẽ hoàn tác */
}
```

**Bước 2 — ✍ dự đoán.** Nhớ rằng `OLED_Render()` vẽ con trỏ bằng
`msg->menuIndex == 0u ? "> Wash" : "  Wash"` ([oled_task.c:30-33](../App/OLEDTask/Src/oled_task.c#L30-L33)),
nên `menuIndex = 99` sẽ khiến **không dòng nào có dấu `>`**.

> - Bấm PA0 lúc máy đang rảnh. Con trỏ `>` còn hiện không?
> - `msg` là biến cục bộ của `Manager_PostDisplay()`. Hàm này **đã return** từ
>   rất lâu trước khi OLEDTask (priority 1, thấp nhất) được chạy tới. Khung stack
>   chứa `msg` lúc OLEDTask đọc queue còn tồn tại không?

**Bước 3 —** build, nạp, bấm PA0 vài lần (và thử cả một chu trình đầy đủ).

### ⛔ Dừng lại — viết dự đoán ra giấy xong mới đọc tiếp

### Kết quả và giải thích

**Con trỏ vẫn nhảy đúng.** Dòng `msg.menuIndex = 99u;` không có một chút ảnh
hưởng nào, kể cả trong một chu trình đầy đủ.

Vì `xQueueSend()` đã **copy 27 byte vào vùng nhớ riêng của queue ngay trong lúc
hàm còn chạy**. Khi hàm return, mọi liên hệ giữa biến `msg` và queue đã chấm
dứt. Bạn sửa `msg` sau đó cũng như sửa một tờ giấy đã photo xong — bản photo
không đổi. (Và `s_menuIndex` của WashingManager cũng không bị đụng tới: `msg` chỉ
là một bản sao.)

Câu hỏi thứ hai còn sắc hơn: `msg` nằm trên **stack**, trong khung của
`Manager_PostDisplay()`. Hàm return là khung đó bị thu hồi ngay lập tức — không
phải "sẽ bị ghi đè lần lặp sau", mà là **không còn tồn tại** — rồi WashingManager
còn chạy tiếp và dùng lại đúng vùng stack ấy cho các lời gọi khác. OLEDTask
(priority 1) có thể mãi tới vài chục mili-giây sau mới được chạy.

**Nếu queue lưu con trỏ, đây sẽ là lỗi dangling pointer 100 % lần nào cũng
dính.** Việc màn hình luôn vẽ đúng chính là bằng chứng cứng rằng queue đã chụp
ảnh dữ liệu, không giữ tham chiếu.

**Bước 4 — hoàn tác.** Xóa dòng vừa thêm, build lại.

---

### Thí nghiệm B — queue là bộ đệm, và dữ liệu tồn đọng phải được xử lý

**✍ Dự đoán trước:**

> Bấm PD2 để chạy, tới lúc đang giặt (đèn 🟢) thì bấm **PD2 thêm một lần nữa**.
> Lệnh đó đi đâu? Khi chu trình kết thúc, máy có tự khởi động lại không?

**Các bước:**

| # | Thao tác | Quan sát |
| --- | --- | --- |
| 1 | Bấm PD2, chờ tới `WASHING` (🟢 sáng) | Đang giặt |
| 2 | Bấm **PD2** một lần nữa | Đèn không đổi gì cả |
| 3 | Chờ hết `DRAIN` và `SPIN` | Máy vào `FINISH` |
| 4 | Quan sát 5 giây | ? |

### Kết quả và giải thích

Bước 2: **không có gì xảy ra ngay**, nhưng lệnh **không mất** — nó nằm chờ trong
`xCommandQueue`. Đây chính là tính chất 1 (có chiều sâu): WashingManager lúc đó
đang `Blocked` trên event group ở trạng thái `WASHING`, không hề đọc
`xCommandQueue`, nhưng UITask vẫn gửi vào được vì queue có chỗ.

Bước 4: máy **đứng yên ở `FINISH`**, không tự khởi động lại.

Vì sao? Vì `Manager_OnEnterFinish()` gọi
[`xQueueReset(xCommandQueue)`](../App/WashingManager/Src/washing_manager.c#L113)
— dọn sạch mọi lệnh tồn đọng trước khi bắt đầu chờ lệnh mới.

**Bỏ dòng đó đi thì chuyện gì xảy ra?** Máy vào `FINISH`, ngay lập tức đọc được
lệnh cũ còn kẹt trong queue, và tự nhảy về `IDLE` — người dùng thấy màn hình
`FINISHED` loé lên rồi biến mất, không kịp đọc. **Đó chính là Bug #10 của
project**, ghi ở [LEARNING_NOTES Phần D, Session 08](LEARNING_NOTES.md).

Bài học mang được sang project khác: **queue nhớ mọi thứ bạn gửi, kể cả những
thứ đã hết ý nghĩa.** Khi hệ thống đổi chế độ hoạt động, phải tự hỏi: *"dữ liệu
đang nằm chờ trong queue có còn đúng cho chế độ mới không?"* Nếu không, phải chủ
động dọn.

## 3.6 Một lỗi thật trong chính project này — và cách nó được sửa

Tầng này có thêm một mục ngoài khuôn, vì nó là ví dụ quá tốt về **hai chủ cho một
dữ liệu**. Lỗi này **đã được sửa**; phần mô tả "bản cũ" dưới đây là để bạn hiểu vì
sao sửa như vậy (code trước khi sửa nằm trong lịch sử git).

### Triệu chứng của bản cũ

Đang giặt, bấm PA0 để di chuyển con trỏ menu → dòng trạng thái dưới cùng của OLED
đổi từ `WASHING` thành `IDLE`, dù máy vẫn đang giặt.

### Truy nguyên

Bản cũ của UITask cũng gửi vào `xDisplayQueue` mỗi khi con trỏ di chuyển. Nhưng
`DisplayMsg_t` chứa **cả cụm** — `menuIndex` *và* `statusText`. UITask chỉ biết
`menuIndex`; nó không biết máy đang ở trạng thái nào, nên lấy tạm từ một biến đệm:

```c
static char s_statusCache[24] = "IDLE";   /* ui_task.c, bản cũ */
```

Grep cả file: **`s_statusCache` không bao giờ được ghi.** Nó nằm nguyên giá trị
khởi tạo `"IDLE"` suốt vòng đời chương trình. Comment ngay trên nó nói *"cached
status text from last WashingManager display update"* — nhưng không có dòng code
nào thực hiện việc cache đó, và cũng **không thể có**: `xDisplayQueue` là kênh
một chiều `WashingManager → OLEDTask`, UITask không đọc nó.

**Đây không phải lỗi đánh máy — đây là lỗi thiết kế kênh liên lạc:**

| Vấn đề gốc | Hai producer (WashingManager và UITask) cùng gửi vào một queue, nhưng mỗi bên **chỉ biết một phần** của thông điệp |
| --- | --- |
| Hệ quả | Bên nào gửi cũng phải bịa nốt phần mình không biết. UITask bịa `statusText`; WashingManager giữ một bản sao `menuIndex` riêng |
| Vi phạm nguyên tắc | Tiêu chí ownership ở Tầng 2: **nội dung màn hình có hai chủ** |

Và lỗi lệch này còn lộ ra **từ phía ngược lại**: bản sao `menuIndex` của
WashingManager chỉ được cập nhật khi nó nhận một lệnh. Sau khi UITask đã dời con
trỏ lúc đang giặt, tới lần chuyển trạng thái kế tiếp WashingManager gửi
`menuIndex` cũ của nó — **con trỏ trên OLED nhảy ngược về chỗ cũ.** Hai bản sao
của cùng một dữ liệu thì sớm muộn sẽ bất đồng.

Một chi tiết phụ: UITask khai `DisplayMsg_t disp;` rồi chỉ gán `menuIndex` và
`statusText`, **để nguyên `state` và `selectedMode` chưa khởi tạo** — queue vẫn
trung thành copy đủ 27 byte kể cả hai trường rác đó. Chính người viết cũng đã
cảm nhận được rằng UITask không đủ thông tin để dựng một `DisplayMsg_t` hoàn chỉnh.

### Ba hướng sửa được cân nhắc

| Hướng | Cách làm | Đánh đổi |
| --- | --- | --- |
| **1. OLEDTask nhớ trạng thái** | Thêm `updateMask` vào `DisplayMsg_t`; OLEDTask giữ khung hình gần nhất và chỉ ghi đè phần được đánh dấu | Sửa ít nhất; nhưng OLEDTask không còn là *pure renderer*, và bản sao `menuIndex` thứ hai vẫn còn đó |
| **2. UITask không gửi thẳng OLED** | UITask báo "con trỏ đã dời" cho WashingManager; WashingManager dựng `DisplayMsg_t` | Đúng nguyên tắc một chủ — **nhưng xem bên dưới** |
| **3. Tách hai queue** | Một queue cho trạng thái, một cho con trỏ; OLEDTask ghép lại | Ranh giới sạch nhất; tốn RAM và thêm một chỗ chờ |

### Vì sao không làm đúng như hướng 2 ban đầu

Khi viết ra mới thấy hướng 2 *như đã mô tả* vẫn còn lỗ. WashingManager chỉ đọc
`xCommandQueue` ở `IDLE / FINISH / ERROR` (Tầng 2) — lúc đang giặt nó nằm chờ trên
event group. Nên lệnh "con trỏ đã dời" gửi lúc đang giặt sẽ **nằm im trong queue
rồi bị `xQueueReset` xoá** khi máy về `FINISH`. UITask vẫn tin con trỏ ở vị trí
mới, màn hình thì ở vị trí cũ, và bấm Start sẽ chạy chương trình **khác với cái
đang hiển thị**. Vẫn là hai chủ, chỉ đổi chỗ lệch.

Lời giải đúng không phải là *báo* cho nhau, mà là **chỉ còn một bản**: chuyển hẳn
quyền sở hữu con trỏ sang WashingManager.

### Cách sửa thật sự

UITask thành **bộ dịch không trạng thái** — chỉ đổi tên cử chỉ thành lệnh, không
biết con trỏ ở đâu hay máy đang làm gì ([ui_task.c:49-68](../App/UITask/Src/ui_task.c#L49-L68)):

```c
BTN_EVENT_NAVIGATE  →  CMD_NAVIGATE
BTN_EVENT_CONFIRM   →  CMD_CONFIRM
```

WashingManager giữ con trỏ và là **bên duy nhất** dựng `DisplayMsg_t`
([washing_manager.c:200-228](../App/WashingManager/Src/washing_manager.c#L200-L228)):

```c
case CMD_NAVIGATE:
    s_menuIndex = (uint8_t)((s_menuIndex + 1u) % MENU_ITEM_COUNT);
    Manager_PostDisplay(state);      /* chữ trạng thái dựng từ trạng thái THẬT */
    break;

case CMD_CONFIRM:
    if (state == WASH_STATE_FINISH || state == WASH_STATE_ERROR)
        WashingSM_ProcessEvent(WASH_EVT_RESET);        /* xác nhận, về IDLE   */
    else if (s_menuIndex == 0u)
        { s_currentMode = WASH_MODE_WASH; WashingSM_ProcessEvent(WASH_EVT_START_WASH); }
    else
        { s_currentMode = WASH_MODE_SPIN; WashingSM_ProcessEvent(WASH_EVT_START_SPIN); }
    break;
```

Ba hệ quả đáng thấy:

1. **`CommandMsg_t` co từ 3 byte xuống còn 1 byte** (số đo thật; comment cũ ghi 8, rồi 12 — đều sai). Nó không cần
   mang `menuIndex` và `mode` nữa — hai thông tin đó giờ nằm trong người nhận.
2. **Chữ trạng thái không còn là chuỗi truyền tay.** `Manager_StatusText()` là một
   bảng duy nhất, tra theo trạng thái — nên chữ trên màn hình không thể lệch với
   trạng thái, như một chuỗi truyền tay (hay một biến đệm không ai ghi) từng lệch.
3. **Một cái giá, nói thẳng:** lúc máy đang chạy chu trình, nút bấm bị bỏ qua và
   con trỏ đứng im (bạn đã thử ở Tầng 1, "Thử thêm" của Thí nghiệm A). Đây là hệ
   quả trực tiếp của việc WashingManager chỉ chờ **một** nguồn tại một thời điểm.
   Nó hợp lý — giữa chu trình không có gì để chọn — và nó cũng chính là lý do
   không thể "gộp UITask vào WashingManager" mà không cân nhắc cơ chế chờ.

### Kiểm chứng

- `make clean && make all`: **0 warning**. Vùng `data` giảm đúng **24 byte**
  (44 → 20) — bằng đúng mảng `s_statusCache[24]`: bằng chứng cứng rằng biến đó từng
  chiếm RAM thật mà chưa từng được ghi.
- **Chưa thử trên board.** Thay vào đó, logic thật được chạy trên PC: biên dịch
  nguyên `washing_manager.c` và `washing_sm.c` của project, chỉ giả lập FreeRTOS và
  BSP. **37 kiểm tra đạt**, gồm cả kịch bản lỗi gốc (con trỏ dời lúc đang giặt phải
  hiện `WASHING`, không phải `IDLE`) và kịch bản con trỏ nhảy ngược.
- **Kiểm tra đột biến:** cố tình đưa lỗi cũ trở lại (gửi cứng chữ `IDLE`; cho
  `CMD_NAVIGATE` reset trạng thái cuối; không tăng con trỏ) → bài test báo hỏng cả
  ba lần. Một bài test chưa từng được thấy *thất bại* thì chưa chứng minh được gì.

### Bài học

1. **Hai bản sao của một dữ liệu thì sớm muộn sẽ lệch.** Cách sửa đúng là *bỏ một
   bản*, không phải *đồng bộ hai bản*.
2. **Một thông điệp nên do đúng một bên biết đủ dựng ra.** Hai bên cùng gửi, mỗi
   bên một nửa, là công thức của dữ liệu rác.
3. **Chạy thử một đề xuất thiết kế qua mọi trạng thái trước khi tin nó.** Hướng 2
   nghe hợp lý và chỉ hỏng ở đúng một trạng thái: lúc đang giặt.
4. **Sau khi sửa, UITask chỉ còn ~25 dòng và đổi tên 1-1.** Câu hỏi "có cần task
   này nữa không" xuất hiện rất tự nhiên — nhưng đó là một quyết định khác, chưa
   được đưa ra.

## 3.7 Tự kiểm tra

Tự trả lời trước, rồi đọc phần giải thích ngay bên dưới.

---

**Câu 1. `xDisplayQueue` sâu 5 và mỗi phần tử 27 byte. Nếu đổi
`DisplayMsg_t.statusText` từ `char[24]` thành `char *`, RAM tiết kiệm được bao
nhiêu, và cái giá phải trả là gì?**

**Trả lời:** tiết kiệm khoảng `5 × 19 = 95 byte`, và cái giá là **đánh đổi một
bug tiềm ẩn nghiêm trọng lấy 95 byte trên tổng 128 KB SRAM** — hoàn toàn không
đáng.

`char *` chỉ 4 byte, nên phần tử co từ 27 xuống 8 byte (3 trường nhỏ + 1 byte đệm căn lề + 4 byte con trỏ). Nhưng khi đó queue chở
**địa chỉ**, và câu hỏi lập tức phát sinh: chuỗi mà địa chỉ đó trỏ tới nằm ở đâu?

| Chuỗi nằm ở đâu | Chuyện gì xảy ra |
| --- | --- |
| Biến cục bộ trên stack | ❌ Khung stack bị thu hồi trước khi OLEDTask kịp đọc → **dangling pointer** |
| Chuỗi hằng `"WASHING"` | ✅ Chạy đúng, nhưng chỉ dùng được cho văn bản cố định, không ghép được số liệu |
| Cấp phát động | ❌ Phải trả lời: ai `free`, và free lúc nào cho chắc bên nhận đã đọc xong? Thêm phân mảnh heap |

Đây là lý do [app_msg.h:5-6](../App/Common/Inc/app_msg.h#L5-L6) ghi thẳng nguyên
tắc: *"No pointers to heap memory — safe, deterministic, ISR-friendly."*

Quy tắc thực chiến: **trong hệ nhúng, gửi con trỏ qua queue chỉ đáng làm khi dữ
liệu đủ lớn để phép copy trở thành vấn đề đo được** (thường là hàng KB). Dưới
mức đó, copy theo giá trị luôn thắng vì nó xoá sạch cả một lớp bug về vòng đời
bộ nhớ.

---

**Câu 2. Cả hai chỗ gửi từ task trong project đều dùng timeout `0`, tức là chấp nhận mất
dữ liệu khi queue đầy. Với `xCommandQueue` — queue chở lệnh của người dùng —
việc mất lệnh có chấp nhận được không?**

**Trả lời:** được, nhưng vì một lý do khác với `xDisplayQueue`, và đáng phân biệt
rõ.

| | `xDisplayQueue` | `xCommandQueue` |
| --- | --- | --- |
| Mất một phần tử nghĩa là | Bỏ một khung hình | Bỏ một lần bấm nút của người dùng |
| Vì sao chấp nhận được | Dữ liệu **tự làm mới** — khung sau vẫn vẽ đúng toàn bộ màn hình | Queue **chỉ đầy khi người dùng bấm 5 lần mà máy chưa xử lý xong lần nào** — tình huống chỉ xảy ra khi bấm loạn |
| Nếu dùng `portMAX_DELAY` thay thế | ❌ WashingManager (prio 3) bị OLEDTask (prio 1) chặn — đảo ưu tiên thủ công | ❌ UITask bị chặn → **ISR nút bấm vẫn chạy nhưng `xButtonQueue` đầy dần** → hỏng lan sang tầng trên |

Điểm quan trọng ở cột phải: chặn UITask **không chỉ làm chậm UITask**. UITask là
bên tiêu thụ duy nhất của `xButtonQueue`; nó dừng thì queue nút bấm đầy, và từ
đó mọi lần bấm đều mất. **Chặn một task luôn kéo theo hậu quả lan ngược lên
thượng nguồn của nó** — đây là cách suy luận cần tập thành phản xạ khi thiết kế
hệ thống nhiều task.

Cách làm chuẩn hơn cho lệnh người dùng: kiểm tra giá trị trả về của
`xQueueSend()` và báo cho người dùng biết khi thất bại (nháy đèn, kêu bíp). Hiện
project bỏ qua giá trị trả về ở cả hai chỗ gửi từ task — chấp nhận được với quy mô này,
nhưng trong sản phẩm thật thì phải xử lý.

---

**Câu 3. UITask nằm `Blocked` trên `xQueueReceive(xButtonQueue, ..., portMAX_DELAY)`.
ISR gửi một sự kiện vào queue. UITask chạy ngay lập tức chứ?**

**Trả lời: gần như ngay, nhưng "ngay" ở đây có nghĩa chính xác — và không phải
là "ngay trong ISR".**

Trình tự thật:

```text
1. ISR gọi xQueueSendFromISR()
      → dữ liệu vào queue
      → kernel thấy UITask đang chờ queue này → chuyển UITask sang [Ready]
      → đặt xHigherPriorityTaskWoken = pdTRUE nếu UITask có priority cao hơn
        task đang bị ngắt

2. ISR gọi portYIELD_FROM_ISR(xHigherPriorityTaskWoken)
      → KHÔNG chuyển task tại chỗ, chỉ ĐẶT CỜ yêu cầu chuyển

3. ISR chạy nốt phần còn lại rồi return

4. Lúc thoát ngắt, kernel mới thực hiện chuyển task → UITask [Running]
```

Ba điều rút ra:

1. **Chuyển task không bao giờ xảy ra giữa chừng một ISR.** ISR luôn chạy tới
   hết. Đây là lý do quy tắc "giữ ISR càng ngắn càng tốt" quan trọng: mọi thứ
   trong ISR đều làm trễ task sắp được đánh thức.
2. **Nếu quên `portYIELD_FROM_ISR()`**, chương trình vẫn *chạy đúng* — UITask
   đã ở `Ready` rồi — nhưng nó phải đợi tới lần chuyển task tự nhiên tiếp theo,
   trễ thêm tối đa một tick (1 ms ở project này). Loại bug im lặng: không sai
   kết quả, chỉ tăng độ trễ.
3. **UITask chỉ vào `Ready`, không phải `Running`.** Nếu lúc đó WashingManager
   (priority 3) đang chạy, UITask (priority 2) vẫn phải chờ. Đúng như Tầng 1 đã
   nói: RTOS đảm bảo thứ tự ưu tiên, không đảm bảo thời điểm tuyệt đối.

Cơ chế đầy đủ của `portYIELD_FROM_ISR` — PendSV, BASEPRI, độ ưu tiên NVIC — là
nội dung **Tầng 6**.

## 3.8 Đào sâu

- Bảng API Queue đầy đủ, cột "gọi được từ ISR?" → [LEARNING_NOTES §C.1](LEARNING_NOTES.md)
- Vì sao chọn Queue cho kênh này, Event Group cho kênh kia → [PROJECT_PROGRESS.md](../PROJECT_PROGRESS.md) mục TD-02
- Cấu trúc bên trong của Queue, chi phí RAM thực tế → [LEARNING_NOTES §1.1](LEARNING_NOTES.md) (Event Group, cùng nguyên lý danh sách task chờ)
- Bug #10 — lệnh tồn đọng trong queue → [LEARNING_NOTES Phần D, Session 08](LEARNING_NOTES.md)

---
---

# Tầng 4–6 — 🚧 Chưa viết

Khung đã chốt ở §0.3. Tầng 4 mổ **Event Group** — cơ chế đằng sau
`xEventGroupWaitBits()` đã gặp ở §2.4(b) và §3.5 Thí nghiệm B.

Phần công cụ debug (SystemView / RTT / logic analyzer) **cố ý để ngoài** — xem
ghi chú "Để sau" ở §0.3.
