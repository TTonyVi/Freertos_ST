# Q&A Learning Log

Ghi lại toàn bộ câu hỏi và câu trả lời trong quá trình học FreeRTOS.
Cập nhật sau mỗi phiên Q&A trước khi bắt đầu viết code.

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
