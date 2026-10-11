# Hardware Bring-Up Bug Report
## FreeRTOS Washing Machine — STM32F411E-DISCO

Tài liệu này ghi lại tất cả các bug phát sinh trong quá trình **lần đầu chạy firmware trên phần cứng thật**.
Mỗi bug được trình bày theo format: Triệu chứng → Root Cause → Fix → Bài học.

---

## Bug #1 — EXTI0 Priority Too High (FreeRTOS API Crash)

### Triệu chứng
Chương trình crash hoặc hoạt động bất thường ngay khi nhấn nút. Không có output rõ ràng.

### Root Cause

CubeMX tự động set priority của `EXTI0_IRQn` = **0** (cao nhất trên Cortex-M4).

FreeRTOS dùng `BASEPRI` register để mask các interrupt có priority **số nhỏ hơn hoặc bằng**
`configMAX_SYSCALL_INTERRUPT_PRIORITY` (= 5 trong project này). Khi scheduler đang chạy,
tất cả ISR phải có priority **số >= 5** mới được phép gọi FreeRTOS API (`xQueueSendFromISR`).

```
CubeMX set:  EXTI0 priority = 0   ← cao hơn FreeRTOS syscall limit
Required:    EXTI0 priority >= 5  ← vì ISR gọi xQueueSendFromISR
```

Với priority = 0, ISR của EXTI0 chạy khi BASEPRI đang mask FreeRTOS → **undefined behavior**,
thường gây HardFault hoặc data corruption trong kernel.

### Fix

Thêm vào `Core/Src/main.c` trong `USER CODE BEGIN 2`:

```c
/* USER CODE BEGIN 2 */
/* Override EXTI0 priority — CubeMX sets 0 (above FreeRTOS syscall limit).
 * ISRs calling xQueueSendFromISR must have priority >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5). */
HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
```

`USER CODE BEGIN 2` nằm sau `MX_GPIO_Init()` (nơi CubeMX set priority 0) và sau
`HAL_Init()`, nên override này luôn có tác dụng và **tồn tại qua CubeMX regeneration**.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Cortex-M4 priority | Số **nhỏ hơn** = ưu tiên **cao hơn**. Priority 0 là cao nhất |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | Ngưỡng tối thiểu để ISR gọi FreeRTOS API |
| `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` | Giá trị dạng 0–15 (trước khi shift); trong project = 5 |
| Rule | Mọi ISR gọi `xXxx_FromISR()` phải có priority **số >= 5** (tức là thấp hơn hoặc bằng mức syscall) |

---

## Bug #2 — I2C Bus Stuck (GPIO Pull-up Missing)

### Triệu chứng
OLED hoàn toàn không phản hồi. Chương trình treo tại `HAL_I2C_Mem_Write(... HAL_MAX_DELAY)`.
Logic analyzer thấy SDA/SCL luôn ở mức LOW.

### Root Cause

I2C là giao thức **open-drain**: cả master lẫn slave chỉ kéo bus xuống LOW, không bao giờ
kéo lên HIGH. Bus cần **pull-up resistor bên ngoài** để trở về HIGH sau mỗi bit.

CubeMX generate code với `GPIO_NOPULL` cho PB8 (SCL) và PB9 (SDA):

```c
GPIO_InitStruct.Pull = GPIO_NOPULL;  // ← CubeMX default
```

Không có pull-up → bus không bao giờ lên HIGH → I2C không giao tiếp được →
`HAL_I2C_Mem_Write` với `HAL_MAX_DELAY` = treo vô hạn.

### Fix

Thêm vào `Core/Src/stm32f4xx_hal_msp.c` trong `USER CODE BEGIN I2C1_MspInit 1`:

```c
/* USER CODE BEGIN I2C1_MspInit 1 */
/* Enable internal pull-ups on SCL/SDA.
 * CubeMX sets GPIO_NOPULL; I2C open-drain bus cannot go HIGH without pull-ups.
 * Internal ~40 kΩ is sufficient for testing at 100 kHz on short wires.
 * Replace with external 4.7 kΩ for production. */
GPIO_InitStruct.Pin  = GPIO_PIN_8 | GPIO_PIN_9;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
/* USER CODE END I2C1_MspInit 1 */
```

`USER CODE BEGIN I2C1_MspInit 1` nằm SAU dòng `HAL_GPIO_Init` của CubeMX → override
`GPIO_NOPULL` thành `GPIO_PULLUP` và tồn tại qua regeneration.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Open-drain | Output chỉ có 2 trạng thái: kéo xuống GND, hoặc thả nổi (high-impedance) |
| Pull-up resistor | Khi tất cả devices thả nổi, điện trở kéo bus lên VCC → logic HIGH |
| External 4.7 kΩ | Chuẩn cho I2C 100 kHz, tốt hơn internal pull-up (~40 kΩ) cho production |
| `HAL_MAX_DELAY` | = `0xFFFFFFFF` ≈ 49 ngày → khi I2C lỗi, hàm treo vĩnh viễn |
| USER CODE section | Luôn đặt fix trong USER CODE section để sống sót qua CubeMX regeneration |

---

## Bug #3 — FreeRTOS Handler Installation Check Assertion

### Triệu chứng
Chương trình treo trong vòng lặp vô hạn tại dòng `configASSERT()` trong `port.c:342`.
Nhìn vào disassembly thấy chương trình spin tại:
```c
configASSERT( pxVectorTable[ portVECTOR_INDEX_SVC ] == vPortSVCHandler );
```

### Root Cause

FreeRTOS 202112+ có tính năng `configCHECK_HANDLER_INSTALLATION`: khi khởi động, kernel
kiểm tra xem `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler` trong **vector table**
có đúng là các hàm của FreeRTOS hay không.

Project đặt FreeRTOS handlers theo tên custom (`vFreeRTOS_SVC_Handler`) và dùng wrapper
trong `stm32f4xx_it.c`:
```c
void SVC_Handler(void) {
    vFreeRTOS_SVC_Handler();  // wrapper call
}
```

Vector table có `SVC_Handler` (wrapper của CubeMX), nhưng FreeRTOS kiểm tra `vPortSVCHandler`
(= `vFreeRTOS_SVC_Handler`) → **khác nhau** → assertion fail.

### Fix (tạm thời — sẽ bị thay thế bởi Bug #4 fix)

Thêm vào `FreeRTOSConfig.h`:
```c
#define configCHECK_HANDLER_INSTALLATION  0
```

### Bài học tạm thời

Disable check chỉ là workaround — vấn đề thực sự nằm ở Bug #4 bên dưới.

---

## Bug #4 — HardFault Tại Context Switch Đầu Tiên (Root Cause)

### Triệu chứng
Sau khi disable `configCHECK_HANDLER_INSTALLATION`, chương trình không còn treo tại assert
nhưng lại bị **HardFault** ngay khi context switch lần đầu xảy ra (thường < 1ms sau khi
`vTaskStartScheduler()` chạy).

### Root Cause — Phân tích sâu

**Vấn đề cốt lõi: Wrapper approach phá hủy `EXC_RETURN` trong `LR`**

Khi một interrupt xảy ra trên Cortex-M4, hardware tự động:
1. Push exception frame (R0–R3, R12, LR, PC, xPSR) lên stack
2. Set `LR = EXC_RETURN` (ví dụ `0xFFFFFFFD` = return to thread mode dùng PSP)

`EXC_RETURN` là giá trị đặc biệt mà `BX LR` dùng để **thoát khỏi exception** và quay
về thread mode. Không phải là địa chỉ code bình thường.

**PendSV (context switch) bị lỗi như thế nào:**

```
PendSV exception fires
  → hardware sets LR = EXC_RETURN (0xFFFFFFFD)

PendSV_Handler (C function, CubeMX wrapper):
  PUSH {R7, LR}          ← compiler prologue: saves EXC_RETURN on stack
  BL vFreeRTOS_PendSV_Handler   ← BL clobbers LR = return address in C code!
  (POP {R7, PC} — never reached)

vFreeRTOS_PendSV_Handler (naked asm):
  MRS R0, PSP
  STMDB R0!, {R4-R11, R14}   ← saves R14 (= C return addr, NOT EXC_RETURN!) to task stack
  ...
```

Khi task đó được schedule lại:
```
LDMIA R0!, {R4-R11, R14}   ← restores saved R14 = C return address
BX R14                      ← nhảy vào giữa C code của PendSV_Handler → HardFault!
```

**Tại sao SVC_Handler thì OK?**

`vPortSVCHandler` (naked) load `R14` từ **TCB của task đầu tiên** (không phải từ LR hiện tại):
```asm
LDR R0, [R1]                ← R0 = pxTopOfStack của task đầu tiên
LDMIA R0!, {R4-R11, R14}   ← R14 được load từ task stack (có EXC_RETURN đúng)
BX R14                      ← return to thread mode → OK
```
SVC handler không save context của task hiện tại (chỉ restore task đầu tiên), nên không bị lỗi.

### Fix — Đúng và đầy đủ

**Nguyên lý:** FreeRTOS `port.c` phải cung cấp handlers trực tiếp vào vector table
(không qua wrapper). Các handlers phải là **naked assembly** để không có prologue/epilogue
làm thay đổi LR.

**Bước 1 — `FreeRTOSConfig.h`: revert về tên chuẩn**

```c
/* port.c sẽ define SVC_Handler, PendSV_Handler, SysTick_Handler trực tiếp */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
/* Bỏ #define configCHECK_HANDLER_INSTALLATION 0 — check sẽ pass với fix này */
```

**Bước 2 — `stm32f4xx_it.c` `USER CODE BEGIN 0`: forward-declare là weak**

```c
/* USER CODE BEGIN 0 */
/* GCC propagates weak attribute from declaration to definition in same TU.
 * port.c provides strong naked-asm versions; linker discards these weak stubs. */
void SVC_Handler(void)     __attribute__((weak));
void PendSV_Handler(void)  __attribute__((weak));
void SysTick_Handler(void) __attribute__((weak));
/* USER CODE END 0 */
```

**Kết quả sau fix:**

```
Linker symbol resolution:
  port.o:          SVC_Handler    (T = strong, naked asm)  ← USED
  stm32f4xx_it.o:  SVC_Handler    (W = weak, C stub)       ← discarded
  startup.s:       SVC_Handler    (W = weak, alias to Default_Handler) ← discarded
```

Vector table chứa port.c's `SVC_Handler` (naked assembly) → `configCHECK_HANDLER_INSTALLATION`
check pass → context switch đúng → no HardFault.

### Verification (dùng arm-none-eabi-nm)

```
arm-none-eabi-nm stm32f4xx_it.o | grep -E "SVC|PendSV|SysTick"
0000003a W PendSV_Handler     ← W = weak ✓
0000001e W SVC_Handler        ← W = weak ✓
00000048 W SysTick_Handler    ← W = weak ✓

arm-none-eabi-nm port.o | grep -E "SVC|PendSV|SysTick"
000003c8 T PendSV_Handler     ← T = strong ✓
000000c0 T SVC_Handler        ← T = strong ✓
0000042a T SysTick_Handler    ← T = strong ✓
```

### Bài học

| Khái niệm | Giải thích |
|---|---|
| EXC_RETURN | Giá trị đặc biệt trong LR khi exception entry; `BX LR` với giá trị này → exception return |
| `BL` instruction | Branch with Link: set `LR = next instruction`. **Phá hủy EXC_RETURN!** |
| Naked function | `__attribute__((naked))`: không có prologue/epilogue → không thay đổi LR |
| Weak symbol | `__attribute__((weak))`: linker dùng strong version nếu có; bỏ qua weak |
| `#pragma GCC weak` | **Không làm definition thành weak!** Chỉ `__attribute__((weak))` trên declaration/definition mới work |
| Forward declaration | `void foo(void) __attribute__((weak));` trước definition trong cùng TU → GCC propagate weak |

**Rule vàng:** Khi integrate FreeRTOS thủ công với CubeMX, luôn đặt `__attribute__((weak))`
trên forward declaration của SVC/PendSV/SysTick trong `USER CODE BEGIN 0`.

---

## Bug #5 — OLED Cursor Không Di Chuyển Khi Nhấn Nút

### Triệu chứng
Nhấn nút ngắn nhiều lần, màn hình OLED không thay đổi. Cursor "> Wash" luôn hiển thị
ở vị trí cố định, không chuyển sang "Spin".

### Root Cause

Có hai vấn đề kết hợp:

**1. UITask không gửi display message khi menuIndex thay đổi:**

```c
// UITask trước fix:
if (event == BTN_EVENT_NAVIGATE)
{
    menuIndex = (menuIndex + 1) % MENU_ITEM_COUNT;
    // ← KHÔNG gửi gì cho OLEDTask → OLED không update
}
```

UITask thay đổi `menuIndex` internally nhưng không notify OLEDTask. OLED chỉ cập nhật
khi WashingManager gửi display message (xảy ra khi state machine chuyển state).

**2. WashingManager hardcode `menuIndex = 0`:**

```c
// washing_manager.c trước fix:
static void Manager_PostDisplay(WashState_t state, const char *statusText)
{
    DisplayMsg_t msg;
    msg.menuIndex = 0u;  // ← LUÔN hardcode, không biết cursor đang ở đâu
    ...
}
```

Ngay cả khi WashingManager gửi display message, cursor luôn về "Wash" (index 0).

### Fix

**`app_msg.h`** — thêm `menuIndex` vào `CommandMsg_t`:
```c
typedef struct {
    Command_t  command;
    WashMode_t mode;
    uint8_t    menuIndex;  // NEW: UITask truyền cursor position theo command
} CommandMsg_t;
```

**`ui_task.c`** — gửi display update ngay khi cursor thay đổi:
```c
if (event == BTN_EVENT_NAVIGATE)
{
    menuIndex = (menuIndex + 1) % MENU_ITEM_COUNT;

    DisplayMsg_t disp;
    disp.menuIndex = menuIndex;
    strncpy(disp.statusText, s_statusCache, sizeof(disp.statusText) - 1u);
    xQueueSend(xDisplayQueue, &disp, 0);  // OLED cập nhật ngay lập tức
}
// Long press: kèm menuIndex theo command
cmd.menuIndex = menuIndex;
xQueueSend(xCommandQueue, &cmd, 0);
```

**`washing_manager.c`** — lưu menuIndex từ command:
```c
static uint8_t s_menuIndex = 0u;

static void Manager_HandleCommand(const CommandMsg_t *cmd)
{
    s_menuIndex = cmd->menuIndex;  // lưu lại để dùng trong display messages
    ...
}
// Trong Manager_PostDisplay:
msg.menuIndex = s_menuIndex;  // thay vì hardcode 0
```

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Data ownership | `menuIndex` thuộc về UITask; WashingManager không được tự đặt giá trị này |
| Message design | Khi một task cần data từ task khác, phải truyền qua message — không dùng shared global |
| Immediate feedback | UI update phải xảy ra **ngay khi input nhận được**, không chờ state machine thay đổi |

---

## Bug #6 — Button "No Response" (Long Press Threshold Quá Cao)

### Triệu chứng
Nhấn giữ nút 1–2 giây, không có phản hồi. Máy giặt không bắt đầu chu trình.

### Root Cause

```c
// app_config.h trước fix:
#define BUTTON_LONG_PRESS_MS  3000U  // ← cần giữ 3 GIÂY
```

User phải giữ nút **3 giây liên tục** mới trigger long press. Trong khi đó, button debounce
còn phân loại: < 50ms = noise (bỏ), 50ms–3000ms = short press (chỉ chuyển cursor, không
khởi động máy). User giữ 1–2 giây → short press → cursor thay đổi → nhưng vì Bug #5 nên
OLED không update → user thấy hoàn toàn "không phản hồi".

### Fix

```c
// app_config.h sau fix:
#define BUTTON_LONG_PRESS_MS  1500U  // giữ 1.5 giây để start
```

### Bài học

| Khái niệm | Giải thích |
|---|---|
| UX threshold | Long press > 1 giây đã bắt đầu cảm giác "chậm"; > 2 giây = "không phản hồi" |
| Debug strategy | Kiểm tra timing constants sớm; nhiều "hardware bug" thực ra là threshold sai |

---

## Bug #7 — Garbage Pixels Ở Phía Dưới OLED

### Triệu chứng
Phần dưới màn hình OLED hiển thị các điểm ảnh ngẫu nhiên, không phải nội dung của app.

### Root Cause

`ssd1306_Init()` khởi động display controller nhưng **không clear pixel RAM** của SSD1306.
RAM của OLED sau power-on có giá trị ngẫu nhiên. Code app chỉ ghi vào 4 dòng đầu (32 pixels),
32 pixels còn lại (dòng 4–7) giữ nguyên giá trị ngẫu nhiên từ lúc power-on.

### Fix

```c
// bsp_oled.c
void BSP_OLED_Init(void)
{
    ssd1306_Init();
    /* Clear display RAM — SSD1306 power-on state is undefined. */
    ssd1306_Fill(Black);      // fill software framebuffer với black
    ssd1306_UpdateScreen();   // gửi toàn bộ framebuffer (128×64 pixels) lên display
}
```

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Display RAM | OLED controller có RAM nội bộ lưu pixel data. Power-on state = undefined |
| Defensive init | Nhớ clear display sau `Init()` trước khi vẽ nội dung thực |
| `ssd1306_UpdateScreen()` | Truyền toàn bộ 128×64 = 1024 bytes qua I2C; tốn thời gian nhưng cần thiết |

---

## Bug #8 — Door Sensor Dùng Chung Chân PA0 Với Button (Race Condition)

### Triệu chứng
Bấm giữ B1 để chọn "Wash" (long press ≥1.5s), LED cam (inlet) và LED xanh dương
(status) sáng lên đúng như vào `FILL_WATER` — nhưng ngay sau khi thả tay ra,
máy lập tức nhảy sang `ERROR`, mọi LED tắt.

### Root Cause

`bsp_sensor.c` mô phỏng cảm biến cửa bằng cách đọc lại **cùng chân PA0** mà
`bsp_button.c` dùng cho B1 User Button (comment gốc: *"Button held = door
closed"*):

```c
#define DOOR_PORT   GPIOA
#define DOOR_PIN    GPIO_PIN_0   // ← trùng chân với B1

uint8_t BSP_Sensor_IsDoorClosed(void)
{
    return (HAL_GPIO_ReadPin(DOOR_PORT, DOOR_PIN) == GPIO_PIN_SET) ? 1u : 0u;
}
```

Cơ chế đo long-press trong `BSP_Button_IRQHandler()` bắt buộc phải chờ **cạnh
xuống** (thả tay) mới tính được duration và bắn `BTN_EVENT_CONFIRM`. Nghĩa
là hành động thả tay — điều kiện bắt buộc để xác nhận lệnh — đồng thời cũng
khiến PA0 về LOW, và `SensorTask` (poll mỗi 100ms) đọc đó là "cửa vừa mở":

```
User giữ B1 ≥1.5s → thả tay
  → PA0 = LOW
  → BSP_Button_IRQHandler(): duration ≥ 1500ms → BTN_EVENT_CONFIRM
      → UITask → CMD_START_WASH → SM: IDLE → FILL_WATER
  → (≤100ms sau) SensorTask: BSP_Sensor_IsDoorClosed() == LOW
      → xEventGroupSetBits(EVT_ERROR)
      → SM: FILL_WATER → ERROR
```

Hai vai trò khác bản chất (input điều khiển UI cần "thả để xác nhận", cảm
biến an toàn cần "thả nghĩa là nguy hiểm") bị gán chung một chân vật lý, tạo
ra race condition gần như luôn xảy ra với thao tác tay người.

### Fix

Tách "cửa" thành biến software thuần túy, giống cách `s_waterFull` đã mô
phỏng mực nước — không đọc GPIO nữa:

```c
// bsp_sensor.c sau fix
static volatile uint8_t s_doorClosed = 1u;   /* mặc định: đóng (an toàn) */

uint8_t BSP_Sensor_IsDoorClosed(void)
{
    return s_doorClosed;
}
```

PA0 giờ chỉ còn đúng một chủ sở hữu: `bsp_button.c`. Muốn test tình huống mở
cửa, set `s_doorClosed = 0` qua debugger (Live Expressions) trong lúc máy
đang chạy.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Shared-pin race condition | Hai module đọc/ghi cùng chân GPIO cho hai mục đích khác bản chất (điều khiển vs an toàn) → hành vi cần thiết của module này vô tình kích hoạt module kia |
| Input điều khiển vs cảm biến an toàn | Luôn tách riêng về mặt vật lý (hoặc mô phỏng riêng) — không bao giờ dùng chung một tín hiệu |
| Simulation qua debugger | Khi chưa có cảm biến thật, biến static + debugger write là cách mô phỏng an toàn, nhất quán (đã áp dụng sẵn cho nước, giờ áp dụng luôn cho cửa) |

---

## Bug #9 — OLED Hiển Thị Sai Status Khi Bấm Nút Giữa Chu Trình (ĐÃ FIX 2026-10-10)

### Triệu chứng
Nếu short-press B1 trong lúc máy đang chạy (`FILL_WATER`/`WASHING`/`DRAIN`/`SPIN`),
dòng status cuối OLED tạm thời nhảy về `IDLE` sai lệch, dù máy vẫn đang chạy
đúng state thực tế.

### Root Cause

`UITask` không biết `WashingManager` đang ở state nào — nó xử lý mọi
`BTN_EVENT_NAVIGATE` như đang ở màn hình chờ, và tự gửi một `DisplayMsg_t`
đè lên màn hình dùng `s_statusCache`:

```c
// ui_task.c
static char s_statusCache[24] = "IDLE";   // chỉ khởi tạo 1 lần, không nơi nào update lại

if (event == BTN_EVENT_NAVIGATE)
{
    ...
    strncpy(disp.statusText, s_statusCache, sizeof(disp.statusText) - 1u);
    xQueueSend(xDisplayQueue, &disp, 0);   // luôn gửi "IDLE"
}
```

### Trạng thái

**Đã fix (2026-10-10), chưa kiểm chứng trên board.** Nguyên nhân gốc là con trỏ
menu có **hai chủ** (UITask và WashingManager) và UITask còn là nhà sản xuất thứ hai
của `DisplayMsg_t`. Hướng đã chọn khác cả hai phương án (a)/(b) ở bản ghi gốc:
- `WashingManager` giữ con trỏ (`s_menuIndex`) và là bên **duy nhất** dựng
  `DisplayMsg_t` (`Manager_PostDisplay()`), chữ trạng thái luôn đúng.
- `UITask` thành bộ dịch không trạng thái: `BTN_EVENT_NAVIGATE → CMD_NAVIGATE`,
  `BTN_EVENT_CONFIRM → CMD_CONFIRM`; `s_statusCache` bị xoá.
- Hệ quả có chủ ý: lúc đang chạy chu trình, nút bị bỏ qua (manager chỉ đọc
  `xCommandQueue` ở IDLE/FINISH/ERROR).
Phân tích đầy đủ: START_HERE Tầng 3 §3.6, LEARNING_NOTES Phần D, Session 16. Các đoạn mã trong mục này
là bản **cũ** (`s_statusCache`, `CMD_START_WASH/SPIN`) — giữ lại làm lịch sử.

---

## Bug #10 — Không Thể Thoát FINISH/ERROR: UITask Không Bao Giờ Gửi CMD_RESET

### Triệu chứng
Chạy hết một chu trình Wash (set `s_waterFull = 1u` qua debugger để qua
`FILL_WATER`) → máy vào `FINISHED` đúng như mong đợi. Nhưng sau đó bấm long
press để chọn lại Wash hoặc Spin thì **hệ thống không phản hồi gì cả** — màn
hình đứng yên ở `FINISHED`, LED không đổi, bấm bao nhiêu lần cũng vậy.

### Root Cause

Đường thoát duy nhất khỏi `FINISH` (và `ERROR`) trong transition table là:

```c
// washing_sm.c
{ WASH_STATE_FINISH, WASH_EVT_RESET, WASH_STATE_IDLE },
{ WASH_STATE_ERROR,  WASH_EVT_RESET, WASH_STATE_IDLE },
```

`WASH_EVT_RESET` chỉ được tạo khi `WashingManager` nhận `CMD_RESET`
([washing_manager.c:170-172](../App/WashingManager/Src/washing_manager.c#L170-L172)).
Nhưng `UITask`'s long-press handler không có nhánh nào gửi `CMD_RESET` — nó
luôn build `CMD_START_WASH` hoặc `CMD_START_SPIN` tùy `menuIndex`
([ui_task.c:73-90](../App/UITask/Src/ui_task.c#L73-L90)):

```c
else if (event == BTN_EVENT_CONFIRM)
{
    CommandMsg_t cmd;
    cmd.menuIndex = menuIndex;
    if (menuIndex == 0u) { cmd.command = CMD_START_WASH; ... }
    else                 { cmd.command = CMD_START_SPIN; ... }
    xQueueSend(xCommandQueue, &cmd, 0);
    // không có branch nào gửi CMD_RESET
}
```

Khi long-press tới `WashingManager` lúc đang ở `FINISH`, nó gọi
`WashingSM_ProcessEvent(WASH_EVT_START_WASH)`, nhưng bảng chuyển trạng thái
không có row `(FINISH, START_WASH)` → sự kiện bị **âm thầm bỏ qua** đúng như
spec: *"Any (state, event) pair not listed above is silently ignored"*
([washing_sm.h:59](../App/WashingManager/Inc/washing_sm.h#L59)). Không phải
crash hay treo — SM chỉ đơn giản đứng yên vì luôn nhận sai loại sự kiện.

### Trạng thái

**Đã fix bằng Phương án A** (`Manager_HandleCommand()` diễn giải lệnh theo state; ở `FINISH`/`ERROR` mọi lệnh thành `WASH_EVT_RESET`). Từ 2026-10-10 chỉ còn một lệnh xác nhận `CMD_CONFIRM`, nên các tên `CMD_START_*` bên dưới là lịch sử. Hai phương án đã thảo luận:

- **Phương án A (khuyến nghị):** `Manager_HandleCommand()` tự kiểm tra
  `WashingSM_GetState()` trước khi switch theo `cmd->command` — nếu đang ở
  `FINISH`/`ERROR`, mọi lệnh đến (bất kể `CMD_START_WASH` hay
  `CMD_START_SPIN`) đều được diễn giải là `WASH_EVT_RESET`. Không cần thêm
  IPC mới, đúng nguyên tắc "WashingManager là chủ sở hữu duy nhất của state
  machine" (Design Rule #1).
- **Phương án B:** Thêm kênh phản hồi ngược để `UITask` biết state hiện tại
  và tự gửi `CMD_RESET` đúng lúc — phức tạp hơn, phá vỡ việc UITask vốn
  "mù" hoàn toàn về state machine từ đầu dự án.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Silent event ignore | Table-driven SM bỏ qua event không khớp state hiện tại — an toàn (không crash) nhưng dễ tạo cảm giác "hệ thống treo" nếu không có đường thoát cho mọi state cuối |
| Terminal state cần đường thoát tường minh | Mọi state không phải IDLE đều phải có ít nhất 1 transition dẫn trở lại — thiết kế transition table xong cần rà lại xem có "state cụt" nào không |
| Ai nên diễn giải command theo context | Task sở hữu state (ở đây là WashingManager) nên là nơi diễn giải ý nghĩa command theo state hiện tại, thay vì bắt task gửi command (UITask) phải biết trước state của task nhận |

---

## Bug #11 — PE1 Start/Confirm Chết Hoàn Toàn (Quên Gọi IRQ Handler)

### Triệu chứng
Sau khi refactor `bsp_button.c`/`.h` để chuyển toàn bộ GPIO+NVIC init từ code
thủ công sang CubeMX-generated (`MX_GPIO_Init()`), nhấn nút Start/Confirm
(PE1) không có phản ứng gì — không chuyển menu, không start chu trình.

### Root Cause

Refactor thêm hàm mới `BSP_StartButton_IRQHandler()` trong `bsp_button.c` và
khai báo trong `bsp_button.h`, nhưng **quên nối nó vào ISR thực tế**. So sánh
2 handler sinh ra bởi CubeMX trong `stm32f4xx_it.c`:

```c
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */
  BSP_Button_IRQHandler();          // ← PA0: có gọi
  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  ...
}

void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */
                                     // ← PE1: TRỐNG, thiếu lời gọi
  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  ...
}
```

`BSP_StartButton_IRQHandler()` tồn tại trong object file nhưng không có
caller nào trong toàn bộ firmware → dead code, và EXTI1 interrupt chỉ đơn
thuần clear pending bit (`HAL_GPIO_EXTI_IRQHandler`) rồi thoát, không bao giờ
đẩy `BTN_EVENT_CONFIRM` vào `xButtonQueue`.

### Fix

Thêm đúng 1 dòng vào `USER CODE BEGIN EXTI1_IRQn 0` trong
[stm32f4xx_it.c](../Core/Src/stm32f4xx_it.c#L228):

```c
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */
  BSP_StartButton_IRQHandler();
  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  ...
}
```

Đặt trong `USER CODE` section nên sống sót qua CubeMX regeneration.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Symmetry check | Khi 2 ISR làm cùng một việc (2 nút bấm cùng pattern), luôn diff 2 handler với nhau — thiếu sót kiểu "quên 1 dòng" rất dễ lọt qua review nếu chỉ đọc từng file riêng lẻ |
| Dead code là dấu hiệu | Một hàm public được định nghĩa + khai báo trong `.h` nhưng không ai gọi gần như luôn là bug refactor dở dang, không phải "future use" |
| CubeMX refactor rủi ro | Khi chuyển từ init thủ công sang CubeMX-generated, dễ sót bước nối `USER CODE` callback vào handler mới — nên grep tên hàm mới ngay sau khi thêm để chắc chắn có nơi gọi |

---

## Bug #12 — PA0 Menu Navigate Chết (Thiếu Cạnh FALLING Trong CubeMX)

### Triệu chứng
Nhấn/nhả B1 (PA0) không chuyển menu "Wash" ↔ "Spin" trên OLED. Không có
crash, không có LED nào đổi trạng thái — hoàn toàn im lặng.

### Root Cause

`BSP_Button_IRQHandler()` trong [bsp_button.c:40-61](../App/BSP/Src/bsp_button.c#L40-L61)
được thiết kế đo **duration giữa 2 cạnh** để debounce và phân biệt nhiễu:

```c
void BSP_Button_IRQHandler(void) {
  if (HAL_GPIO_ReadPin(...) == GPIO_PIN_SET) {
    s_pressStartMs = HAL_GetTick();   // cạnh lên: bắt đầu đếm
    s_pressed = 1u;
  } else {
    if (s_pressed) {                  // cạnh xuống: mới tính duration & bắn event
      uint32_t duration = HAL_GetTick() - s_pressStartMs;
      ...
      s_callback(BTN_EVENT_NAVIGATE); // ← CHỈ xảy ra ở nhánh else (falling edge)
    }
  }
}
```

Header comment ([bsp_button.h:11](../App/BSP/Inc/bsp_button.h#L11)) ghi rõ
yêu cầu: `EXTI0, both edges (rising = press, falling = release)`. Nhưng GPIO
mode thực tế do CubeMX generate trong `MX_GPIO_Init()`
([main.c:226](../Core/Src/main.c#L226)) chỉ là:

```c
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;   // ← thiếu FALLING
```

vì `Freertos.ioc` không có override nào cho `PA0-WKUP` (không có
`GPIOParameters`/`GPIO_ModeDefaultEXTI`), nên CubeMX dùng mặc định của chân
"WKUP" là chỉ bắt RISING. Kết quả: `EXTI0_IRQHandler` không bao giờ fire khi
thả tay → nhánh `else` (nơi bắn `BTN_EVENT_NAVIGATE`) không bao giờ chạy.

**Bài học quan trọng:** đây là ví dụ điển hình của "comment nói một đằng, config generate ra
một nẻo" — code C và `.ioc` phải luôn đồng bộ thủ công vì CubeMX không đọc
comment để suy ra ý định.

### Fix

**1. `Freertos.ioc`** — thêm override cho `PA0-WKUP` để CubeMX generate đúng
cạnh (fix gốc, sống sót qua regenerate):

```
PA0-WKUP.GPIOParameters=GPIO_ModeDefaultEXTI
PA0-WKUP.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
```

**2. `Core/Src/main.c`** — sửa trực tiếp để có hiệu lực ngay mà không cần mở
STM32CubeIDE bấm "Generate Code" (file này vốn là generated code, nằm ngoài
`USER CODE`, nhưng đồng bộ với bước 1 nên lần regenerate tiếp theo sẽ ra y hệt):

```c
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
```

> Khuyến nghị: mở lại `Freertos.ioc` trong CubeMX/STM32CubeIDE ít nhất 1 lần
> và bấm Generate để xác nhận 2 file thực sự khớp nhau — sửa tay `main.c`
> chỉ là biện pháp tạm để test ngay.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| WKUP pin default | Chân "WKUP" (Wake-Up, PA0 trên board này) mặc định CubeMX chỉ cấu hình RISING vì vai trò gốc là wake-from-standby (chỉ cần 1 cạnh) — dùng làm button 2 cạnh phải override thủ công |
| Config phải đồng bộ 2 chiều | `.ioc` là nguồn sự thật cho generated code; sửa `.c` mà không sửa `.ioc` sẽ mất khi regenerate, sửa `.ioc` mà không regenerate thì `.c` chưa có hiệu lực — cần làm cả 2 |
| Comment không phải test | Header comment mô tả đúng ý định thiết kế (`both edges`) nhưng không có gì enforce nó khớp với `.ioc` — chỉ phát hiện được bằng cách đọc chéo code thực tế, không thể tin comment |

---

## Bug #13 — `.ioc` Và `main.c` Lệch Nhau Về NVIC Priority (Bom Nổ Chậm Khi Regenerate)

### Triệu chứng
Không có triệu chứng nào ngay bây giờ — firmware hiện tại vẫn chạy đúng.
Đây là bug dạng **"config drift"**: chỉ phát tác vào lần tiếp theo ai đó mở
`Freertos.ioc` bằng CubeMX/STM32CubeIDE và bấm "Generate Code".

### Root Cause

`MX_GPIO_Init()` trong [main.c:250-254](../Core/Src/main.c#L250-L254) đang
hardcode priority 5 cho cả EXTI0 và EXTI1 — **đúng** theo rule của project
(ISR gọi `xQueueSendFromISR()` phải có priority số ≥
`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` = 5, xem lại Bug #1):

```c
HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
```

Nhưng 2 dòng này nằm **ngoài** khối `USER CODE` — tức là bản thân chúng
cũng do CubeMX generate ra, không phải override thủ công như Bug #1 (Bug #1
đặt override trong `USER CODE BEGIN 2`, sau `MX_GPIO_Init()`, nên luôn thắng).
Giá trị "5" ở đây chỉ tồn tại vì `Freertos.ioc` trước khi sửa đang khai báo
đúng như vậy:

```
NVIC.EXTI0_IRQn=true:0:0:...   ← preempt priority = 0, KHÔNG PHẢI 5
NVIC.EXTI1_IRQn=true:0:0:...
```

Nói cách khác: `.ioc` (nguồn cấu hình) nói priority = 0, còn `.c` (code
generate lần gần nhất) lại đang có priority = 5 — hai file đã lệch nhau,
rất có thể do ai đó từng sửa tay `main.c` để test mà quên đồng bộ ngược lại
`.ioc`. Lần tới regenerate, CubeMX đọc `.ioc` và ghi đè `main.c` về đúng
những gì `.ioc` khai báo → priority tụt về 0, cao hơn (ưu tiên hơn) ngưỡng
FreeRTOS syscall mask → tái hiện chính xác Bug #1 (crash/undefined behaviour
khi `xQueueSendFromISR()` chạy ở priority không được BASEPRI che chắn), lần
này không có gì cảnh báo trước vì code "trông như đã đúng".

### Fix

Sửa `Freertos.ioc` để khớp với giá trị đã đúng trong `main.c`, thay vì sửa
ngược lại (main.c đang đúng theo rule dự án):

```
NVIC.EXTI0_IRQn=true:5:0:false:false:true:true:true:true
NVIC.EXTI1_IRQn=true:5:0:false:false:true:true:true:true
```

Không cần sửa `main.c` vì giá trị ở đó đã đúng — chỉ cần `.ioc` bắt kịp thực
tế. Sau fix, quan hệ nguồn-sự-thật (`.ioc` → generate → `.c`) trở nên
consistent trở lại: regenerate bao nhiêu lần cũng ra priority 5.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Config drift | `.ioc` là single source of truth cho phần generated code; nếu sửa tay phần generated (ngoài `USER CODE`) mà không sửa `.ioc` theo, 2 nguồn sẽ lệch nhau và im lặng cho đến lần regenerate kế tiếp |
| USER CODE vs generated-nhưng-đúng | Có 2 cách để một giá trị "đúng" tồn tại trong generated code: (1) override trong `USER CODE` — bền vững qua regenerate; (2) trùng hợp đúng với `.ioc` hiện tại — chỉ bền vững cho tới khi `.ioc` đổi hoặc drift. Bug #1 dùng cách (1), nên vẫn an toàn dù `.ioc` sai. Bug #13 là hệ quả của việc dựa vào cách (2) mà không tự nhận ra |
| Review checklist cho generated code | Khi thấy một giá trị "đúng" nằm ngoài `USER CODE` block, luôn hỏi ngược: "`.ioc` có thực sự khai báo giá trị này không, hay nó chỉ đang trùng hợp còn sót lại từ lần sửa tay trước?" |

---

## Bug #14 — PE1 Xung Đột Với MEMS_INT2 Của Cảm Biến Gia Tốc Tích Hợp

### Triệu chứng

Đấu nút nhấn ngoài vào PE1 đúng như hướng dẫn (PE1 ── [Nút] ── GND, pull-up
nội đã bật trong `.ioc`), nhưng **không nút nào chọn được chức năng** — như
thể nút hoàn toàn không hoạt động. PA0 (Navigate) vẫn hoạt động bình thường,
loại trừ khả năng firmware chưa build/flash lại.

Đo trực tiếp bằng logic analyzer: **PE1 luôn ở mức LOW**, bất kể cấu hình
pull-up hay pull-down trong `.ioc`, bất kể có nhấn nút hay không.

### Root Cause

Việc mức tín hiệu không đổi dù đảo pull nội (~40 kΩ) chỉ có một cách giải
thích: có một nguồn khác đang chủ động ghì PE1 xuống GND với trở kháng thấp
hơn nhiều so với điện trở pull nội — tức là PE1 **không phải một GPIO tự do**.

Tra schematic chính thức của STM32F411E-DISCO (board `mb1115-f411ve-b02-b03`)
xác nhận: PE1 được đấu cứng trên board với chân **MEMS_INT2** — ngắt số 2 của
cảm biến gia tốc **LIS3DSH** gắn sẵn trên kit.

```
LIS3DSH (accelerometer, on-board)
    INT2 (output) ──── PE1 (MCU)
                          │
                          ├── điện trở pull nội của MCU (yếu, ~40kΩ)
                          └── [Nút nhấn ngoài của bạn] ── GND
```

INT2 là một **output đã có driver tích cực** từ phía LIS3DSH (không phải
input thả nổi), nên khi ở trạng thái driving-low nó thắng áp đảo điện trở
pull nội của MCU dù cấu hình pull-up hay pull-down — PE1 luôn đọc LOW.

Đây là bug **cùng lớp** với hiện tượng PD5 làm LED LD8 sáng khi nhấn (PD5 =
`OTG_FS_OverCurrent`, cũng là một chân bị tái sử dụng cho ngoại vi tích hợp
trên board): **STM32F411E-DISCO không có nghĩa mọi chân trên header đều là
GPIO tự do** — nhiều chân đã bị chiếm dụng bởi cảm biến/IC gắn sẵn trên board,
và không cách nào giải phóng chỉ bằng cấu hình firmware.

### Fix

Không thể sửa bằng software — phải **đổi chân vật lý** cho nút Start/Confirm
sang một GPIO thực sự tự do. Đã chọn **PD4**: chân này không nằm trên bất kỳ
đường tín hiệu nào của LIS3DSH/CS43L22 audio DAC/USB OTG, và (khác PE1) nó là
một *output do MCU điều khiển vào ngoại vi khác* trong thiết kế gốc của board
(dây tới chân RESET của CS43L22 audio DAC) chứ không phải input nhận tín hiệu
ngược lại từ IC khác — vì project này không dùng audio nên chân này ở trạng
thái mặc định (input thả nổi) và hoạt động như một GPIO trống thông thường.

> ⚠️ **Cập nhật:** kết luận "PD4 an toàn" ở trên dựa trên schematic của board
> STM32F4-Discovery đời **407**, chưa được xác nhận trực tiếp với đúng board
> 411E (MB1115) đang dùng ở đây. Trên thực tế PD4 **cũng đo được LOW cố
> định** giống PE1 — xem
> [Bug #16](#bug-16--pd4-cũng-đọc-low-cố-định-nguyên-nhân-chưa-xác-nhận).
> Đừng coi phần "Fix" dưới đây là điểm dừng cuối cùng của vấn đề.

Thay đổi trên toàn bộ chain:

- `Freertos.ioc`: `PE1.*` → `PD4.*`, `NVIC.EXTI1_IRQn` → `NVIC.EXTI4_IRQn`
- `Core/Src/main.c`: `MX_GPIO_Init()` cấu hình `GPIO_PIN_4` trên `GPIOD` thay
  vì `GPIO_PIN_1` trên `GPIOE`; xoá `__HAL_RCC_GPIOE_CLK_ENABLE()` (không còn
  pin nào dùng GPIOE)
- `Core/Src/stm32f4xx_it.c` / `.h`: `EXTI1_IRQHandler` → `EXTI4_IRQHandler`
- `App/BSP/Inc/bsp_button.h`, `App/BSP/Src/bsp_button.c`: cập nhật comment
  (không đổi tên hàm — `BSP_StartButton_IRQHandler()` giữ nguyên, chỉ đổi
  chân vật lý nó phục vụ)

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Signal direction quyết định rủi ro xung đột | Input nhận tín hiệu từ IC khác (PE1 ← LIS3DSH INT2) rủi ro cao hơn nhiều so với output MCU điều khiển ra IC khác (PD4 → CS43L22 RESET) khi tái sử dụng cho mục đích khác — vì chiều đầu tiên có driver bên ngoài chủ động chống lại pull nội |
| "Discovery board" ≠ mọi chân đều tự do | Board evaluation luôn tích hợp sẵn nhiều ngoại vi (cảm biến, audio, USB) dùng chung silicon với header mở rộng — trước khi chọn một chân bất kỳ cho input/output tự chế, phải đối chiếu schematic, không chỉ đối chiếu pinout MCU thuần túy |
| Đảo pull-up/pull-down để chẩn đoán | Nếu mức tín hiệu không đổi dù đảo hướng điện trở pull nội, gần như chắc chắn có driver bên ngoài áp đảo — đây là phép thử nhanh, không cần thiết bị đo phức tạp, để phân biệt "hở mạch/dây lỏng" và "bị chiếm dụng/short" |
| Logic analyzer là công cụ chẩn đoán đầu tiên, không phải cuối cùng | Xác nhận được tín hiệu "luôn LOW" chỉ cho biết triệu chứng; phải tra schematic mới biết được **nguyên nhân vật lý** đằng sau |

---

## Bug #15 — Generate Code Cho PD4 Làm Hồi Sinh Bug #12/#13 Trên PA0

### Triệu chứng

Sau khi đổi chân Start/Confirm từ PE1 sang PD4 bằng STM32CubeIDE GUI (Pinout
& Configuration) và để IDE tự "Generate Code", kiểm tra lại `main.c` thì phát
hiện — dù không ai chủ ý sửa PA0:

```c
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;   // ← mất FALLING, y hệt Bug #12
...
HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);       // ← mất priority 5, y hệt Bug #13
HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);       // ← EXTI4 (PD4) cũng bị priority 0
```

Đồng thời, khối `EXTI4_IRQHandler()` mới được CubeMX sinh ra trong
`stm32f4xx_it.c` **rỗng** — không có lời gọi `BSP_StartButton_IRQHandler()` —
y hệt Bug #11 nhưng lần này xảy ra ngay từ lần đầu chân PD4 tồn tại.

### Root Cause

Hai cơ chế khác nhau cộng hưởng:

**1. `.ioc` trên đĩa chưa từng có override cho PA0/NVIC priority.** Fix của
Bug #12/#13 trong phiên làm việc trước chỉ áp cho `main.c`/`.ioc` tại thời
điểm đó, nhưng giữa 2 phiên, `Freertos.ioc` đã bị mất 2 dòng override
(`PA0-WKUP.GPIO_ModeDefaultEXTI=...`, `NVIC.EXTI0/1_IRQn` priority `5`) —
firmware vẫn build và chạy đúng nên không ai phát hiện config đã drift.
Khi CubeMX generate lại toàn bộ `MX_GPIO_Init()` (bắt buộc để thêm PD4), nó
đọc đúng những gì `.ioc` khai báo lúc đó — tức là bản mặc định — và ghi đè
luôn phần PA0 dù người dùng chỉ định đổi PE1 → PD4.

**2. Đổi số chân EXTI (1 → 4) đổi luôn tên khối `USER CODE`.** CubeMX đặt
tên khối theo tên IRQ (`EXTI1_IRQn`, `EXTI4_IRQn`), không theo chức năng. Khi
chân đổi từ EXTI1 sang EXTI4, khối `USER CODE BEGIN EXTI1_IRQn 0` (chứa lời
gọi `BSP_StartButton_IRQHandler()`) không còn tồn tại trong template mới —
nội dung bên trong **không được CubeMX tự động chuyển sang** khối
`EXTI4_IRQn 0` mới, phải tự thêm lại bằng tay.

**Bài học cốt lõi:** "Generate Code" áp dụng cho **toàn bộ** `MX_GPIO_Init()`
mỗi lần chạy, không chỉ phần chân vừa sửa trên GUI. Bất kỳ phần nào của
`.ioc` đang drift so với `main.c` (dù không liên quan tới thay đổi hiện tại)
đều bị "sửa sai" ngược lại theo `.ioc` cũ.

### Fix

Áp dụng lại đồng thời 3 fix cũ, lần này viết thẳng vào cả `.ioc` lẫn `main.c`
để không còn lệch nhau (đúng bài học của Bug #13):

```
# Freertos.ioc
PA0-WKUP.GPIOParameters=GPIO_ModeDefaultEXTI
PA0-WKUP.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
NVIC.EXTI0_IRQn=true:5:0:false:false:true:true:true:true
NVIC.EXTI4_IRQn=true:5:0:false:false:true:true:true:true
```

```c
// main.c — MX_GPIO_Init()
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;   // PA0
...
HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
```

```c
// stm32f4xx_it.c
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */
  BSP_StartButton_IRQHandler();
  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  ...
}
```

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Generate Code không phải thao tác cục bộ | Bấm Generate Code sau khi sửa 1 chân trên GUI sẽ ghi lại **toàn bộ** `MX_GPIO_Init()`, không chỉ chân vừa đổi — mọi phần đã drift âm thầm giữa `.ioc` và `.c` đều bị "đồng bộ ngược" về đúng những gì `.ioc` (có thể đang sai) khai báo |
| USER CODE block key theo tên IRQ, không theo ý nghĩa | Đổi EXTI line (đổi chân) luôn kéo theo đổi tên khối `USER CODE`; nội dung khối cũ **mất**, không tự chuyển — phải tự tay chép lại mọi lần đổi chân EXTI |
| Regression là rủi ro thật, không phải lý thuyết | Cùng một bug (#12, #13) đã tái phát nguyên vẹn chỉ sau 1 lần Generate Code không liên quan trực tiếp — checklist trong GPIO_CONFIG.md phải được chạy lại sau **mọi** lần Generate Code, không chỉ sau lần đầu fix |
| `.ioc` phải luôn là nguồn sự thật thực tế, không phải "đã từng đúng" | Sửa `.ioc` một lần rồi quên không có nghĩa nó ở nguyên trạng thái đó mãi mãi — nếu nghi ngờ, luôn diff `.ioc` hiện tại với các dòng đã fix trước đó trước khi tin tưởng |

---

## Bug #16 — PD4 Cũng Đọc LOW Cố Định, Nguyên Nhân Chưa Xác Nhận

### Triệu chứng

Sau khi đổi chân Start/Confirm sang PD4 (fix của Bug #14) và wiring lại nút
bấm y hệt cách cũ (PD4 ── [Nút] ── GND, pull-up nội đã cấu hình), đo bằng
logic analyzer: **PD4 cũng luôn ở mức LOW cố định**, giống hệt triệu chứng đã
gặp ở PE1 (Bug #14) — không chuyển được chức năng nào.

### Root Cause

**Chưa xác nhận.** Giả thuyết ban đầu khi chọn PD4 (xem Bug #14) là: PD4 trên
STM32F411E-DISCO nối tới chân RESET của audio DAC CS43L22, một *output* do
MCU điều khiển ra ngoài — nên về lý thuyết rủi ro xung đột thấp hơn nhiều so
với PE1 (một *input* nhận tín hiệu từ cảm biến gia tốc). Giả thuyết này dựa
trên schematic của board STM32F4-Discovery đời **407** (cùng chip CS43L22),
**không phải** schematic gốc của đúng board 411E (MB1115) — việc tra cứu PDF
schematic chính thức của board 411E bị timeout nhiều lần liên tiếp, không lấy
được xác nhận trực tiếp.

Ba khả năng chưa loại trừ được (theo thứ tự nghi ngờ giảm dần, xem thêm phép
thử cô lập bên dưới):

1. **Giả thuyết PD4 sai** — pin mapping CODEC_RESET trên board 411E có thể
   khác board 407, hoặc còn một mạch RC/pull-down phần cứng khác trên đường
   này mà suy luận từ signal-direction không lường tới.
2. **Lỗi lặp lại ở dây/nút/breadboard** — cùng một hàng breadboard, cùng một
   dây jumper, hoặc cùng một nút bấm đã dùng cho PE1 được tái sử dụng nguyên
   trạng cho PD4 mà không kiểm tra lại — nếu bản thân nút/dây có vấn đề
   (short thường trực, cắm nhầm hàng GND), triệu chứng LOW cố định sẽ lặp lại
   y hệt trên bất kỳ chân nào được nối vào, không liên quan gì tới việc chọn
   PE1 hay PD4.
3. **Sai tham chiếu đo** — thiết bị đo (logic analyzer/đồng hồ) đo sai chân
   vật lý hoặc kẹp GND-reference không đúng chỗ, khiến mọi phép đo đều ra LOW
   bất kể chân nào.

**Phép thử cô lập được đề xuất nhưng chưa thực hiện** (do quyết định của
người dùng ưu tiên đổi chân ngay để tiết kiệm thời gian thay vì chẩn đoán):

1. Chạm que đo vào chân 3.3V trên board — phải đọc HIGH. Nếu vẫn LOW, lỗi nằm
   ở thiết bị đo/GND-reference, không phải MCU.
2. Đo chân nghi vấn khi **rút hẳn dây nút bấm ra** (không nối gì) — nếu đọc
   HIGH, chân đó thực chất bình thường và lỗi nằm ở dây/nút/breadboard.
3. Đo continuity của nút bấm khi KHÔNG nhấn — phải hở mạch; nếu thông mạch
   sẵn, nút hỏng hoặc là loại thường-đóng (NC), hoặc dây đang cắm chung hàng
   với một mạch GND khác trên breadboard.

### Fix (tạm thời — chưa xác nhận triệt để nguyên nhân)

Đổi chân Start/Confirm sang **PD2** (xem thay đổi trong tài liệu này và
`GPIO_CONFIG.md` mục 2). PD2 **cũng chưa được xác nhận bằng schematic gốc**
của board 411E — nếu PD2 tiếp tục gặp hiện tượng tương tự, **bắt buộc** phải
chạy phép thử cô lập ở trên trước khi đổi sang chân thứ 4, vì xác suất "2 lần
liên tiếp đúng là xung đột phần cứng thật" đã thấp, "3 lần liên tiếp" gần như
chắc chắn là lỗi ở khâu đo/dây/nút chứ không phải ở từng chân MCU riêng lẻ.

### Bài học

| Khái niệm | Giải thích |
|---|---|
| Suy luận gián tiếp cần gắn nhãn "chưa xác nhận" | Kết luận PD4 an toàn dựa trên schematic của **board khác** (407, không phải 411E) — kết luận kiểu này phải được đánh dấu rõ là giả thuyết, không phải sự thật đã kiểm chứng, để không lặp lại sai lầm tự tin quá mức |
| Pattern lặp lại quan trọng hơn từng điểm dữ liệu đơn lẻ | Một chân đọc LOW là dấu hiệu đáng điều tra; **hai chân khác hẳn nhau về chức năng cùng đọc LOW** là dấu hiệu mạnh cho thấy nguyên nhân nằm ở yếu tố *không đổi* giữa 2 lần đo (dây, nút, thiết bị đo) — không nên tiếp tục đổi biến số (chân MCU) trong khi các biến số khác (dây, nút, cách đo) vẫn giữ nguyên chưa kiểm tra |
| Bỏ qua chẩn đoán để tiết kiệm thời gian là một đánh đổi có ý thức | Ở đây người dùng chủ động chọn đổi chân ngay thay vì cô lập nguyên nhân — quyết định hợp lý nếu ưu tiên tiến độ, nhưng phải ghi lại rõ ràng là "chưa xác nhận" để không nhầm với một bug đã điều tra đầy đủ, và để việc đổi chân tiếp theo (nếu cần) bắt đầu từ đúng bước đã bỏ qua |
| Tra cứu tài liệu bên ngoài có thể thất bại âm thầm | Không lấy được PDF schematic gốc (timeout) không có nghĩa là dừng lại — nhưng cũng không nên tự tin trình bày kết luận suy luận gián tiếp như thể đã xác nhận trực tiếp |

---

## Bug #17 — Sticky Level Flag: `EVT_ERROR` Không Bao Giờ Được Clear Khi Cửa Đóng Lại (CHƯA FIX)

### Triệu chứng (dự đoán từ đọc code, chờ kiểm chứng đầy đủ)

Sau khi mở cửa rồi đóng lại ở trạng thái `IDLE` — thao tác tự nhiên nhất khi
bỏ quần áo vào máy — bấm Start sẽ **không giặt được**: OLED nháy `FILL WATER`
trong vài chục micro giây rồi lập tức nhảy sang `ERROR`. Phải bấm Start
thêm 2 lần nữa (lần 2 reset về `IDLE`, lần 3 mới chạy thật).

### Root Cause

`sensor_task.c` duy trì `EVT_ERROR` **bất đối xứng** — có đường lên, không có
đường xuống:

```c
if (BSP_Sensor_IsDoorClosed()) {
    xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
    /* ← KHÔNG có xEventGroupClearBits(EVT_ERROR) tương ứng */
} else {
    xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
    xEventGroupSetBits(xSystemEvents, EVT_ERROR);
}
```

Trace theo thao tác người dùng:

| Bước | Hành động | `EVT_DOOR_CLOSED` | `EVT_ERROR` |
| --- | --- | --- | --- |
| 1 | `IDLE`, cửa đóng | 1 | 0 (do `Manager_OnEnterIdle()` clear) |
| 2 | Mở cửa bỏ đồ | 0 | **1** |
| 3 | Đóng cửa | 1 | **1** ← dính lại |
| 4 | Bấm Start | 1 | **1** |

Ở bước 4, `Manager_OnEnterFillWater()` mở van rồi vòng lặp gọi:

```c
xEventGroupWaitBits(xSystemEvents,
                    EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE, pdFALSE, portMAX_DELAY);
```

`EVT_ERROR` đã bằng 1 sẵn → hàm **không block, trả về ngay** → khớp dòng
`{ FILL_WATER, WASH_EVT_ERROR, ERROR }` trong `kTransitions[]` → vào `ERROR`.

Lý do bug này ẩn lâu nay: cửa được mô phỏng bằng nút toggle PD5 với mặc định
`s_doorClosed = 1`; nếu chưa từng bấm PD5 thì `EVT_ERROR` chưa từng được set.

**`EVT_WATER_FULL` mắc đúng bệnh này** — cũng không có nhánh `else` để clear.
Hệ quả: nếu PD6 đang ở trạng thái "đầy" từ chu trình trước, lần vào
`FILL_WATER` kế tiếp sẽ nhảy thẳng sang `WASHING` mà không nạp nước.

### Trạng thái

**Đã phát hiện, CHƯA fix** — theo quyết định của người dùng: ưu tiên học để
làm chủ project trước, sửa sau. Ba phương án đã phân tích:

- **A — Cho SensorTask clear `EVT_ERROR` ở nhánh `if`.** Đơn giản nhưng
  **sai về ownership**: `EVT_ERROR` còn do fill-timeout (`Manager_OnEnterError`)
  gây ra, SensorTask clear sẽ xóa nhầm lỗi của module khác.
- **B — Tách cờ theo nguyên nhân** (khuyến nghị): SensorTask chỉ sở hữu
  `EVT_DOOR_CLOSED` (level, đối xứng); WashingManager tự suy ra lỗi cửa từ
  level đó thay vì dùng `EVT_ERROR` chung. Mỗi bit một chủ sở hữu duy nhất.
  Cùng tinh thần với đề xuất tách `EVT_TIMEOUT` thành 4 bit theo phase.
- **C — Consumer dọn cờ trước khi "vũ trang"**: thêm
  `xEventGroupClearBits(EVT_ERROR | EVT_WATER_FULL)` vào đầu
  `Manager_OnEnterFillWater()`, theo đúng pattern `Manager_OnEnterIdle()`
  đang dùng. Rẻ nhất nhưng chỉ xử lý cờ *cũ*, không thay thế được **guard**
  kiểm tra điều kiện lúc bắt đầu.

Lời giải đầy đủ là **B (hoặc C) + guard** — hai thứ giải quyết hai vấn đề
khác nhau: cờ đối xứng đảm bảo thông tin không cũ, guard chặn khởi động khi
điều kiện an toàn chưa thỏa.

### Bài học

| Khái niệm | Giải thích |
| --- | --- |
| Sticky / stale level flag | Cờ biểu diễn *trạng thái hiện tại* nhưng chỉ có code set mà không có code clear tương ứng → cờ mô tả **quá khứ**, tức là nó nói dối. Không crash, không warning, chỉ hành xử sai |
| Cờ level phải đối xứng | Đã dùng một bit làm level thì **mọi nhánh** của producer đều phải cập nhật nó (`if` set thì `else` phải clear) — thiếu một nhánh là đủ hỏng |
| Một bit, một chủ sở hữu | `EVT_ERROR` đang có 2 producer (SensorTask + WashingManager) nên không ai dám clear an toàn. Bit dùng chung nhiều nguồn thì nên tách, hoặc chỉ định rõ **một** chủ thể chịu trách nhiệm clear |
| Bug phụ thuộc lịch sử | Chỉ lộ ra sau một *chuỗi* thao tác (mở → đóng → start), test từng chức năng riêng lẻ không bao giờ thấy — cần test theo kịch bản người dùng thật |
| Triệu chứng ở xa nguyên nhân | Bug nằm ở `sensor_task.c` nhưng biểu hiện ở `washing_manager.c`, cách nhau 2 module — lý do phải trace theo *dữ liệu* (bit), không chỉ theo *luồng gọi hàm* |

---

## Tổng kết — Timeline fix

```
Flash lần 1 → [Bug #1] EXTI priority → fix → flash lại
Flash lần 2 → [Bug #2] I2C no pull-up → fix → flash lại
Flash lần 3 → [Bug #3] configASSERT hang → disable check (workaround)
Flash lần 4 → [Bug #4] HardFault (EXC_RETURN) → proper fix with weak symbols
Flash lần 5 → Firmware RUNS ✓ → phát hiện Bug #5, #6, #7
Flash lần 6 → All bugs fixed ✓
Flash lần 7 → Test thủ công FILL_WATER → [Bug #8] PA0 shared với button → fix (door = software flag)
Flash lần 8 → Test lại: đúng thiết kế (fill timeout 10s → ERROR khi chưa set s_waterFull) ✓
             → phát hiện [Bug #9] OLED status cache — đã fix 2026-10-10 (manager giữ con trỏ)
Review code (chưa flash) → refactor bsp_button.c sang CubeMX-generated GPIO/NVIC
             → phát hiện [Bug #11] EXTI1 quên gọi handler, [Bug #12] PA0 thiếu cạnh FALLING,
               [Bug #13] .ioc/.c lệch NVIC priority → fix cả 3 trước khi flash lại
Đấu dây PE1 thật → [Bug #14] PE1 xung đột MEMS_INT2 (logic analyzer: luôn LOW
             dù đảo pull) → tra schematic → quyết định đổi chân sang PD4
Đổi chân qua CubeMX GUI → Generate Code → [Bug #15] PA0 + EXTI4 hồi sinh
             Bug #12/#13 (drift trong .ioc), EXTI4_IRQHandler rỗng (tái phát
             Bug #11) → fix cả 3 lại lần nữa, đồng bộ .ioc/.c triệt để
Đấu dây PD4 thật → [Bug #16] PD4 cũng luôn LOW, nguyên nhân CHƯA xác nhận
             (bỏ qua bước cô lập theo yêu cầu) → đổi tiếp sang PD2
Đổi chân qua CubeMX GUI lần 2 → Generate Code → Bug #15 tái phát lần 3 y hệt
             (PA0 mất FALLING + priority 0, EXTI2 priority 0) → fix lại,
             đồng bộ .ioc/.c → CHƯA xác minh PD2 có hoạt động trên phần cứng
```

## Cheat Sheet — Debug Checklist cho STM32 + FreeRTOS

```
[ ] Tất cả ISR gọi FreeRTOS API có priority >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY?
[ ] I2C GPIO có pull-up resistor không (external hoặc internal)?
[ ] FreeRTOS handlers (SVC/PendSV/SysTick) vào vector table đúng cách (naked asm, không qua C wrapper)?
[ ] Display RAM được clear sau Init()?
[ ] Long press threshold có phù hợp UX (< 2 giây)?
[ ] Mỗi khi internal state thay đổi, có gửi display update không?
[ ] Data ownership rõ ràng: ai own field nào trong display message?
[ ] Có chân GPIO nào dùng chung giữa input điều khiển (button) và cảm biến an toàn (door/limit switch) không?
[ ] Cache trạng thái ở task khác (vd UITask) có được đồng bộ khi nguồn sự thật (WashingManager) đổi không?
[ ] Mỗi ISR mới thêm có thực sự được gọi từ vector handler tương ứng không (diff với 1 ISR tương tự đã hoạt động)?
[ ] Cạnh EXTI (RISING/FALLING/cả hai) trong .ioc có khớp với logic thực tế trong IRQHandler không?
[ ] Giá trị "đúng" nằm ngoài USER CODE block — có thực sự khớp với .ioc, hay chỉ đang trùng hợp còn sót lại từ lần sửa tay trước (config drift)?
[ ] Chân GPIO định dùng cho input/output tự chế đã đối chiếu với schematic board chưa (không bị chiếm dụng bởi cảm biến/audio/USB tích hợp sẵn)?
[ ] Đo bằng logic analyzer thấy mức tín hiệu không đổi dù đảo pull-up/pull-down nội — có nghi ngờ driver bên ngoài đang chiếm dụng chân không?
[ ] Sau MỌI lần Generate Code (kể cả khi chỉ đổi 1 chân khác) — đã re-check toàn bộ GPIO liên quan tới FreeRTOS ISR, không chỉ chân vừa sửa?
[ ] Vừa đổi số EXTI line (đổi chân) — đã tự tay chép lại nội dung USER CODE từ khối cũ sang khối mới chưa (CubeMX không tự chuyển)?
[ ] Đã đổi ≥2 chân liên tiếp mà vẫn cùng một triệu chứng (vd luôn LOW)? Nếu có — dừng đổi chân tiếp, nghi ngờ dây/nút/thiết bị đo trước khi nghi ngờ tiếp con chân MCU thứ 3
[ ] Kết luận "chân này an toàn" có dựa trên schematic của ĐÚNG board revision đang dùng không, hay chỉ suy luận gián tiếp từ board tương tự (khác revision)?
```
