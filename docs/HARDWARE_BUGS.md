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
if (event == BTN_EVENT_SHORT_PRESS)
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
if (event == BTN_EVENT_SHORT_PRESS)
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
| Defensive init | Luôn clear display sau `Init()` trước khi vẽ nội dung thực |
| `ssd1306_UpdateScreen()` | Truyền toàn bộ 128×64 = 1024 bytes qua I2C; tốn thời gian nhưng cần thiết |

---

## Tổng kết — Timeline fix

```
Flash lần 1 → [Bug #1] EXTI priority → fix → flash lại
Flash lần 2 → [Bug #2] I2C no pull-up → fix → flash lại
Flash lần 3 → [Bug #3] configASSERT hang → disable check (workaround)
Flash lần 4 → [Bug #4] HardFault (EXC_RETURN) → proper fix with weak symbols
Flash lần 5 → Firmware RUNS ✓ → phát hiện Bug #5, #6, #7
Flash lần 6 → All bugs fixed ✓
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
```
