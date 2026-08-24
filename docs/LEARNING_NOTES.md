# FreeRTOS Learning Notes — Washing Machine Simulator

Ghi lại toàn bộ kiến thức lý thuyết, thiết kế, và bài học từ mỗi phase.
Cập nhật sau mỗi phase theo đúng quy trình CLAUDE.md.

---

## Phase 1 — Project Setup & FreeRTOS Tasks

### Problem Being Solved

Bắt đầu từ một project STM32 trống. Cần thiết lập nền tảng: RTOS chạy được,
cấu trúc thư mục rõ ràng, và proof-of-life bằng 3 task blink chạy song song.

---

### FreeRTOS Concept: Task

#### ① Problem it solves

Trong lập trình truyền thống (bare-metal), chỉ có một luồng thực thi. Nếu muốn
hai việc xảy ra "cùng lúc" phải viết state machine thủ công hoặc dùng interrupt.
Task trong FreeRTOS cho phép chia chương trình thành nhiều luồng độc lập, mỗi
luồng tưởng như có CPU riêng.

#### ② Tại sao cần trong project này

Máy giặt cần xử lý đồng thời: đọc cảm biến, xử lý nút bấm, điều khiển motor,
cập nhật OLED. Nếu bare-metal phải tự quản lý từng việc trong một vòng lặp —
rất dễ bug và khó mở rộng.

#### ③ Tại sao alternatives bị loại

- **Super-loop (while 1):** Không phản ứng kịp thời, khó thêm feature mới
- **Interrupt-only:** ISR không nên chứa logic phức tạp, không thể block chờ

#### ④ Runtime behavior

```
vTaskStartScheduler() gọi → FreeRTOS chiếm quyền điều khiển
    → Tạo Idle Task (priority 0)
    → Bật SysTick interrupt (configTICK_RATE_HZ = 1000 → cứ 1ms một tick)
    → Chạy task có priority cao nhất đang Ready
```

#### ⑤ Task state transitions

```
xTaskCreate() → [Ready]
                   │ scheduler chọn
                   ▼
              [Running]  ←──────────────────┐
                   │ vTaskDelay()/block      │ preempt/yield
                   ▼                         │
              [Blocked]  ──timeout/signal──► [Ready]
                   │ vTaskSuspend()
                   ▼
             [Suspended] ──vTaskResume()──► [Ready]
```

Chỉ **một** task ở trạng thái Running tại một thời điểm (single-core).

#### ⑥ Context switching behavior

Khi SysTick fire (mỗi 1ms), FreeRTOS kiểm tra xem có task Ready nào có
priority cao hơn task đang chạy không. Nếu có → preempt ngay. Context switch
lưu toàn bộ registers (R0-R15, PSR, FPU registers trên Cortex-M4F) vào stack
của task bị preempt, nạp registers của task mới.

#### ⑦ Memory implications

```
xTaskCreate(func, name, stackDepth, param, priority, handle)
                         ^^^^^^^^^^^
                         ĐƠN VỊ: WORDS (4 bytes trên Cortex-M4)
                         stackDepth=256 → 256 × 4 = 1024 bytes RAM
```

Stack lưu: local variables, function call frames, saved registers khi
context switch, FPU registers (nếu dùng floating point).

Stack quá nhỏ → Stack Overflow → crash hoặc data corruption.

#### ⑧ Common mistakes

- Nhầm stack size: nghĩ 256 là 256 bytes, thực ra là 256 words = 1024 bytes
- Gọi code sau `vTaskStartScheduler()` — code đó không bao giờ chạy
- Task function `return` — undefined behavior, phải có `vTaskDelete(NULL)` hoặc vòng lặp vô tận
- Tạo task trước `App_FreeRTOSObjectsInit()` → task dùng queue NULL → assert

#### ⑨ Interview-style

> "FreeRTOS task là một luồng thực thi độc lập với stack riêng và priority riêng.
> Scheduler chạy task có priority cao nhất đang Ready. Mỗi task phải là vòng lặp
> vô tận — nếu function return, hành vi là undefined. Stack overflow trên Cortex-M4
> có thể kích hoạt HardFault hoặc làm corrupt dữ liệu của task khác tùy vị trí
> stack trong RAM."

---

### Design Decisions — Phase 1

**Tại sao `vTaskDelay()` thay vì `HAL_Delay()`:**

| | `HAL_Delay()` | `vTaskDelay()` |
|---|---|---|
| Cơ chế | Busy-wait (spin) | Block task, nhường CPU |
| CPU trong khi chờ | 100% | 0% (Idle task chạy) |
| Dùng trong RTOS | Sai | Đúng |

`HAL_Delay()` trong RTOS task = lãng phí CPU và có thể starve các task khác.

---

### What I Should Learn From Phase 1

1. Task là đơn vị thực thi cơ bản — mỗi task có stack, priority, state riêng.
2. Stack size đơn vị là **words**, không phải bytes — nhân 4 để ra bytes.
3. Code sau `vTaskStartScheduler()` không bao giờ chạy — scheduler không return.
4. `vTaskDelay()` là blocking (Blocked state), `HAL_Delay()` là busy-wait — luôn dùng `vTaskDelay` trong RTOS.

---

---

## Phase 2 — Application Data Types & FreeRTOS Objects

### Problem Being Solved

Trước khi viết bất kỳ task logic nào, cần định nghĩa "ngôn ngữ" mà các tasks
dùng để giao tiếp: data types, message structs, event masks, và RTOS objects.
Làm điều này trước giúp tránh circular dependency và forced refactor sau.

---

### Design: Tại sao định nghĩa types trước khi viết tasks

Kinh nghiệm embedded: Data flows drive architecture. Nếu biết dữ liệu gì di
chuyển giữa tasks, tự nhiên biết cần queue/event group nào, ai là producer/consumer.

Ngược lại — viết task trước rồi mới nghĩ data flow → thường dẫn đến refactor lớn.

---

### FreeRTOS Concept: Queue

#### ① Problem it solves

Task A cần gửi dữ liệu cho Task B mà không cần biết Task B đang làm gì. Queue
là FIFO buffer thread-safe: A ghi vào, B đọc ra, FreeRTOS đảm bảo không có
race condition.

#### ② Tại sao cần trong project này

- `xButtonQueue`: ISR → UITask (button events)
- `xCommandQueue`: UITask → WashingManager (commands)
- `xDisplayQueue`: WashingManager → OLEDTask (display updates)

Queue cho phép producer và consumer chạy ở tốc độ khác nhau mà không mất dữ liệu.

#### ③ Tại sao alternatives bị loại

- **Global variable:** Race condition, không có synchronization
- **Task Notification:** Depth = 1 (mất event nếu chưa đọc kịp), chỉ mang 32-bit scalar

#### ④ Runtime behavior

```
xQueueSend(q, &item, timeout):
    Queue có chỗ trống  → copy item vào queue, return pdPASS ngay
    Queue đầy, timeout=0 → return errQUEUE_FULL ngay
    Queue đầy, timeout>0 → task vào Blocked state, chờ có chỗ trống

xQueueReceive(q, &item, portMAX_DELAY):
    Queue có item → copy ra, return pdPASS ngay
    Queue trống  → task vào Blocked state, chờ item mới
```

#### ⑤ Task state transitions

```
Producer (UITask):
  xQueueSend(..., 0) → queue đầy → return pdFAIL (không block, timeout=0)
  xQueueSend(..., portMAX_DELAY) → queue đầy → [Blocked] → có chỗ → [Ready]

Consumer (WashingManager):
  xQueueReceive(..., portMAX_DELAY) → queue trống → [Blocked]
  → producer gửi item → [Ready] → [Running] → đọc item
```

#### ⑥ Memory implications

```
xQueueCreate(length, itemSize):
    RAM = ~76 bytes (control block) + length × itemSize (storage buffer)

xButtonQueue  : ~76 + 5×4   = ~96  bytes
xCommandQueue : ~76 + 5×8   = ~116 bytes
xDisplayQueue : ~76 + 5×36  = ~256 bytes
```

FreeRTOS copy item **by value** vào queue storage. Không lưu pointer — an toàn với stack variables.

#### ⑦ Common mistakes

- Truyền pointer vào queue thay vì struct → pointer trỏ vào stack đã bị giải phóng
- Gọi `xQueueSend()` từ ISR thay vì `xQueueSendFromISR()` → crash
- Queue depth = 1 → producer block ngay khi consumer chưa đọc kịp

#### ⑧ Interview-style

> "FreeRTOS queue là FIFO buffer thread-safe. Nó copy data by value — producer
> không cần giữ data sống sau khi send. ISR phải dùng `xQueueSendFromISR()` và
> gọi `portYIELD_FROM_ISR()` sau để scheduler re-evaluate task priority."

---

### FreeRTOS Concept: Event Group

#### ① Problem it solves

Cho phép một task chờ **nhiều điều kiện cùng lúc** trong một lệnh blocking.
Queue chỉ chờ một item. Event Group cho chờ "bit A AND bit B" hoặc "bit A OR bit B".

#### ② Tại sao cần trong project này

WashingManager cần chờ `EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR` đồng thời.
Nếu dùng 3 queue riêng phải polling tuần tự — không thể block đúng nghĩa.

#### ③ Tại sao alternatives bị loại

- **Polling loop:** Tốn CPU, độ trễ phụ thuộc polling period
- **Binary semaphore:** Chỉ signal 1:1, không mang bit meaning
- **Queue:** Không thể wait OR/AND nhiều condition cùng lúc

#### ④ Runtime behavior

```
xEventGroupSetBits(group, EVT_TIMEOUT):
    Set bit trong EventBits_t (32-bit atomic)
    Kiểm tra xem task nào đang WaitBits trên các bit này
    Nếu điều kiện wait thỏa → unblock task đó

xEventGroupWaitBits(group, bits, clearOnExit, waitAll, timeout):
    Nếu bits đã set → return ngay
    Nếu chưa → task vào Blocked state
    clearOnExit=pdTRUE → xóa bits matched sau khi wake
    waitAll=pdFALSE    → wake khi BẤT KỲ bit nào trong mask được set
```

#### ⑤ Bits persistence — điểm quan trọng

Queue: item được consume → biến mất.
Event Group: bit được set → **tồn tại** cho đến khi xóa thủ công.

Nếu SensorTask set `EVT_DOOR_CLOSED` trước khi WashingManager đến lệnh
WaitBits, bit vẫn còn đó — WashingManager không bị miss event.

#### ⑥ Memory implications

`xEventGroupCreate()` = ~76 bytes trên heap_4.
`EventBits_t` = 32 bits, nhưng 8 bits cao reserved by kernel → 24 bits usable.

#### ⑦ Common mistakes

- Gọi `xEventGroupSetBits()` từ ISR → phải dùng `xEventGroupSetBitsFromISR()`
- Không clear bits sau khi xử lý → bit cũ ảnh hưởng wait tiếp theo
- Dùng bit 24-31 → reserved, behavior undefined

---

### Design: Self-Contained Message Structs

`DisplayMsg_t` chứa `char statusText[24]` — mảng inline, không phải pointer.

```c
// SAI: pointer vào stack
char buf[24] = "WASHING";
DisplayMsg_t msg;
msg.statusText = buf;        // buf có thể bị overwrite trước khi OLEDTask đọc
xQueueSend(xDisplayQueue, &msg, 0);

// ĐÚNG: copy by value
strncpy(msg.statusText, "WASHING", sizeof(msg.statusText) - 1);
xQueueSend(xDisplayQueue, &msg, 0);  // toàn bộ 24 bytes được copy vào queue
```

FreeRTOS copy `sizeof(DisplayMsg_t)` bytes khi send. Không có pointer, không có
heap string, không có lifetime issue.

---

### What I Should Learn From Phase 2

1. Queue copy by value — không dùng pointer trong message structs.
2. Event Group dành cho "chờ nhiều điều kiện" — Queue dành cho "truyền dữ liệu".
3. Event bits persistent — bits tồn tại đến khi xóa thủ công, không bị miss nếu producer nhanh hơn consumer.
4. Thiết kế data types trước task logic — data flow drives architecture.
5. ISR phải dùng `FromISR` variants của tất cả FreeRTOS API.

---

---

## Phase 3 — UI Task & Button ISR

### Problem Being Solved

Button PA0 tạo EXTI interrupt. Cần:
1. ISR ngắn, không chứa logic
2. Debounce để lọc bounce
3. Phân biệt short press / long press
4. Truyền event đến UITask một cách thread-safe

---

### Design: ISR → Queue → Task Pattern

```
[Hardware interrupt PA0]
        │
        ▼
EXTI0_IRQHandler()          ← phải ngắn nhất có thể
        │
BSP_Button_IRQHandler()     ← debounce + ghi timestamp
        │
xQueueSendFromISR()         ← copy ButtonEvent_t vào queue
        │
portYIELD_FROM_ISR()        ← cho scheduler biết có task high-priority ready
        │
        ▼
UITask (Blocked → Ready → Running)
        │
xQueueReceive()             ← đọc event, xử lý logic
        │
        ▼
xQueueSend(xCommandQueue)   ← gửi command nếu cần
```

ISR chỉ làm một việc: capture event và post. Mọi logic đều trong task.

---

### Tại sao `portYIELD_FROM_ISR()` bắt buộc

```c
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

`xQueueSendFromISR()` set `xHigherPriorityTaskWoken = pdTRUE` nếu việc gửi
item vừa unblock một task có priority cao hơn task đang chạy.

Nếu không gọi `portYIELD_FROM_ISR()`: ISR return, CPU tiếp tục chạy task
cũ (priority thấp hơn), phải đợi đến tick tiếp theo mới switch sang UITask.
Latency tăng lên đến 1ms.

Nếu có `portYIELD_FROM_ISR(pdTRUE)`: ISR return → PendSV được trigger ngay →
scheduler chạy UITask ngay lập tức. Latency chỉ vài microseconds.

---

### Design: Debounce

Cơ chế: so sánh timestamp rising/falling edge.

```c
// Khi có interrupt:
uint32_t now = HAL_GetTick();
if (now - s_lastEdgeTime < BUTTON_DEBOUNCE_MS) return;  // bounce, bỏ qua
s_lastEdgeTime = now;
```

Tại sao không dùng `vTaskDelay()` trong ISR để debounce:
- ISR không thể block
- `vTaskDelay()` trong ISR → crash

---

### Design: Short Press vs Long Press

```c
// Rising edge (nhấn xuống): ghi timestamp
s_pressTime = HAL_GetTick();

// Falling edge (thả ra): tính duration
uint32_t duration = HAL_GetTick() - s_pressTime;
ButtonEvent_t event = (duration >= BUTTON_LONG_PRESS_MS)
                    ? BTN_EVENT_CONFIRM
                    : BTN_EVENT_NAVIGATE;
xQueueSendFromISR(xButtonQueue, &event, &xWoken);
```

---

### What I Should Learn From Phase 3

1. ISR phải ngắn nhất có thể — chỉ capture và post, không xử lý logic.
2. `portYIELD_FROM_ISR()` giảm latency từ 1ms xuống vài microseconds.
3. Debounce bằng timestamp comparison — không dùng delay trong ISR.
4. `xQueueSendFromISR()` không block — nếu queue đầy thì drop event.
5. Pattern: ISR → Queue → Task là nền tảng của mọi interrupt-driven system trong FreeRTOS.

---

---

## Phase 4 — Sensor Task

### Problem Being Solved

Cần poll trạng thái door sensor và water level sensor định kỳ, rồi cập nhật
event group để WashingManager có thể react.

---

### Design: Periodic Task Pattern

```c
void SensorTask(void *pvParameters)
{
    for (;;)
    {
        // đọc sensor, set/clear bits
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));  // 100ms
    }
}
```

Đây là "tick-driven polling" — không phải event-driven. Chấp nhận được cho
sensor polling vì sensor hardware không tạo interrupt.

**Tại sao 100ms:**
- Đủ nhanh để phát hiện door open trong < 100ms
- Không quá nhanh để tốn CPU

---

### Design: Event Group Producer

```c
if (BSP_Sensor_IsDoorClosed())
    xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
else {
    xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
    xEventGroupSetBits(xSystemEvents, EVT_ERROR);
}
```

SensorTask là producer — chỉ set/clear bits, không biết consumer là ai.
WashingManager là consumer — chờ bits, không biết producer là ai.
Đây là loose coupling qua event group.

---

### `vTaskDelay` vs `vTaskDelayUntil`

| | `vTaskDelay(100)` | `vTaskDelayUntil(&last, 100)` |
|---|---|---|
| Ý nghĩa | Ngủ 100ms kể từ **bây giờ** | Ngủ đến khi **last + 100ms** |
| Nếu task bị preempt | Period bị kéo dài | Period vẫn đúng 100ms |
| Dùng khi nào | Delay đơn giản | Cần period chính xác (control loop) |

SensorTask dùng `vTaskDelay` là đủ — không cần period chính xác đến microsecond.

---

### What I Should Learn From Phase 4

1. Sensor polling là acceptable khi sensor không có interrupt output.
2. `vTaskDelay` vs `vTaskDelayUntil` — chọn dựa trên yêu cầu timing.
3. Event group producer/consumer loose coupling — producer không biết ai đang chờ.
4. Clear bits chủ động khi condition không còn đúng — bit persistent nên phải clear thủ công.

---

---

## Phase 5 — WashingManager & State Machine

### Problem Being Solved

Cần một module duy nhất điều phối toàn bộ logic máy giặt: nhận command, chạy
state machine, điều khiển actuators, cập nhật display. Nếu logic phân tán
trong nhiều task → race condition và khó debug.

---

### Design: Table-Driven State Machine

```c
static const SmTransition_t kTransitions[] = {
    { WASH_STATE_IDLE,       WASH_EVT_START_WASH,  WASH_STATE_FILL_WATER },
    { WASH_STATE_FILL_WATER, WASH_EVT_WATER_FULL,  WASH_STATE_WASHING    },
    // ...
};
```

**Tại sao table-driven thay vì switch-case lồng nhau:**

```c
// BAD: switch-case lồng nhau
switch (state) {
    case IDLE:
        switch (event) {
            case START_WASH: ...   // thêm event mới → sửa switch
        }
}

// GOOD: table-driven
// Thêm transition → chỉ thêm 1 row vào table
// Dispatcher loop không bao giờ thay đổi
```

Table-driven SM: O(n) tìm kiếm nhưng n nhỏ (< 20 transitions). Đổi lại
được code cực kỳ dễ đọc và maintain.

---

### Design: Entry-Action Callbacks

SM core không biết FreeRTOS hay HAL tồn tại. Hardware actions được inject
qua callback pointers:

```c
typedef struct {
    void (*onEnterWashing)(void);
    void (*onEnterDrain)(void);
    // ...
} WashingSM_Callbacks_t;
```

**Lợi ích:**
- SM có thể unit test mà không cần hardware
- WashingManager quyết định hardware action, SM quyết định state logic
- Separation of concerns rõ ràng

---

### Design: State-Dependent Blocking

WashingManager block trên primitive khác nhau tùy state:

```
IDLE/FINISH/ERROR → xQueueReceive(xCommandQueue)   ← chờ user input
FILL_WATER        → xEventGroupWaitBits(EVT_WATER_FULL|EVT_TIMEOUT|EVT_ERROR)
WASHING/DRAIN/SPIN→ xEventGroupWaitBits(EVT_TIMEOUT|EVT_ERROR)
```

**Tại sao không dùng một vòng lặp unified với timeout ngắn:**

```c
// BAD: polling approach
while (1) {
    checkQueue();
    checkEvents();
    vTaskDelay(10);  // 10ms polling → tốn CPU, không event-driven
}

// GOOD: state-dependent blocking
// Task block 100% cho đến khi đúng signal đến
// Zero CPU consumption khi chờ
```

---

### Design: BSP Layer

```
Application (washing_manager.c)
        │
        │ BSP_Motor_Start()
        ▼
BSP (bsp_motor.c)
        │
        │ HAL_GPIO_WritePin(...)
        ▼
HAL / Hardware
```

**Tại sao cần BSP:**
- Application code không phụ thuộc HAL → dễ port sang board khác
- Nếu chân GPIO thay đổi: chỉ sửa BSP, không sửa application
- Tên hàm có ý nghĩa domain: `BSP_Motor_Start()` rõ hơn `HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)`

**Một BSP file per peripheral:**
`bsp_motor.c`, `bsp_valve.c`, `bsp_led.c` riêng biệt — không gộp vào
`bsp_actuators.c`. Lý do: file nhỏ, trách nhiệm rõ ràng, dễ tìm code.

---

### What I Should Learn From Phase 5

1. Table-driven SM: thêm transition = thêm 1 row, dispatcher không đổi.
2. Entry-action callbacks tách SM logic khỏi hardware — SM không import FreeRTOS hay HAL.
3. State-dependent blocking: mỗi state block trên đúng signal nó cần — không polling.
4. BSP layer là ranh giới giữa application và hardware — chỉ BSP biết HAL tồn tại.
5. WashingManager là single owner của state machine — không task nào khác gọi WashingSM_ProcessEvent().

---

---

## Phase 6 — OLED Task & Driver Integration

### Problem Being Solved

Cần render trạng thái máy giặt lên OLED SSD1306 qua I2C mà không làm
WashingManager phải chờ I2C transaction hoàn tất.

---

### Design: Pure Renderer Pattern

OLEDTask là "pure renderer" — không có state riêng, không quyết định gì,
chỉ render những gì được gửi đến:

```c
void OLEDTask(void *pvParameters)
{
    BSP_OLED_Init();
    DisplayMsg_t msg;
    for (;;) {
        xQueueReceive(xDisplayQueue, &msg, portMAX_DELAY);
        OLED_Render(&msg);  // render từ msg, không từ global state
    }
}
```

**Tại sao không để WashingManager gọi BSP_OLED trực tiếp:**
- I2C transaction có thể mất 5-10ms (128×64 pixels, 100kHz I2C)
- WashingManager block 10ms mỗi lần update display → bỏ lỡ events
- OLEDTask priority thấp nhất → bị preempt bất cứ lúc nào, không ảnh hưởng hệ thống

**Tại sao `DisplayMsg_t` self-contained:**
```c
typedef struct {
    WashState_t state;
    WashMode_t  selectedMode;
    uint8_t     menuIndex;
    char        statusText[24];  // mảng inline, không phải pointer
} DisplayMsg_t;
```

OLEDTask có đủ mọi thứ cần render chỉ từ một message — không cần đọc
global state, không cần lock/mutex.

---

### Design: Driver Integration Rule

```
Drivers/SSD1306/ssd1306.c   ← driver thuần, không biết FreeRTOS
        │
BSP/bsp_oled.c              ← wraps driver, expose simple API
        │
OLEDTask/oled_task.c        ← gọi BSP, không bao giờ gọi driver trực tiếp
```

Driver không được include FreeRTOS headers. Driver là pure C library.
BSP là adapter giữa driver và application.

---

### FreeRTOS: OLEDTask Priority = Lowest (1)

OLEDTask có priority 1 (thấp nhất trong application). Lý do:
- Display update không time-critical — trễ 10ms không ai nhận ra
- Nếu hệ thống bận, OLEDTask bị preempt → chấp nhận được
- WashingManager (priority 3) luôn preempt OLEDTask nếu cần

---

### What I Should Learn From Phase 6

1. Queue decouples producer (WashingManager) khỏi consumer (OLEDTask) về thời gian.
2. Pure renderer không có state — mọi thứ cần render phải nằm trong message.
3. Self-contained message structs tránh dangling pointer và race condition.
4. Driver không được depend on FreeRTOS — chỉ BSP layer mới wrap driver.
5. Priority thấp cho renderer task — bị preempt là acceptable, không block hệ thống.

---

---

## Phase 7 — Software Timers

### Problem Being Solved

`vTaskDelay(20000ms)` trong WASHING state block WashingManager cứng — không
thể phản ứng với EVT_ERROR (cửa mở) trong 20 giây. Cần cơ chế timeout
event-driven: timer đếm độc lập, báo signal khi hết giờ.

---

### Design Overview

```
WashingManager (task)          Timer Service Task
       │                              │
       │  xTimerStart(xWashTimer)     │
       │─────────────────────────────>│
       │                              │  đếm 20s
       │  xEventGroupWaitBits(        │
       │    EVT_TIMEOUT|EVT_ERROR)    │
       │◄── blocked ─────────────────│
       │                              │  20s hết → callback
       │                              │  xEventGroupSetBits(EVT_TIMEOUT)
       │◄── unblocked ───────────────│
       │  xử lý event                │
```

---

### FreeRTOS Concept: Software Timer

#### ① Problem it solves

Thực thi code sau một khoảng thời gian mà không cần tạo task riêng. Thay
polling `while(tick < deadline)` bằng event-driven callback.

#### ② Tại sao cần trong project này

Mỗi phase máy giặt có deadline cố định. Cần báo "hết giờ" mà không block
WashingManager bằng `vTaskDelay`.

#### ③ Tại sao alternatives bị loại

| Alternative | Vấn đề |
|---|---|
| `vTaskDelay` | Block cứng, không thể interrupt bằng EVT_ERROR |
| HAL TIM peripheral | Tốn hardware resource, không portable |
| Polling trong task | Tốn CPU, không event-driven |
| Dedicated timeout task | Lãng phí RAM/stack cho task chỉ đếm giờ |

#### ④ Runtime behavior

```
xTimerCreate() → tạo timer control block trên heap_4 (stopped state)
xTimerStart()  → gửi lệnh vào Timer Command Queue
                 Timer Service Task đọc lệnh, bắt đầu đếm tick
Khi hết period → Timer Service Task gọi callback trực tiếp
                 (trong context của Timer Service Task)
```

#### ⑤ Task state transitions

```
WashingManager: Running → Blocked (xEventGroupWaitBits)
Timer Service : Blocked → Running (khi timer expire)
                Running → gọi callback → xEventGroupSetBits(EVT_TIMEOUT)
WashingManager: Blocked → Ready → Running (preempt vì priority cao hơn)
```

#### ⑥ Context switching

`xEventGroupSetBits()` trong callback unblock WashingManager (priority 3).
Timer Service Task có priority 2. Scheduler preempt Timer Service Task ngay
sau khi callback return — WashingManager chạy ngay, không đợi tick tiếp theo.

#### ⑦ Memory implications

```
xTimerCreate() = ~60 bytes/timer trên heap_4
4 timers       = ~240 bytes

Timer Service Task stack:
configTIMER_TASK_STACK_DEPTH = configMINIMAL_STACK_SIZE × 2 = 260 words = 1040 bytes
```

#### ⑧ Common mistakes

- Gọi blocking API trong callback → treo Timer Service Task, mọi timer trong hệ thống bị ảnh hưởng
- Quên `xTimerStop()` khi signal khác đến trước → zombie timer fire vào state sai
- Dùng `uxAutoReload = pdTRUE` cho timeout → fire liên tục thay vì một lần
- `xTimerStart()` gửi command vào Timer Command Queue — nếu queue đầy trả về pdFAIL, không block

#### ⑨ Interview-style

> "FreeRTOS software timer không dùng hardware timer — nó chạy trong Timer
> Service Task, đếm tick thông qua SysTick. Callback chạy trong context của
> Timer Service Task, không phải task của bạn. Không được block trong callback.
> One-shot timer dùng cho deadline/timeout. Auto-reload dùng cho periodic action."

---

### Design: One Shared Callback

Tất cả 4 timers dùng chung `Timer_PhaseCallback` vì tất cả đều làm cùng việc:
set `EVT_TIMEOUT`. WashingManager đọc SM state để biết timer nào fire — không
cần phân biệt timer ID.

```c
static void Timer_PhaseCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT);
}
```

**Nguyên tắc:** Callback chứa càng ít code càng tốt. Logic nằm trong task.

---

### Design: xTimerStop() — Khi nào cần

```
FILL_WATER: chờ EVT_WATER_FULL | EVT_TIMEOUT
    → EVT_WATER_FULL đến trước → xTimerStop(xFillTimeoutTimer)  ← cần
    → EVT_TIMEOUT đến trước   → timer đã tự dừng (one-shot)    ← không cần

WASHING: chờ EVT_TIMEOUT | EVT_ERROR
    → EVT_TIMEOUT đến trước → timer đã tự dừng                  ← không cần
    → EVT_ERROR đến trước   → xTimerStop(xWashTimer)            ← cần
```

Rule: gọi `xTimerStop()` khi và chỉ khi một signal khác kết thúc phase trước
khi timer tự fire.

---

### What I Should Learn From Phase 7

1. `vTaskDelay` là "ngủ mù" — không thể interrupt. Software timer + `xEventGroupWaitBits` là "ngủ thức tỉnh" — phản ứng được với bất kỳ event nào.
2. Timer callback là shared resource — mọi callback trong hệ thống chạy tuần tự trong Timer Service Task. Một callback block = toàn bộ timer system treo.
3. Callback chứa ít code nhất có thể: chỉ set bits, không logic.
4. One-shot cho timeout/deadline. Auto-reload cho periodic.
5. Luôn stop timer thủ công khi signal khác kết thúc phase trước timer fire.
6. `xTimerStart()` gửi command vào queue, không block WashingManager — timer bắt đầu chạy bất đồng bộ.

---

---

## Phase 8 — SEGGER SystemView Analysis

### Problem Being Solved

Hệ thống có 4 tasks, 3 queues, 4 timers chạy đồng thời. Không có tool thì
chỉ đoán: task nào chiếm CPU? Queue send/receive đúng thứ tự không? Timer
fire đúng lúc không? SystemView cho nhìn thấy toàn bộ trên timeline thời gian thực.

---

### How SystemView Works — SEGGER RTT

```
MCU (STM32)                        PC (SEGGER SystemView app)
    │                                        │
    │  RTT buffer (ring buffer trong RAM)    │
    │  ← FreeRTOS hooks ghi event vào đây   │
    │                                        │
    │◄────── ST-Link đọc trực tiếp RAM ─────│
    │        qua SWD — CPU không tham gia   │
```

**RTT vs UART:**

| | UART printf | SEGGER RTT |
|---|---|---|
| CPU overhead | Cao — chờ TX xong | ~0 — chỉ ghi RAM |
| Ảnh hưởng timing | Làm trễ task | Không đáng kể |
| Cần thêm pin | UART TX/RX | Không — dùng SWD sẵn có |

Một `printf` UART 115200 baud mất ~1ms cho 10 ký tự. RTT ghi RAM mất vài chục ns.

---

### Key APIs

**`SEGGER_SYSVIEW_Conf()`** — khởi tạo RTT buffer, đăng ký tên interrupt, setup MCU info. Gọi một lần trước `vTaskStartScheduler()`.

**`SEGGER_SYSVIEW_Start()`** — bắt đầu record events. Phải gọi trước scheduler để capture từ t=0.

**`configQUEUE_REGISTRY_SIZE`** — số queue/semaphore/mutex tối đa có thể đăng ký tên. Project dùng 3 queues → cần ≥ 3, set = 8.

**`vQueueAddToRegistry(handle, name)`** — gán tên cho queue handle. Nếu không gọi, SystemView hiển thị địa chỉ RAM thô (`0x20003A40`) thay vì `"ButtonQ"`.

---

### Expected Timeline

```
WashMgr  ████░░░░░░████░░░░░░░░░░░░░░  (blocked chờ command/event)
UITask   ░░░░████░░░░░░░░░░░░░░░░░░░░  (blocked chờ button)
Sensor   ░░░░░░░░░████░░░░░░░░░░████░  (100ms periodic)
OLED     ░░░░░░░░░░░░░████░░░░░░░░░░░  (blocked chờ display msg)
Tmr Svc  ░░░░░░░░░░░░░░░░░░░░░░░░░███  (rất ngắn khi timer fire)
IDLE     ████░░░░░░░░░░░░░░░░░░░░░░░░  (chạy khi không ai cần CPU)
```

---

### What I Should Learn From Phase 8

1. RTT tận dụng SWD debug interface — không cần UART, không làm chậm hệ thống.
2. `SEGGER_SYSVIEW_Start()` phải gọi trước `vTaskStartScheduler()` — code sau scheduler không chạy được.
3. `vQueueAddToRegistry()` không ảnh hưởng hoạt động queue — chỉ thêm tên cho debug tool.
4. Timer Service Task (`Tmr Svc`) hiện trên timeline — verify timer callback chạy đúng lúc.
5. IDLE task chạy khi tất cả tasks đều blocked — thời gian IDLE = CPU free time.

---

# PART II — Deep Dive Theo Khái Niệm (Mentoring)

> **Khác gì Part I?**
> Part I (Phase 1–8) ghi theo **tiến trình xây dựng project** — học tới đâu
> làm tới đó. Part II ghi theo **khái niệm**, và cố ý mở rộng ra **kiến thức
> tổng quát của chính khái niệm đó**: đầy đủ API, cơ chế bên trong, các
> pattern kinh điển ở domain khác, lỗi thường gặp, và bảng so sánh khi nào
> nên dùng cái gì.
>
> Mục tiêu Part II không phải "hiểu project này", mà là **"tự làm được
> project khác"**. Cập nhật sau mỗi buổi mentoring.

### ⚠️ Chuẩn kiểm chứng của Part II

Mọi khẳng định kỹ thuật trong Part II phải được **kiểm chứng bằng source
FreeRTOS thật trong repo** (`OS/FreeRTOS/`), không viết theo trí nhớ.

- **Phiên bản kernel:** FreeRTOS **V11.1.0** (`OS/FreeRTOS/include/task.h`,
  `tskKERNEL_VERSION_NUMBER`).
- Khẳng định nào có tham chiếu `file:dòng` là đã đối chiếu trực tiếp source.
- Khẳng định nào **chưa kiểm chứng được** phải ghi rõ *"chưa xác nhận"* —
  tuyệt đối không trình bày giả thuyết như sự thật.
- Con số hiệu năng lấy từ tài liệu chính thức phải **ghi rõ nguồn và ngữ
  cảnh gốc**, không suy rộng sang ngữ cảnh khác.

> Lưu ý về tính di động: một số chi tiết dưới đây (tên macro, điều kiện biên
> dịch, kích thước struct) **thay đổi giữa các phiên bản FreeRTOS**. Khi áp
> dụng sang project dùng kernel khác, phải kiểm tra lại source của phiên bản
> đó.

## Lộ trình mentoring

| # | Chủ đề | Trạng thái |
| --- | --- | --- |
| 1 | Task, Queue (cơ bản) | ✅ đã có từ trước (Part I) |
| 2 | Event Group | ✅ DD-01 |
| 3 | Software Timer + Timer Service Task | ✅ DD-04 (mục 1–29, đầy đủ F, G) |
| 4 | Task Notification | ⬜ |
| 5 | Semaphore & Mutex, priority inversion | ⬜ |
| 6 | Scheduler internals (priority, preemption, context switch, tick) | ✅ DD-05 |
| 7 | ISR design (NVIC, BASEPRI, FromISR, portYIELD_FROM_ISR) | ⬜ |
| 8 | Memory (heap_4, stack sizing, static vs dynamic, overflow) | ⬜ |
| 9 | Design patterns (state machine, layering, ownership) | 🔶 một phần ở DD-02 |
| 10 | Debug (SystemView, đọc trace, bug timing) | 🔶 một phần ở Part I Phase 8 |

**Điều kiện tốt nghiệp:** không phải học hết bảng trên, mà là qua được **bài
kiểm tra thiết kế** — nhận một đề bài mới hoàn toàn (thang máy, máy pha cà
phê, hệ tưới cây...) và tự thiết kế: bao nhiêu task, priority nào, dùng
primitive gì ở đâu, vẽ state machine — rồi được review như một junior
engineer thật.

## Mục lục Part II

| Mã | Khái niệm | Buổi |
| --- | --- | --- |
| DD-01 | Event Group — toàn tập | 1 |
| DD-02 | Guard Condition & thiết kế State Machine | 1 |
| DD-03 | Sticky / Stale Level Flag | 1 |
| DD-04 | Software Timer (mục 1–14 · dùng API) | 1 |
| DD-04 (tiếp) | Software Timer — nội tại, pattern, debug (mục 15–26) | 2 |
| DD-05 | Tick, SysTick & Context Switch | 2 |
| DD-04 (khép) | Software Timer — test (mục 27), so sánh hệ khác (mục 28), ranh giới test/Bug #17 (mục 29) | 2 |

> **Lưu ý về trạng thái ✅:** một chủ đề chỉ được đánh dấu hoàn thành khi
> người học xác nhận, không phải khi mình thấy "đã viết đủ dài". DD-04 từng bị
> đóng sớm ở mục 14 rồi phải mở lại — xem ghi chú đầu phần "DD-04 (tiếp)".

---

## DD-01 — Event Group (Buổi 1 · 2026-08-23)

### 1. Bản chất bên trong

Event Group thực chất chỉ là **một biến bitmask + một danh sách task đang chờ**:

Nguồn: `OS/FreeRTOS/event_groups.c:54-65`.

```c
typedef struct EventGroupDef_t {
    EventBits_t uxEventBits;              /* chính là các cờ, 1 biến duy nhất */
    List_t      xTasksWaitingForBits;     /* danh sách task đang Blocked      */

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxEventGroupNumber;   /* chỉ để trace tool nhận diện      */
    #endif
    #if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
        uint8_t ucStaticallyAllocated;    /* để biết có được free hay không   */
    #endif
} EventGroup_t;
```

Mọi tính chất khác đều **suy ra được** từ cấu trúc này:

- Không có buffer → **không lưu lịch sử, không đếm được**.
- Set bit = phép `|=` → **idempotent**, set 2 lần = set 1 lần (`1 OR 1 = 1`).
- Rất nhẹ, và **kích thước không phụ thuộc** số bit hay số task chờ.

**Kích thước thực tế trên Cortex-M4** (tính từ `list.h`: `List_t` =
`uxNumberOfItems`(4) + `pxIndex`(4) + `MiniListItem_t`(12) = **20 bytes**):

| Thành phần | Bytes |
| --- | --- |
| `uxEventBits` | 4 |
| `xTasksWaitingForBits` (`List_t`) | 20 |
| `uxEventGroupNumber` (nếu `configUSE_TRACE_FACILITY == 1`) | 4 |
| `ucStaticallyAllocated` (nếu bật static alloc) | 1 → padding 4 |
| **Tổng — cấu hình của project này** (trace = 1) | **28** |

So với Queue: Queue còn tốn thêm `depth × item_size` cho vùng chứa data.

> `configUSE_EVENT_GROUPS` (mặc định `1` — `FreeRTOS.h:321-322`) phải bằng 1
> thì `xEventGroupCreate()` mới được biên dịch.

### 2. Số bit dùng được — con số phải nhớ

Số bit **không** phụ thuộc gì tới tick về mặt ý nghĩa, nhưng vì lý do cài đặt
nội bộ (event group tái dùng cơ chế list của kernel) nó bị quyết định bởi
`configTICK_TYPE_WIDTH_IN_BITS`. Nguồn:
`OS/FreeRTOS/include/event_groups.h:133-140`.

| `configTICK_TYPE_WIDTH_IN_BITS` | `EventBits_t` | Bit dùng được |
| --- | --- | --- |
| `TICK_TYPE_WIDTH_16_BITS` (0) | 16-bit | **8 bit** (bit 0 → 7) |
| `TICK_TYPE_WIDTH_32_BITS` (1) | 32-bit | **24 bit** (bit 0 → 23) |
| `TICK_TYPE_WIDTH_64_BITS` (2) | 64-bit | **56 bit** |

> ⚠️ **Tên macro đã đổi.** Từ FreeRTOS V11, tên chuẩn là
> `configTICK_TYPE_WIDTH_IN_BITS`. Tên cũ `configUSE_16_BIT_TICKS` **vẫn được
> hỗ trợ** và tự động ánh xạ sang tên mới (`FreeRTOS.h:66-78`), nhưng
> **không được định nghĩa cả hai** — kernel sẽ `#error`. Project này dùng tên
> cũ: `configUSE_16_BIT_TICKS = 0` → tương đương 32-bit tick → **24 bit dùng
> được**, đang xài 5/24.

**8 bit cao kernel giữ** làm cờ điều khiển nội bộ — với tick 32-bit
(`event_groups.h:47-51`):

```c
#define eventCLEAR_EVENTS_ON_EXIT_BIT    ( ( uint32_t ) 0x01000000U )
#define eventUNBLOCKED_DUE_TO_BIT_SET    ( ( uint32_t ) 0x02000000U )
#define eventWAIT_FOR_ALL_BITS           ( ( uint32_t ) 0x04000000U )
#define eventEVENT_BITS_CONTROL_BYTES    ( ( uint32_t ) 0xff000000U )
```

Lấn vào đó không phải "hành vi không xác định" một cách mơ hồ — kernel bắt
được bằng `configASSERT()` ngay đầu `WaitBits`/`SetBits`/`ClearBits`
(`event_groups.c:197, 328, 483`):

```c
configASSERT( ( uxBitsToWaitFor & eventEVENT_BITS_CONTROL_BYTES ) == 0 );
```

### 3. API đầy đủ

| API | Gọi từ ISR? | Ghi chú |
| --- | --- | --- |
| `xEventGroupCreate()` / `...Static()` | ❌ | Static = không dùng heap |
| `xEventGroupSetBits()` | ❌ | Giá trị trả về **không đáng tin** (mục 8) |
| `xEventGroupSetBitsFromISR()` | ✅ | **Deferred** — mục 5 |
| `xEventGroupClearBits()` | ❌ | Trả về giá trị **trước khi** xóa |
| `xEventGroupClearBitsFromISR()` | ✅ | Cũng deferred |
| `xEventGroupWaitBits()` | ❌ **không bao giờ** | Vì có blocking |
| `xEventGroupGetBits()` | ❌ | Đọc tức thì, không block |
| `xEventGroupGetBitsFromISR()` | ✅ | Đọc tức thì, an toàn |
| `xEventGroupSync()` | ❌ | **Rendezvous/barrier** — mục 6 |
| `vEventGroupDelete()` | ❌ | Task đang chờ bị unblock |

### 4. Đặc tính quan trọng nhất — Broadcast

```
Queue        : 1 item → CHỈ MỘT task nhận được (unicast, item bị tiêu thụ)
Event Group  : set 1 bit → TẤT CẢ task đang chờ bit đó đều thức (broadcast)
```

5 task cùng `WaitBits(EVT_WIFI_CONNECTED)` → chỉ cần **một** lần `SetBits` là
cả 5 cùng unblock. Với Queue phải gửi 5 lần.

→ Đây là lý do Event Group là công cụ chuẩn cho **"thông báo trạng thái hệ
thống cho nhiều bên quan tâm"**.

⚠️ Broadcast + `xClearOnExit = pdTRUE` là cặp nguy hiểm: task đến chờ **muộn
hơn một chút** sẽ mất sự kiện vĩnh viễn (lost wakeup). Quy tắc: **nhiều
waiter → dùng `pdFALSE`, và chỉ định MỘT chủ thể duy nhất chịu trách nhiệm
`ClearBits`**.

### 5. Gotcha: `SetBitsFromISR` KHÔNG set ngay

```c
xEventGroupSetBitsFromISR(xEvents, EVT_DATA_READY, &xHigherPriorityTaskWoken);
/* ← Bit CHƯA được set tại dòng này! */
```

Thao tác set bit + duyệt danh sách task chờ là **unbounded time**, không được
phép làm trong ISR. FreeRTOS đẩy sang **Timer Service Task** qua timer command
queue:

```
ISR → gửi lệnh vào timer command queue → (ISR kết thúc)
                                       ↓
                      Timer Service Task chạy → giờ mới thực sự set bit
```

**Bằng chứng từ source** (`event_groups.c:812-829`) — toàn bộ thân hàm chỉ
là một lời gọi chuyển tiếp:

```c
BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                      const EventBits_t uxBitsToSet,
                                      BaseType_t * pxHigherPriorityTaskWoken )
{
    BaseType_t xReturn;
    xReturn = xTimerPendFunctionCallFromISR( vEventGroupSetBitsCallback,
                                             ( void * ) xEventGroup,
                                             ( uint32_t ) uxBitsToSet,
                                             pxHigherPriorityTaskWoken );
    return xReturn;   /* pdPASS nếu xếp được lệnh vào timer queue, pdFAIL nếu không */
}
```

Hệ quả bắt buộc nhớ:

1. **Có độ trễ** — bit chỉ được set khi daemon task được lên lịch chạy.
2. **Có thể THẤT BẠI** — timer command queue đầy → trả `pdFAIL`. **Luôn phải
   kiểm tra return value**, khác với `xQueueSendFromISR`.
3. **Yêu cầu BA điều kiện biên dịch** (`event_groups.c:812`), không phải hai:

   ```c
   #if ( ( configUSE_TRACE_FACILITY == 1 ) && \
         ( INCLUDE_xTimerPendFunctionCall == 1 ) && \
         ( configUSE_TIMERS == 1 ) )
   ```

   Việc phải bật `configUSE_TRACE_FACILITY` là một điểm kỳ quặc thật của
   kernel, dễ khiến người dùng bối rối khi hàm "biến mất".

4. Nhiều codebase production **tránh hẳn** hàm này, thay bằng **Task
   Notification** — gọi trực tiếp từ ISR, không qua daemon, không thể fail
   vì lý do "queue đầy".

> 🔍 **Kiểm chứng cho chính project này:** `FreeRTOSConfig.h` có
> `configUSE_TRACE_FACILITY = 1` và `configUSE_TIMERS = 1`, nhưng **không hề
> định nghĩa** `INCLUDE_xTimerPendFunctionCall`, mà mặc định của nó là **0**
> (`FreeRTOS.h:283-285`). Nghĩa là **`xEventGroupSetBitsFromISR()` hiện KHÔNG
> được biên dịch trong project này** — gọi nó sẽ lỗi lúc link. Đây cũng là
> một lý do nữa cho thấy comment cũ trong `app_events.h` (nói timer callback
> dùng hàm này) là sai.

### 6. `xEventGroupSync()` — pattern rendezvous / barrier

Bài toán: N task phải **cùng đến một điểm** rồi mới được đi tiếp.

```c
#define TASK_A_READY  (1 << 0)
#define TASK_B_READY  (1 << 1)
#define TASK_C_READY  (1 << 2)
#define ALL_READY     (TASK_A_READY | TASK_B_READY | TASK_C_READY)

/* Trong task A: */
xEventGroupSync(xEvents,
                TASK_A_READY,   /* bit tôi set  — "tôi xong rồi"     */
                ALL_READY,      /* bit tôi chờ  — "chờ tất cả xong"  */
                portMAX_DELAY);
/* Tới đây: cả 3 task cùng chạy tiếp, bit tự động clear sạch */
```

`Sync` = `SetBits` + `WaitBits` **nguyên tử**. Không thể tự ghép 2 hàm rời để
đạt hiệu quả tương đương (sẽ có race condition).

### 7. `WaitBits` vs `GetBits` — phân biệt sống còn

| | `xEventGroupWaitBits()` | `xEventGroupGetBits()` |
| --- | --- | --- |
| Câu hỏi nó trả lời | *"Tôi **đợi cho tới khi** điều kiện đúng"* | *"**Ngay lúc này** đúng hay sai?"* |
| Blocking | Có — task rời CPU | Không — trả về tức thì |
| Dùng cho | Chờ sự kiện sẽ đến | **Guard / precondition check** |
| Task state | Running → Blocked → Ready | Vẫn Running suốt |

Dùng `WaitBits` làm guard là **anti-pattern**: task bị block sẽ không thể báo
cho user biết **tại sao** nó từ chối → hệ thống trông như bị treo. Xem DD-02.

### 8. Ngữ nghĩa giá trị trả về

**`WaitBits` — cách phát hiện timeout.** Không có mã lỗi riêng; phải tự kiểm
tra bit:

```c
EventBits_t bits = xEventGroupWaitBits(eg, EVT_WATER_FULL,
                                       pdTRUE, pdFALSE,
                                       pdMS_TO_TICKS(5000));
if ((bits & EVT_WATER_FULL) != 0) { /* thành công */ }
else                              { /* TIMEOUT   */ }
```

Hai điểm dễ sai:

- Với `xWaitForAllBits = pdTRUE` (AND), phải so `(bits & WANTED) == WANTED`,
  không phải `!= 0`.
- Giá trị trả về là bits **trước khi** clear-on-exit xóa → vẫn đọc được.

**Idiom cần thuộc:**

```c
(bits & mask) != 0      /* OR  — có ít nhất 1 bit  */
(bits & mask) == mask   /* AND — có đủ tất cả bit  */
```

**`SetBits` — giá trị trả về KHÔNG đáng tin.** Tài liệu chính thức
(`event_groups.h:485-493`) nêu **hai** lý do bit vừa set có thể đã biến mất
khỏi giá trị trả về:

1. Việc set bit làm một task đang chờ thoát Blocked, và task đó dùng
   `xClearOnExit = pdTRUE` → **bit bị xóa tự động**.
2. Bất kỳ task nào có **priority cao hơn** task gọi `SetBits` mà được unblock
   (hoặc đang Ready) sẽ chạy và **có thể đổi giá trị event group** trước khi
   `SetBits` kịp return.

→ Coi giá trị trả về của `SetBits` như `void`.

### 9. Nhiều task cùng chờ + clear-on-exit — chi tiết cơ chế

Đây **không phải suy đoán** — đọc thẳng `xEventGroupSetBits()` trong
`event_groups.c` thấy rõ trình tự:

```c
vTaskSuspendAll();                              /* ① khoá scheduler        */
{
    pxEventBits->uxEventBits |= uxBitsToSet;    /* ② set bit TRƯỚC         */

    while( pxListItem != pxListEnd )            /* ③ duyệt HẾT task đang chờ */
    {
        ...
        if( điều kiện thỏa )
        {
            if( clear-on-exit )
                uxBitsToClear |= uxBitsWaitedFor;   /* chỉ GOM lại, chưa xóa */

            vTaskRemoveFromUnorderedEventList( ... );  /* unblock task này   */
        }
        pxListItem = pxNext;
    }

    pxEventBits->uxEventBits &= ~uxBitsToClear;  /* ④ giờ mới XÓA, sau vòng lặp */
}
( void ) xTaskResumeAll();                       /* ⑤ mở khoá → preempt ở đây  */
```

Hai kết luận quan trọng rút ra từ trình tự này:

- **Bước ③ chạy hết trước bước ④** → mọi task đang chờ **đều được unblock**,
  broadcast không bị mất dù có clear-on-exit. Nhưng task đến chờ **sau** bước
  ④ thì mất sự kiện vĩnh viễn (lost wakeup).
- **Toàn bộ nằm trong `vTaskSuspendAll()`/`xTaskResumeAll()`** → không task
  nào thực sự chạy ở giữa vòng lặp. Việc chuyển sang task priority cao hơn
  chỉ xảy ra tại `xTaskResumeAll()` (bước ⑤). Đây là lý do thao tác này an
  toàn dù đụng vào danh sách task đang chờ.

#### C.1 — Nhiều consumer cùng "chịu trách nhiệm" một bit: ai được phép clear?

> ⚠️ Phần khung quyết định 4 bước dưới đây là **suy luận kỹ thuật của mình**,
> không phải một API hay rule được đặt tên chính thức trong tài liệu
> FreeRTOS. Đã đối chiếu với 3 nguồn để tránh nói sai phần cơ chế nền:
> `OS/FreeRTOS/event_groups.c` (source thật trong repo), trang API chính thức
> [`xEventGroupWaitBits()`](https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/04-xEventGroupWaitBits),
> và [`xEventGroupSync()`](https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/11-xEventGroupSync).

**Đính chính một điểm mình từng nói sai ở buổi trước:** dùng
`xClearOnExit = pdTRUE` trong chính lời gọi `WaitBits()` của một waiter
**không hề gây race**, kể cả khi nhiều waiter cùng khớp một lần `SetBits()`.
Tài liệu chính thức nói rõ (nguyên văn từ trang API):

> *"If xClearOnExit is set to pdTRUE then any bits set in the value passed as
> the uxBitsToWaitFor parameter will be cleared in the event group before
> xEventGroupWaitBits() returns..."*

Và đúng như đã trace ở mục C: việc "gom bit cần xóa rồi xóa một lần" xảy ra
**bên trong `vTaskSuspendAll()`/`xTaskResumeAll()`**, tức là **trước khi bất
kỳ waiter nào thực sự được chạy trở lại**. Không có waiter nào "chạy nhanh
hơn" waiter khác ở bước này — tất cả được xử lý xong bởi kernel trước khi ai
được resume. Nên **nếu nhiều task cùng gọi `WaitBits(..., pdTRUE, ...)` trên
cùng bit, đó là cách dùng an toàn và đúng chuẩn**, không cần "chọn 1 chủ thể".

**Vậy câu hỏi "ai chịu trách nhiệm clear" áp dụng cho trường hợp nào?** Chỉ
áp dụng khi có một lời gọi `xEventGroupClearBits()` **độc lập, tách rời**
khỏi bất kỳ `WaitBits()` nào — tức là một hành động "dọn dẹp vòng đời" do
application tự quyết định thực hiện lúc nào, không phải cơ chế tự động của
kernel. Đây chính xác là trường hợp `Manager_OnEnterIdle()`:

```c
xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);
```

Lời gọi này **không nằm trong** một `WaitBits()` — nó là code ứng dụng chủ
động quyết định "giờ là lúc dọn sạch 2 bit này". Ở đây mới thực sự có câu
hỏi **"ai được phép làm việc dọn dẹp này"**, vì không có cơ chế kernel nào tự
động đảm bảo đúng lúc cho loại lời gọi này — và đây là chỗ khung quyết định
dưới đây áp dụng.

**Khung quyết định — 4 bước (cho việc gọi `ClearBits()` độc lập):**

1. **Bit là CONDITION (level) hay OCCURRENCE (broadcast một lần)?**
   Condition có transition ngược tự nhiên (`EVT_DOOR_CLOSED`); Occurrence thì
   không (`EVT_FINISHED`).

2. **Nếu CONDITION** → chủ thể là **producer**, clear đối xứng (quy tắc
   DD-03, không đổi gì khi có nhiều waiter).

3. **Nếu OCCURRENCE, kiểm tra có phải BARRIER thật** (mọi bên vừa set phần
   mình vừa chờ đủ N phần, đối xứng)?
   - Có → dùng **`xEventGroupSync()`**, kernel tự clear, câu hỏi "ai clear"
     biến mất.
   - Không (1 producer, N consumer thụ động) → bước 4.

4. **1 producer, N consumer thụ động** → chủ thể clear là **producer đó**,
   **với điều kiện producer là DUY NHẤT**. Nếu có ≥2 producer cho cùng một
   bit → không có lựa chọn an toàn bằng cách "chọn 1 trong số họ" — phải tách
   bit theo nguyên nhân, hoặc gán quyền sở hữu ở cấp kiến trúc (mở rộng
   nguyên tắc ownership sẵn có của project, xem dưới).

**Đối chứng bằng chính code project — `EVT_FINISHED` vs `EVT_ERROR`:**

Cả hai đều được clear tại cùng một chỗ, `Manager_OnEnterIdle()`:

```c
xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);
```

Nhưng số phận khác nhau:

| Bit | Số producer | Kết quả |
| --- | --- | --- |
| `EVT_FINISHED` | **1** (chỉ `WashingManager`) | An toàn — đúng bước 4, producer duy nhất tự nhiên là chủ thể clear |
| `EVT_ERROR` | **2** (`SensorTask` + `WashingManager`) | **Bug #17** — không ai trong 2 bên dám tự tin clear mà không giẫm chân bên kia |

→ Khác biệt duy nhất giữa "đúng" và "Bug #17" là **số lượng producer trên
cùng một bit**. Đây là bằng chứng cụ thể nhất cho quy tắc ở bước 4.

**Bảng quyết định tổng hợp:**

| Tình huống | Chủ thể clear | Công cụ |
| --- | --- | --- |
| Bit là level, có true/false tự nhiên | Producer, đối xứng set/clear | `SetBits`/`ClearBits` thường |
| N bên vừa set vừa chờ nhau (barrier thật) | **Kernel** | `xEventGroupSync()` |
| 1 producer, N consumer thụ động | Producer đó | `ClearBits` khi khởi động lại vòng đời |
| N producer, không phải barrier | **Không có lựa chọn an toàn** bằng cách chọn 1 trong số họ | Tách bit theo nguyên nhân, hoặc gán ownership kiến trúc |
| N delivery độc lập, mỗi consumer cần bản riêng | *(không phải bài toán của Event Group)* | Task Notification × N, hoặc Queue riêng mỗi consumer |

### 10. Cấp phát tĩnh

```c
static StaticEventGroup_t xEventGroupBuffer;   /* nằm ở .bss, không dùng heap */
EventGroupHandle_t eg = xEventGroupCreateStatic(&xEventGroupBuffer);
```

Vì sao quan trọng: ô tô, y tế, hàng không (MISRA-C, DO-178C, IEC 61508)
thường **cấm cấp phát động hoàn toàn**. Mọi object FreeRTOS đều có bản Static
(`xQueueCreateStatic`, `xTaskCreateStatic`, `xTimerCreateStatic`...), bật bằng
`configSUPPORT_STATIC_ALLOCATION = 1`. Lợi ích phụ: biết chính xác RAM lúc
**compile-time**, không bao giờ fail lúc runtime.

### 11. Task Notification dùng thay Event Group — quyết định kiến trúc

Task Notification có thể hoạt động như **một event group 32-bit riêng của
từng task**:

```c
/* Bên gửi — thay cho xEventGroupSetBits() */
xTaskNotify(xManagerTaskHandle, EVT_WATER_FULL, eSetBits);

/* Bên nhận — thay cho xEventGroupWaitBits() */
uint32_t bits;
xTaskNotifyWait(0x00,        /* bit clear khi VÀO                        */
                ULONG_MAX,   /* bit clear khi RA (tương đương clear-on-exit) */
                &bits,       /* giá trị nhận được                        */
                portMAX_DELAY);
```

| | Event Group | Task Notification |
| --- | --- | --- |
| Số task nhận được | **Nhiều** (broadcast) | **Chỉ 1** (địa chỉ tới đúng 1 task) |
| Chờ tổ hợp AND | ✅ có sẵn (`xWaitForAllBits`) | ❌ phải tự viết vòng lặp |
| Chi phí RAM | 28 B cho object dùng chung | **Không cần object riêng** — nằm sẵn trong TCB của **mỗi** task |
| Gọi từ ISR | Deferred qua daemon, **có thể `pdFAIL`** | **Trực tiếp**, không qua daemon |
| Tốc độ | Chậm hơn | Nhanh hơn (xem ghi chú) |

**Chi phí RAM — con số chính xác.** Notification không phải "0 byte". Mỗi TCB
luôn mang sẵn (`tasks.c:419-420`):

```c
volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
volatile uint8_t  ucNotifyState [ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
```

Với mặc định `configTASK_NOTIFICATION_ARRAY_ENTRIES = 1`
(`FreeRTOS.h:2782-2784`) → **5 bytes (padding lên 8) trong MỖI TCB**, dù task
đó có dùng notification hay không. Nghĩa là: notification rẻ hơn khi chỉ có 1
consumer, nhưng chi phí là **cố định theo số task**, không phải bằng 0.
`configUSE_TASK_NOTIFICATIONS` mặc định `1` (`FreeRTOS.h:2778-2780`); đặt `0`
sẽ bỏ 2 trường này khỏi TCB.

> ⚠️ **Về con số "nhanh hơn 45%" — đã xác minh với nguồn chính thức.** Trang
> [FreeRTOS Direct-to-task-notifications — As binary semaphore](https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/02-As-binary-semaphore)
> viết nguyên văn:
>
> *"Unblocking an RTOS task with a direct notification is 45% faster and
> uses less RAM than unblocking a task with a binary semaphore."*
>
> Đây là con số **có thật, chính thức**, nhưng so sánh với **BINARY
> SEMAPHORE**, không phải Event Group. FreeRTOS **không công bố** con số
> tương ứng khi so với Event Group, và con số này **không xuất hiện trong
> source kernel** (`event_groups.c`/`timers.c`) — nó tới từ đo đạc riêng của
> đội FreeRTOS, không tự suy ra được từ đọc code. Vì vậy khi so Event Group
> với Task Notification, chỉ nên nói "notification nhanh hơn vì đường đi
> ngắn hơn — không cần cấp phát object, không duyệt danh sách waiter, ISR
> không phải qua daemon task" — **đừng trích số 45% cho phép so sánh này.**

**Quy tắc chọn:**

- Chỉ **một** task quan tâm → **Task Notification** (nhanh, nhẹ, an toàn từ ISR).
- **Nhiều** task quan tâm, hoặc cần chờ AND → **Event Group**.

Áp vào project: `EVT_WATER_FULL`, `EVT_TIMEOUT`, `EVT_ERROR` chỉ có
WashingManager đọc → về kỹ thuật **có thể thay bằng Task Notification**. Dùng
Event Group ở đây vẫn hợp lý vì (a) `EVT_FINISHED` thiết kế cho nhiều
consumer tương lai, (b) đây là project **học FreeRTOS**, (c) chênh lệch hiệu
năng ở tần suất 10–20 giây/sự kiện là vô nghĩa.

### 12. Debug Event Group

```c
EventBits_t now = xEventGroupGetBits(xSystemEvents);   /* đặt vào Live Expressions */
```

Mẹo thực dụng: khi hệ thống "đứng im không rõ lý do", việc **đầu tiên** nên
làm là pause debugger và đọc giá trị event group. Với Bug #17, làm vậy sẽ
thấy ngay `EVT_ERROR = 1` dù cửa đang đóng — chẩn đoán xong trong 10 giây
thay vì phải đọc code.

### 13. Use case kinh điển ngoài project này

| Domain | Cách dùng |
| --- | --- |
| **Network stack** (lwIP, ESP-IDF) | `WIFI_CONNECTED_BIT`, `IP_GOT_BIT`, `WIFI_FAIL_BIT` — nhiều task chờ mạng sẵn sàng |
| **Power management** | `SLEEP_REQUEST`, `PERIPH_A_IDLE`, `PERIPH_B_IDLE` — dùng AND (`pdTRUE`) để chỉ ngủ khi **mọi** ngoại vi rảnh |
| **System init barrier** | `xEventGroupSync()` — mọi driver init xong mới cho application chạy |
| **Multi-sensor fusion** | Chờ **đủ cả** accel + gyro + mag có mẫu mới (AND) rồi mới tính |
| **Graceful shutdown** | Broadcast `SHUTDOWN_REQ`, rồi chờ AND của các bit `TASK_x_STOPPED` |

Pattern chung: **OR** (`pdFALSE`) cho *"cái nào tới trước cũng được"*, **AND**
(`pdTRUE`) cho *"phải đủ hết mới đi tiếp"*.

### 14. Bảng quyết định — khi nào dùng primitive nào

| Nhu cầu | Công cụ đúng | Vì sao |
| --- | --- | --- |
| Truyền **dữ liệu** | **Queue** | Event Group chỉ mang true/false |
| Báo hiệu **trạng thái level** | **Event Group** | Idempotent, đọc bao nhiêu lần cũng được |
| **Đếm** số lần xảy ra | **Counting Semaphore** / counter + critical section | Event Group mất thông tin số lượng |
| Đánh thức **đúng 1 task**, cực nhanh | **Task Notification** | Nhanh nhất, ít RAM nhất |
| Đánh thức **nhiều task** cùng lúc | **Event Group** | Duy nhất hỗ trợ broadcast tự nhiên |
| Chờ **tổ hợp AND** | **Event Group** (`pdTRUE`) | Không primitive nào khác làm gọn được |
| Bảo vệ **tài nguyên dùng chung** | **Mutex** | Có priority inheritance |
| **Barrier** cho N task | **`xEventGroupSync()`** | Nguyên tử, không tự ghép được |

### 15. Lỗi kinh điển

1. Dùng Event Group để **truyền data** — nó chỉ mang true/false theo từng bit.
2. Dùng để **đếm** — không đếm được, mất thông tin số lượng.
3. Dùng `WaitBits` (blocking) làm **guard** — xem DD-02.
4. Tin vào **giá trị trả về của `SetBits`**.
5. Không kiểm tra return của **`SetBitsFromISR`** (có thể `pdFAIL`).
6. Cờ level duy trì **bất đối xứng** → sticky flag, xem DD-03.
7. Nhiều waiter + `clear-on-exit = pdTRUE` → lost wakeup.

### 16. Câu hỏi phỏng vấn

1. *"Event Group khác Semaphore ở điểm nào?"* → Semaphore đếm được và
   unicast (1 task lấy 1 token); Event Group là level-based, broadcast, hỗ
   trợ chờ tổ hợp AND/OR.
2. *"Tại sao `xEventGroupSetBitsFromISR` có thể fail còn `xQueueSendFromISR`
   thì không (khi queue chưa đầy)?"* → Vì nó là deferred operation qua timer
   command queue.
3. *"Có bao nhiêu bit dùng được và tại sao?"* → 24 (hoặc 8), do 8 bit cao
   dành cho kernel control flags.

---

## DD-02 — Guard Condition & Thiết kế State Machine (Buổi 1 · 2026-08-23)

> Nguồn: **Guard condition** là thuật ngữ chính thức trong đặc tả UML State
> Machine (Harel statechart). Định nghĩa chuẩn: *"a condition that has to be
> met in order to enable the transition to which it belongs"*, được đánh giá
> đúng một lần tại thời điểm event xảy ra, và ký hiệu chuẩn trên diagram là
> `event [guard] / action`. Đây không phải thuật ngữ riêng của FreeRTOS hay
> project này — nó tồn tại độc lập với mọi RTOS, áp dụng cho bất kỳ state
> machine nào (kể cả `washing_sm.c` dù không dùng UML tooling).

### 1. FSM đầy đủ có 4 thành phần, không phải 2

Bảng transition của project hiện tại:

```
(current_state, event) → next_state
```

Dạng đầy đủ theo lý thuyết FSM / UML statechart:

```
(current_state, event, [guard]) → next_state / action
                        ↑
              điều kiện boolean phải ĐÚNG thì transition mới được phép nổ
```

Ký hiệu chuẩn UML cho cùng ý nghĩa: `event [guard] / action`.

**Guard** = "cổng gác". Event tới, nhưng guard sai → transition **không xảy
ra**, state giữ nguyên, event bị từ chối.

Ví dụ đời thường: thang máy nhận lệnh "đi tầng 5" (event), nhưng guard
`cửa_đã_đóng == true` sai → lệnh bị từ chối.

### 2. Guard đặt ở đâu — nguyên tắc

**Guard phải đặt ở chỗ mọi đường đi đều bắt buộc chạy qua**, ngay tại điểm
quyết định transition, **trước khi bất kỳ action nào chạy**.

Ví dụ sai trong project: nếu đặt guard "cửa đóng" trong
`case WASH_STATE_FILL_WATER` thì:

- **Quá muộn** — `Manager_OnEnterFillWater()` đã chạy rồi, van đã mở, nước
  đang chảy.
- **Bỏ sót nhánh** — `IDLE → SPIN` không đi qua `FILL_WATER`, nên chế độ vắt
  hoàn toàn không bị kiểm tra. Lỗ hổng an toàn thật (hiện project vẫn đang
  có lỗ hổng này).

Chỗ đúng là `Manager_HandleCommand()` — nơi duy nhất **cả WASH lẫn SPIN** đều
bắt buộc đi qua:

```c
const EventBits_t required = EVT_DOOR_CLOSED | EVT_FILTER_OK;
EventBits_t bits = xEventGroupGetBits(xSystemEvents);   /* KHÔNG block */

if ((bits & required) != required) {
    Manager_PostDisplay(currentState, "CLOSE DOOR");
    return;                        /* từ chối, KHÔNG đổi state */
}
```

### 3. Guard phải KHÔNG BLOCK — "fail fast with feedback"

Nếu dùng `WaitBits(..., portMAX_DELAY)` làm guard:

```
User bấm Start → Manager block vô hạn
                 → không xử lý được lệnh nào khác
                 → không gửi được message lên OLED
                 → user không biết TẠI SAO máy không chạy
                 → user tưởng máy hỏng
```

Nguyên tắc UX embedded: **từ chối ngay, và nói rõ lý do** — thay vì im lặng
chờ. Đây là lý do guard phải dùng `GetBits` (non-blocking), không phải
`WaitBits`.

### 4. Ba loại điều kiện — đừng nhầm lẫn

| Loại | Định nghĩa | Kiểm tra khi nào | Ví dụ máy giặt |
| --- | --- | --- | --- |
| **Precondition (guard)** | Đúng **trước khi** cho phép bắt đầu | Tại điểm quyết định transition | Cửa đóng + nắp lọc OK trước khi chạy |
| **Invariant** | Đúng **suốt** thời gian ở state đó | Liên tục, song song (monitor) | Cửa luôn đóng trong lúc motor quay |
| **Postcondition** | Đúng **sau khi** hoàn thành | Khi thoát state | Nước đã xả hết trước khi vắt |

Project hiện tại xử lý cửa như **invariant** (mở van ngay, rồi giám sát song
song qua `EVT_ERROR` trong `WaitBits`) nhưng **thiếu guard**. Một hệ thống
tốt thường cần **cả hai**: guard chặn lúc bắt đầu, invariant canh chừng lúc
đang chạy.

> Đây cũng là hiểu nhầm phổ biến khi đọc code lần đầu: tưởng code kiểm tra
> điều kiện **tuần tự kiểu if-else** rồi mới hành động, trong khi thực tế nó
> **hành động ngay + canh chừng song song**. Hai mô hình khác bản chất.

### 5. Level flag vs Edge event

| | Level | Edge |
| --- | --- | --- |
| Ý nghĩa | "Điều kiện **đang** đúng/sai" | "Sự kiện **vừa** xảy ra" |
| Được set/clear | Đối xứng, liên tục theo trạng thái vật lý | Set một lần khi xảy ra |
| Dùng làm **guard** | ✅ **Bắt buộc phải là level** | ❌ Không có giá trị "ngay lúc này" |
| Dùng để **phản ứng** | Được, nhưng dễ lặp | ✅ Tự nhiên |
| Ví dụ | `EVT_DOOR_CLOSED` | "nút vừa được nhấn" |

Quy tắc: **guard cần level, phản ứng cần edge.**

---

## DD-03 — Sticky / Stale Level Flag (Buổi 1 · 2026-08-23)

> Ca thực tế đầy đủ: xem **Bug #17** trong `HARDWARE_BUGS.md`. Dưới đây là
> phần nguyên lý tổng quát để nhận ra nó ở project khác.

### Định nghĩa vấn đề

Một cờ **level** (biểu diễn trạng thái hiện tại) nhưng được duy trì **bất đối
xứng**: có code set, không có code clear tương ứng. Kết quả: cờ mô tả **quá
khứ** thay vì hiện tại — **nó nói dối**.

```c
/* Level flag ĐÚNG (đối xứng)        Level flag SAI (sticky) */
if (cond) set(BIT);                  if (cond) set(BIT);
else      clear(BIT);                /* thiếu else → bit dính */
```

### Vì sao rất khó phát hiện

1. **Không crash, không assert, không compiler warning** — hệ thống vẫn chạy,
   chỉ hành xử sai.
2. **Phụ thuộc lịch sử** — chỉ lộ ra sau một *chuỗi* thao tác cụ thể; test
   từng chức năng riêng lẻ không bao giờ thấy.
3. **Triệu chứng ở xa nguyên nhân** — bug ở module A, biểu hiện ở module B.
   → Phải trace theo **dữ liệu** (giá trị bit), không chỉ theo **luồng gọi hàm**.

### Ba cách sửa chuẩn (dùng được ở mọi project)

| Cách | Nội dung | Đánh giá |
| --- | --- | --- |
| **A** | Cho producer clear ở nhánh ngược lại | Đơn giản, nhưng **sai nếu bit có nhiều producer** — sẽ xóa nhầm tín hiệu của module khác (vi phạm ownership) |
| **B** | **Tách cờ theo nguyên nhân**, mỗi bit một chủ sở hữu duy nhất | ✅ Đúng nhất về thiết kế |
| **C** | Consumer **dọn cờ trước khi "vũ trang"** (clear ngay đầu entry action) | Rẻ nhất, nhưng chỉ xử lý cờ *cũ* — **không thay thế được guard** |

Lời giải đầy đủ thường là **B (hoặc C) + guard** — hai thứ giải quyết hai vấn
đề khác nhau: cờ đối xứng đảm bảo thông tin không cũ, guard chặn khởi động
khi điều kiện chưa thỏa.

### Quy tắc phòng tránh

1. Cờ level phải được duy trì **đối xứng ở mọi nhánh** của producer.
2. **Mỗi bit có đúng một chủ sở hữu** chịu trách nhiệm clear.
3. Không trộn ngữ nghĩa level và edge trong cùng một bit mà không ghi chú rõ.

---

## DD-04 — Software Timer (Buổi 1 · 2026-08-23)

### 1. Vấn đề nó giải quyết

Cần "làm gì đó sau X ms" hoặc "làm định kỳ". Cách ngây thơ là tạo hẳn một
task cho mỗi việc — nhưng **mỗi task tốn một stack riêng** (project này: 1 KB
cho WashMgr, 512 B cho UITask). Bốn timer = bốn task = 2–4 KB RAM chỉ để đếm
giờ, cộng TCB (~100 B mỗi task) và gánh nặng cho scheduler.

Software Timer giải quyết bằng cách: **tất cả timer dùng chung MỘT task duy
nhất**. Thêm timer thứ 100 cũng **không tốn thêm stack nào**.

**Kích thước control block — tính từ source** (`timers.c`, struct
`tmrTimerControl`), trên Cortex-M4:

| Thành phần | Bytes |
| --- | --- |
| `pcTimerName` | 4 |
| `xTimerListItem` (`ListItem_t`) | 20 |
| `xTimerPeriodInTicks` | 4 |
| `pvTimerID` | 4 |
| `pxCallbackFunction` | 4 |
| `uxTimerNumber` (nếu `configUSE_TRACE_FACILITY == 1`) | 4 |
| `ucStatus` | 1 → padding 4 |
| **Tổng — cấu hình của project này** | **44** |

So sánh: 4 timer ở đây tốn **176 bytes** tổng cộng, thay vì 2–4 KB nếu làm
bằng 4 task riêng.

### 2. Kiến trúc bên trong — hiểu cái này thì mọi hành vi tự suy ra

Software Timer **không phải phần cứng, không phải ngắt**:

```
┌──────────────┐   xTimerStart()    ┌──────────────────────┐
│ Task bất kỳ  │ ─────────────────► │ Timer Command Queue  │
│ hoặc ISR     │  (gửi LỆNH, không  │ configTIMER_QUEUE_   │
└──────────────┘   thực thi ngay!)  │ LENGTH slot          │
                                    └──────────┬───────────┘
                                               │ đọc lệnh
                                               ▼
                                    ┌──────────────────────┐
                                    │ Timer Service Task   │ ← MỘT TASK THẬT
                                    │ tên: "Tmr Svc"       │   có stack riêng
                                    │ - xử lý lệnh         │
                                    │ - quản 2 danh sách   │
                                    │   sắp theo hạn nổ    │
                                    │ - GỌI CALLBACK       │ ← callback chạy Ở ĐÂY
                                    └──────────────────────┘
```

Task này kernel **tự tạo** bên trong `vTaskStartScheduler()` khi
`configUSE_TIMERS = 1` — không thấy trong `main.c` nhưng nó có thật. Tên mặc
định `"Tmr Svc"`, đổi được qua `configTIMER_SERVICE_TASK_NAME`
(`timers.c:63-65`). Đây chính là tên bạn sẽ thấy trên timeline của SEGGER
SystemView.

Cấu hình của project này:

```c
#define configUSE_TIMERS              1
#define configTIMER_TASK_PRIORITY     ( 2 )     /* ngang UITask/SensorTask */
#define configTIMER_QUEUE_LENGTH      10
#define configTIMER_TASK_STACK_DEPTH  ( configMINIMAL_STACK_SIZE * 2 )
```

### 3. Hệ quả #1 — Mọi lệnh timer đều BẤT ĐỒNG BỘ

`xTimerStart/Stop/Reset/ChangePeriod` **không thao tác lên timer**. Chúng chỉ
**gửi message** vào command queue rồi trả về ngay.

```c
xTimerStop(xFillTimeoutTimer, 0);
/* ← Tại dòng này timer VẪN ĐANG CHẠY.
     Nó chỉ dừng khi Timer Service Task được lên lịch và đọc lệnh. */
```

→ Đây là gốc rễ của rủi ro "timeout của state cũ bắn nhầm sang state mới":
không phải code viết ẩu, mà là **bản chất kiến trúc**.

Tham số thứ 2 **không phải** thời gian timer — nó là "chờ bao lâu nếu command
queue đang đầy":

```c
xTimerStart(xWashTimer, 0);      /* 0   = queue đầy thì trả pdFAIL NGAY  */
xTimerStart(xWashTimer, 100);    /* 100 = chờ tối đa 100 tick để có chỗ  */
```

**Chi tiết tinh tế cứu vãn độ chính xác — có bằng chứng source.** Khi gửi
lệnh START, FreeRTOS đính kèm **thời điểm phát lệnh** vào message. Bản thân
`xTimerStart` chỉ là một macro (`timers.h:507-508`):

```c
#define xTimerStart( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START, ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )
/*                                                        ↑ tick ĐƯỢC CHỤP TẠI ĐÂY */
```

Và khi daemon xử lý lệnh, nó tính hạn nổ từ **con tem thời gian đó**, không
phải từ "bây giờ" (`timers.c:1006`):

```c
prvInsertTimerInActiveList( pxTimer,
                            xMessage.u.xTimerParameters.xMessageValue + pxTimer->xTimerPeriodInTicks,
                            xTimeNow,
                            xMessage.u.xTimerParameters.xMessageValue );
```

→ Dù daemon xử lý trễ, timer vẫn tính giờ từ lúc *gọi* `xTimerStart()`. Độ
trễ chỉ ảnh hưởng *thời điểm callback chạy*, **không làm sai chu kỳ**.

### 4. Hệ quả #2 — Callback chạy trong TASK, không phải ISR

| Trong callback | Được phép? |
| --- | --- |
| API thường (`xEventGroupSetBits`, `xQueueSend`) | ✅ Có — và **phải** dùng bản thường |
| API `...FromISR()` | ❌ Sai — không ở trong ISR |
| `vTaskDelay()`, `xQueueReceive(..., portMAX_DELAY)` | ❌ **TUYỆT ĐỐI KHÔNG** |

### 5. Hệ quả #3 — Callback KHÔNG ĐƯỢC BLOCK (lỗi kinh điển số 1)

Vì **tất cả** timer dùng chung một task: callback của timer A block 2 giây →
callback của B, C, D **đều trễ 2 giây theo**. Một timer hỏng làm hỏng cả hệ
thống timer.

```c
/* ❌ SAI NGHIÊM TRỌNG */
void BadCallback(TimerHandle_t t) {
    vTaskDelay(pdMS_TO_TICKS(100));            /* chặn daemon 100ms       */
    HAL_I2C_Mem_Write(..., HAL_MAX_DELAY);     /* có thể chặn VĨNH VIỄN   */
}
```

Nguyên tắc vàng: **callback phải ngắn, không block, chỉ "báo hiệu" rồi
thoát** — giống hệt nguyên tắc viết ISR.

### 6. API đầy đủ

| API | Từ ISR? | Ghi chú |
| --- | --- | --- |
| `xTimerCreate()` / `...Static()` | ❌ | Static = không dùng heap |
| `xTimerStart()` | `...FromISR()` | Nếu đang chạy thì reset |
| `xTimerStop()` | `...FromISR()` | Dừng, không xóa |
| `xTimerReset()` | `...FromISR()` | Đặt lại về 0 và chạy tiếp |
| `xTimerChangePeriod()` | `...FromISR()` | **Tự động start** nếu đang dừng |
| `xTimerDelete()` | ❌ | Handle thành vô hiệu |
| `xTimerIsTimerActive()` | ❌ | Đang chạy hay không |
| `pvTimerGetTimerID()` / `vTimerSetTimerID()` | ❌ | Gắn dữ liệu riêng — mục 8 |
| `xTimerGetPeriod()` / `xTimerGetExpiryTime()` | ❌ | Truy vấn |
| `pcTimerGetName()` | ❌ | Tên đặt lúc create |
| `xTimerPendFunctionCall()` | `...FromISR()` | Chạy hàm bất kỳ trong daemon — mục 9 |

**`xTimerReset()` — pattern hay dùng thực tế:** làm "watchdog mềm" hoặc phát
hiện end-of-frame. Ví dụ: mỗi ký tự UART đến thì `xTimerReset()`; nếu 50 ms
không có ký tự nào nữa → callback nổ → "gói tin đã kết thúc". Đây là cách
chuẩn cho giao thức không có ký tự kết thúc.

### 7. One-shot vs Auto-reload, và vấn đề drift

```c
xTimerCreate(name, period, uxAutoReload, id, callback);
/*                         ↑ pdFALSE = one-shot : nổ 1 lần rồi tự dừng
                             pdTRUE  = auto-reload: nổ lặp lại mãi        */
```

**Điểm quan trọng về độ chính xác — có bằng chứng source.** Auto-reload lên
lịch lần kế tiếp dựa trên **thời điểm hết hạn trước đó**, không phải lúc
callback chạy (`timers.c:699-707`):

```c
static void prvReloadTimer( Timer_t * const pxTimer,
                            TickType_t xExpiredTime,
                            const TickType_t xTimeNow )
{
    while( prvInsertTimerInActiveList( pxTimer,
                                       ( xExpiredTime + pxTimer->xTimerPeriodInTicks ),
                                       /*   ↑ cộng từ HẠN CŨ, không phải từ xTimeNow */
                                       xTimeNow, xExpiredTime ) != pdFALSE )
    { ... }
}
```

Thêm một chi tiết củng cố: trong `prvProcessExpiredTimer()`, việc **reload
được làm TRƯỚC khi gọi callback** (`timers.c:735` reload, `:742-743`
callback) → callback chạy chậm cũng không đẩy lịch lần sau.

```
Auto-reload (period 100):     nổ tại 100, 200, 300, 400...  ← chuẩn
Tự start lại trong callback:  nổ tại 100, 203, 307, 412...  ← DRIFT tích lũy
```

→ Cần định kỳ chính xác thì **luôn dùng auto-reload**, đừng tự `xTimerStart()`
lại trong callback.

### 8. Timer ID — chia sẻ một callback cho nhiều timer

Hai cách chuẩn để phân biệt "timer nào vừa nổ":

```c
/* Cách 1 — so sánh handle (đơn giản, cần biến global) */
static void Callback(TimerHandle_t xTimer) {
    if      (xTimer == xWashTimer)  { ... }
    else if (xTimer == xDrainTimer) { ... }
}

/* Cách 2 — dùng Timer ID (linh hoạt hơn, không cần global) */
xTimerCreate("WashTmr", ..., (void *)PHASE_WASH, Callback);

static void Callback(TimerHandle_t xTimer) {
    uint32_t phase = (uint32_t)pvTimerGetTimerID(xTimer);
    switch (phase) { ... }
}
```

Timer ID còn có thể trỏ tới một `struct` chứa dữ liệu riêng của từng timer →
cho phép viết **một callback tổng quát dùng lại được**, rất phổ biến trong
driver/library.

Project này dùng cách thứ ba: **không phân biệt gì cả** — set chung
`EVT_TIMEOUT` rồi để WashingManager nhìn state hiện tại mà suy ra (TD-10).
Gọn, nhưng chính là gốc rễ rủi ro ở mục 3.

### 9. `xTimerPendFunctionCallFromISR()` — deferred interrupt processing

Cho phép ISR nói: *"chạy hàm này giùm tôi, nhưng trong ngữ cảnh task"*:

```c
/* Trong ISR — không làm việc nặng ở đây */
xTimerPendFunctionCallFromISR(MyHeavyWork, pvParam, ulParam, &xHigherPriorityTaskWoken);
/* → MyHeavyWork() chạy trong Timer Service Task, nơi được phép block */
```

Đây là pattern chuẩn để giữ ISR cực ngắn. Và đây cũng chính là cơ chế mà
`xEventGroupSetBitsFromISR()` dùng bên dưới — lý do nó "deferred và có thể
fail" (DD-01 mục 5).

### 10. So sánh 4 cách "đợi thời gian" — bảng cần thuộc

| Công cụ | Ngữ cảnh chạy | Độ chính xác | RAM thêm | Dùng khi |
| --- | --- | --- | --- | --- |
| **`vTaskDelay()`** | Task tự ngủ | Tick, **có drift** | 0 | Task tự nhịp, chấp nhận trôi |
| **`vTaskDelayUntil()`** | Task tự ngủ | Tick, **không drift** | 0 | Vòng lặp định kỳ chính xác |
| **Software Timer** | Timer Service Task | Tick, phụ thuộc priority daemon | ~44 B/timer | Nhiều sự kiện định giờ, không muốn tốn task |
| **Hardware Timer (TIM)** | ISR thật | **Rất cao** (µs–ns) | 0, tốn 1 peripheral | PWM, sampling, real-time ngặt |

Điểm mấu chốt: **Software Timer KHÔNG dùng cho real-time ngặt**. Cần chính
xác µs → phải dùng hardware timer.

### 11. Phân tích cấu hình priority của project này

```
Priority 3 : WashingManager
Priority 2 : UITask, SensorTask, Timer Service Task   ← daemon ngang 2 task này
Priority 1 : OLEDTask
```

- Daemon **ngang priority** UITask/SensorTask → nếu 2 task đó đang chạy,
  daemon phải đợi. Với timer 10–20 giây thì trễ vài ms là vô nghĩa; nhưng
  project cần timer 10 ms thì đây sẽ là vấn đề thật.
- Daemon **thấp hơn** WashingManager (3) → khi callback set `EVT_TIMEOUT`,
  WashingManager lập tức được đánh thức và **preempt** daemon ngay tại dòng
  `xEventGroupSetBits()`. Phản hồi nhanh — thiết kế tốt.

**Khuyến nghị chung cho project khác:** đặt `configTIMER_TASK_PRIORITY`
**cao** (thường cao nhất hoặc gần cao nhất), vì callback ngắn và bạn muốn
timer đúng giờ. Đặt thấp là nguyên nhân phổ biến khiến "timer chạy không
chính xác".

### 12. Lỗi kinh điển — checklist

1. **Block trong callback** → treo toàn bộ hệ thống timer.
2. **Tưởng callback là ISR** → dùng nhầm `...FromISR()`.
3. **Tưởng `xTimerStop()` dừng ngay** → race condition.
4. **Không kiểm tra return value** của `xTimerStart/Stop`.
5. **Period = 0** → `configASSERT()` fail ngay lúc create.
6. **Tự start lại trong callback** thay vì auto-reload → drift tích lũy.
7. **`configTIMER_TASK_PRIORITY` quá thấp** → callback trễ, đổ lỗi nhầm cho FreeRTOS.
8. **Dùng handle sau `xTimerDelete()`** → dangling handle.
9. **Quên `configUSE_TIMERS = 1`** → daemon không tồn tại.

### 13. Câu hỏi phỏng vấn

1. *"Software timer callback chạy trong ngữ cảnh nào?"* → Timer Service Task,
   **không phải ISR**.
2. *"Vì sao không được block trong timer callback?"* → Daemon dùng chung cho
   mọi timer; block một cái làm trễ tất cả.
3. *"`xTimerStart()` có bắt đầu timer ngay không?"* → Không, nó gửi lệnh vào
   timer command queue; daemon xử lý sau.
4. *"Khác nhau giữa software timer và hardware timer?"* → Độ chính xác, ngữ
   cảnh chạy, chi phí tài nguyên, khả năng mở rộng số lượng.

### 14. Ý tưởng thiết kế rút ra cho project này

`xEventGroupWaitBits()` có sẵn tham số timeout → hoàn toàn có thể **bỏ
`xFillTimeoutTimer`** và viết:

```c
bits = xEventGroupWaitBits(xSystemEvents, EVT_WATER_FULL | EVT_ERROR,
                           pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
if      (bits & EVT_WATER_FULL) { /* nước đầy    */ }
else if (bits & EVT_ERROR)      { /* lỗi         */ }
else                            { /* TIMEOUT 10s */ }
```

→ Bỏ được 1 timer, bỏ luôn `EVT_TIMEOUT`, và **xóa sạch cả lớp bug
"timeout của state cũ rò rỉ sang state mới"** vì không còn bit timeout nào
tồn tại để mà rò rỉ. Đáng cân nhắc nghiêm túc khi quay lại sửa project.

---

# DD-04 (tiếp) — Phần bổ sung Buổi 2 · 2026-08-23

> **Vì sao có phần này:** buổi 1 đóng DD-04 ở mục 14 và chuyển sang DD-05 —
> quá sớm. Mục 1–14 mới dạy phần **dùng** API; phần **nội tại** (danh sách,
> tick overflow, stack, độ phức tạp) và **pattern thực chiến** còn thiếu hẳn.
> Mục 20 còn là một **đính chính**: buổi 1 dạy một API mà project này không
> biên dịch.

### 15. Vòng đời timer — trạng thái Dormant (thuật ngữ chính thức)

`timers.h:111` nói nguyên văn:

> *"Timers are created in the **dormant** state. The xTimerStart(),
> xTimerReset(), xTimerStartFromISR(), xTimerResetFromISR(),
> xTimerChangePeriod() and xTimerChangePeriodFromISR() API functions can all be
> used to transition a timer into the **active** state."*

Trạng thái nằm trong 3 bit của `ucStatus` (`timers.c:78-80`):

```c
#define tmrSTATUS_IS_ACTIVE                  ( 0x01U )
#define tmrSTATUS_IS_STATICALLY_ALLOCATED    ( 0x02U )
#define tmrSTATUS_IS_AUTORELOAD              ( 0x04U )
```

```text
        xTimerCreate()
              │
              ▼
      ┌───────────────┐   xTimerStart/Reset/ChangePeriod   ┌──────────────┐
      │   DORMANT     │ ────────────────────────────────►  │   ACTIVE     │
      │ (không đếm,   │ ◄────────────────────────────────  │ (nằm trong   │
      │  vẫn tồn tại) │           xTimerStop()             │  timer list) │
      └───────────────┘                                    └──────┬───────┘
              ▲                                                   │ hết hạn
              │  one-shot (pdFALSE) → tự về dormant               ▼
              └──────────────────────────────────────── gọi callback
                                                                  │
                        auto-reload (pdTRUE) → ở lại ACTIVE ◄──────┘
```

**Dormant ≠ bị xóa.** Timer vẫn chiếm 44 byte, handle vẫn hợp lệ,
`xTimerStart()` lại là chạy tiếp. Chỉ `xTimerDelete()` mới giải phóng.

Bằng chứng one-shot tự về dormant (`timers.c:733-739`):

```c
if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) != 0U )
    prvReloadTimer( pxTimer, xNextExpireTime, xTimeNow );        /* ở lại active */
else
    pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );   /* → dormant    */
```

### 16. Vì sao có HAI danh sách timer — xử lý tick overflow

```c
PRIVILEGED_DATA static List_t * pxCurrentTimerList;    /* timers.c:145 */
PRIVILEGED_DATA static List_t * pxOverflowTimerList;   /* timers.c:146 */
```

**Vấn đề:** `TickType_t` 32-bit tràn về 0 sau ~49.7 ngày (@1 kHz). Giả sử tick
hiện tại `0xFFFFFF00`, start timer 1000 tick → hạn nổ `0xFFFFFF00 + 1000`
**tràn** thành `0x000002E7` — *nhỏ hơn* tick hiện tại. Nếu chỉ có một danh
sách sắp tăng dần, timer này bị coi là "đã quá hạn" và **nổ ngay** thay vì sau
1 giây.

**Giải pháp:** hạn nổ bị tràn → bỏ vào danh sách **overflow**
(`timers.c:916`); còn lại vào danh sách hiện tại (`:930`). Khi tick thật sự
tràn, kernel **hoán đổi hai con trỏ** (`prvSwitchTimerLists()`,
`timers.c:1086-1108`):

```c
/* The tick count has overflowed.  The timer lists must be switched.
 * If there are any timers still referenced from the current timer list
 * then they must have expired and should be processed before the lists
 * are switched. */
while( listLIST_IS_EMPTY( pxCurrentTimerList ) == pdFALSE )
{
    xNextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxCurrentTimerList );
    prvProcessExpiredTimer( xNextExpireTime, tmrMAX_TIME_BEFORE_OVERFLOW );
}

pxTemp = pxCurrentTimerList;
pxCurrentTimerList  = pxOverflowTimerList;   /* ← chỉ đổi CON TRỎ */
pxOverflowTimerList = pxTemp;
```

Đổi con trỏ là **O(1)** — không duyệt và sửa từng timer.

> **Kiến thức chuyển giao:** kỹ thuật kinh điển xử lý bộ đếm tràn, dùng y hệt ở
> `xDelayedTaskList` / `xOverflowDelayedTaskList` trong `tasks.c`. Hệ quả thực
> tế cho code của bạn: **đừng bao giờ viết `if (now > deadline)`** — luôn dùng
> `if ((now - start) >= period)`, vì phép trừ unsigned tự đúng khi tràn.

### 17. Vòng lặp thật của daemon — chỉ 3 dòng

`timers.c:766-778`:

```c
for( ; configCONTROL_INFINITE_LOOP(); )
{
    xNextExpireTime = prvGetNextExpireTime( &xListWasEmpty );      /* ① hạn gần nhất */
    prvProcessTimerOrBlockTask( xNextExpireTime, xListWasEmpty );  /* ② ngủ hoặc nổ  */
    prvProcessReceivedCommands();                                  /* ③ dọn queue    */
}
```

Toàn bộ Software Timer gói gọn ở đây. Bước ② chính là chỗ gọi
`vQueueWaitForMessageRestricted()` đã phân tích ở [DD-05](#dd-05).

Thứ tự ②→③ có ý nghĩa: **timer hết hạn được xử lý TRƯỚC lệnh trong queue**
trong cùng một vòng. Đây là cơ chế chính xác phía sau rủi ro ở mục 3 — một
`xTimerStop()` gửi "sát nút" vẫn có thể thua: callback đã nổ trước khi lệnh
stop được đọc.

### 18. Command queue đầy, và gọi lệnh TRƯỚC khi scheduler chạy

`timers.c:474-483`:

```c
if( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING )
    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, xTicksToWait );
else
    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, tmrNO_DELAY );  /* ← ép 0 */
```

1. **Gọi `xTimerStart()` trước `vTaskStartScheduler()` là hợp lệ** — lệnh nằm
   chờ trong queue, daemon xử lý ngay khi scheduler chạy. Nhưng `xTicksToWait`
   **bị ép về 0** (chưa có scheduler thì không block được) → queue đầy lúc đó
   là `pdFAIL` im lặng.
2. **Queue đầy khi đang chạy** → tuỳ `xTicksToWait`.

**Trong project này:** cả 10 lời gọi đều dùng `0` và **không chỗ nào kiểm tra
giá trị trả về**:

```c
xTimerStart(xFillTimeoutTimer, 0);   /* washing_manager.c:76  — bỏ qua return */
xTimerStop(xWashTimer, 0);           /* washing_manager.c:306 — bỏ qua return */
/* ...8 chỗ nữa, tất cả đều bỏ qua */
```

`configTIMER_QUEUE_LENGTH = 10`, chỉ 4 timer, ít lệnh → thực tế chưa tràn.
Nhưng đây đúng là **lỗi kinh điển số 4** ở mục 12, đang tồn tại thật trong
code.

**Cách sizing đúng:** `configTIMER_QUEUE_LENGTH` ≥ số lệnh timer tối đa có thể
dồn lại **giữa hai lần daemon được chạy**. Daemon priority càng thấp thì con
số này càng lớn.

### 19. Static allocation — và vì sao project này không dùng được

```c
TimerHandle_t xTimerCreateStatic( const char * pcTimerName,
                                  TickType_t xTimerPeriodInTicks,
                                  BaseType_t xAutoReload,
                                  void * pvTimerID,
                                  TimerCallbackFunction_t pxCallbackFunction,
                                  StaticTimer_t * pxTimerBuffer );  /* ← bạn cấp RAM */
```

`StaticTimer_t` (`FreeRTOS.h:3284-3295`) là struct "vỏ" cùng kích thước
`Timer_t` nhưng toàn field `Dummy` — chính sách **data hiding** của FreeRTOS:
bạn cấp đúng lượng RAM mà không nhìn thấy nội dung.

**Nhưng** `configSUPPORT_STATIC_ALLOCATION` không được khai trong
`FreeRTOSConfig.h` → mặc định `0` (`FreeRTOS.h:2804-2806`) →
**`xTimerCreateStatic()` không tồn tại trong build này.** 4 timer của project
đều cấp phát động từ heap_4.

Khi nào bắt buộc static: hệ thống an toàn (DO-178C, IEC 61508, automotive)
thường **cấm hoàn toàn dynamic allocation** vì không chứng minh được
worst-case. Static cũng loại bỏ khả năng `xTimerCreate()` trả `NULL` giữa
chừng.

### 20. ⚠ Đính chính mục 9 — `xTimerPendFunctionCall` KHÔNG có trong project này

Buổi 1 dạy `xTimerPendFunctionCallFromISR()` như một API sẵn dùng. Kiểm tra
lại:

```c
#ifndef INCLUDE_xTimerPendFunctionCall
    #define INCLUDE_xTimerPendFunctionCall    0      /* FreeRTOS.h:283-284 */
#endif
```

Project không khai macro này → **mặc định 0 → hàm không được biên dịch**. Gọi
nó sẽ lỗi linker.

Đây là **cùng một lỗi** đã mắc ở DD-01 mục 5 (`xEventGroupSetBitsFromISR`
cũng không compile được vì chính lý do này) nhưng lặp lại ở DD-04. Kiến thức
về API vẫn đúng và vẫn đáng học cho project khác; muốn dùng thì phải thêm
`#define INCLUDE_xTimerPendFunctionCall 1`.

> **Bài học phương pháp:** "API tồn tại trong FreeRTOS" ≠ "API dùng được trong
> build này". Luôn kiểm tra macro `INCLUDE_*` / `configUSE_*` trước khi dạy
> hoặc trước khi viết code gọi nó.

### 21. Các API còn thiếu ở bảng mục 6

| API | Ghi chú |
| --- | --- |
| `vTimerSetReloadMode()` | **Đổi one-shot ↔ auto-reload lúc chạy** (`timers.c:1272`) |
| `xTimerGetReloadMode()` / `uxTimerGetReloadMode()` | Đọc chế độ hiện tại |
| `xTimerGetTimerDaemonTaskHandle()` | Lấy handle daemon — dùng đo stack (mục 26) |
| `xTimerGetStaticBuffer()` | Lấy lại buffer đã cấp cho timer static |
| `xTimerCreateTimerTask()` | Kernel tự gọi trong `vTaskStartScheduler()` — không gọi tay |
| `vApplicationDaemonTaskStartupHook()` | Hook chạy **một lần** khi daemon khởi động (`timers.c:762`), cần `configUSE_DAEMON_TASK_STARTUP_HOOK = 1` |

Cái cuối đáng chú ý: nó cho bạn một chỗ chạy code khởi tạo **sau khi scheduler
đã chạy** — hữu ích khi việc khởi tạo cần gọi API FreeRTOS blocking mà `main()`
không làm được.

### 22. Độ phức tạp — Software Timer KHÔNG miễn phí khi số lượng lớn

Mục 1 khoe "thêm timer thứ 100 không tốn thêm stack nào". Đúng về **RAM**, sai
nếu suy ra là không tốn gì.

Timer được cắm vào danh sách **sắp xếp theo hạn nổ** bằng `vListInsert()`
(`list.c:192-196`):

```c
for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd );
     pxIterator->pxNext->xItemValue <= xValueOfInsertion;
     pxIterator = pxIterator->pxNext )
{
    /* There is nothing to do here, just iterating to the wanted
     * insertion position. */
}
```

→ **Chèn là O(n)**, n = số timer đang active. Với 100 timer active, mỗi
`xTimerStart()` duyệt trung bình 50 node — chạy **trong daemon**, làm trễ mọi
timer khác.

| Thao tác | Độ phức tạp | Ghi chú |
| --- | --- | --- |
| `xTimerStart/Reset/ChangePeriod` (chèn) | **O(n)** | Duyệt tìm vị trí sắp xếp |
| Lấy timer sắp nổ nhất | **O(1)** | Luôn là head |
| `xTimerStop` (gỡ) | **O(1)** | Doubly-linked list |
| Chuyển danh sách khi tick tràn | **O(1)** | Chỉ đổi con trỏ (mục 16) |

**Vì sao FreeRTOS chọn sorted list?** Tối ưu cho trường hợp phổ biến của hệ
nhúng nhỏ: ít timer (< 20), và thao tác nhiều nhất là "cái nào sắp nổ" — cái
đó O(1). Đánh đổi hợp lý; nhưng hàng trăm timer (server, network stack) thì
sorted list là điểm nghẽn.

**Cấu trúc khác dùng trong thực tế — kiến thức chuyển giao:**

| Cấu trúc | Chèn | Lấy min | Ai dùng |
| --- | --- | --- | --- |
| Sorted linked list | O(n) | O(1) | **FreeRTOS**, hệ nhỏ |
| Min-heap / priority queue | O(log n) | O(1) | Zephyr, nhiều RTOS khác |
| **Timer wheel** (hashed) | **O(1)** | O(1) amortized | Linux `timer_list`, Netty, Kafka |
| Red-black tree | O(log n) | O(1) | Linux `hrtimer` |

Timer wheel đáng biết: chia thời gian thành các "ô" như mặt đồng hồ, timer ném
vào ô tương ứng → chèn O(1) vì không cần sắp xếp. Trả giá bằng độ phân giải
theo ô và bộ nhớ cho mảng ô. Câu hỏi phỏng vấn hay ở vị trí system/backend.

### 23. Callback chạy trên stack của AI — nguồn của một loại bug rất khó tìm

```c
#define configMINIMAL_STACK_SIZE       ( ( unsigned short ) 130 )   /* words */
#define configTIMER_TASK_STACK_DEPTH   ( configMINIMAL_STACK_SIZE * 2 )
```

→ 260 words × 4 = **1040 bytes**. Đó là **toàn bộ** stack mà mọi callback
timer dùng chung.

```c
/* ❌ Bom hẹn giờ */
void MyCallback(TimerHandle_t t) {
    char buffer[512];              /* nửa stack của daemon, cho MỘT biến */
    sprintf(buffer, "...");        /* sprintf tự nó cũng ngốn stack      */
}
```

Cái độc ác: **stack tràn ở một task bạn chưa từng viết** ("Tmr Svc"). Chưa bật
`configCHECK_FOR_STACK_OVERFLOW` thì triệu chứng là HardFault ngẫu nhiên hoặc
dữ liệu hỏng ở nơi hoàn toàn không liên quan — bạn sẽ tìm bug trong code của
mình mãi không ra.

**Quy tắc:**

- Callback chỉ dùng biến local nhỏ. Cần buffer lớn → `static` hoặc buffer cấp
  sẵn ở ngoài.
- Callback gọi `printf`/`sprintf`/hàm float → cộng thêm vài trăm byte stack →
  phải tăng `configTIMER_TASK_STACK_DEPTH`.
- Đo thật, đừng đoán — xem mục 26.

Callback của project chỉ có một dòng `xEventGroupSetBits()` → an toàn tuyệt
đối. Nhưng đây là lý do nguyên tắc "callback ngắn" không chỉ về *thời gian*,
mà còn về *stack*.

### 24. Reentrancy — điểm tinh tế nhất, hay bị hiểu sai cả hai chiều

Vì **mọi** callback chạy trong **một** task duy nhất:

**✅ Hai callback KHÔNG BAO GIỜ chạy đồng thời** — tuần tự hoá tự nhiên.

```c
static int s_sharedCounter;      /* chia sẻ giữa callback A và callback B */

void CallbackA(TimerHandle_t t) { s_sharedCounter++; }   /* KHÔNG cần mutex */
void CallbackB(TimerHandle_t t) { s_sharedCounter--; }   /* KHÔNG cần mutex */
```

Lợi ích **miễn phí** rất ít người tận dụng: gom các thao tác trên một tài
nguyên vào các timer callback thì tự động loại bỏ nhu cầu đồng bộ giữa chúng —
cùng nguyên lý với "single-threaded event loop" của Node.js / Redis.

**❌ Nhưng callback KHÔNG được bảo vệ khỏi các TASK khác.** Daemon prio 2;
WashingManager prio 3 **preempt được** nó bất cứ lúc nào, kể cả giữa chừng
callback (chứng minh ở DD-05 mục 8).

```c
static uint32_t s_sharedWithTask;   /* callback đọc/ghi, WashingManager cũng vậy */

void Callback(TimerHandle_t t) {
    s_sharedWithTask++;             /* ⚠ đọc-sửa-ghi KHÔNG nguyên tử         */
}                                   /*   WashingManager có thể chen vào giữa */
```

| Chia sẻ giữa | Cần đồng bộ? |
| --- | --- |
| Callback ↔ Callback | ❌ Không — cùng một task, tuần tự |
| Callback ↔ Task | ✅ Có |
| Callback ↔ ISR | ✅ Có (critical section / `...FromISR`) |

Tốt nhất: **đừng chia sẻ biến** — gửi message qua queue/event group, đúng như
project này đang làm.

### 25. Tám pattern thực chiến — dùng lại ở mọi project

**① Debounce bằng timer** (thay cho đếm tick trong ISR)

```c
void EXTI_Handler(void) {                       /* ISR chỉ reset timer */
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
}
void DebounceCallback(TimerHandle_t t) {        /* one-shot 20ms */
    ButtonEvent_t e = BSP_Button_Read();
    xQueueSend(xButtonQueue, &e, 0);
}
```

Nhiễu bật-tắt liên tục → mỗi lần đều reset → callback chỉ nổ khi tín hiệu **đã
yên 20 ms**. Sạch hơn cách project đang dùng.

**② Watchdog mềm — phát hiện "cái gì đó NGỪNG xảy ra"**

```c
void OnHeartbeatReceived(void) { xTimerReset(xLinkWatchdog, 0); }
void LinkTimeoutCallback(TimerHandle_t t) { xEventGroupSetBits(ev, EVT_LINK_LOST); }
```

Pattern quan trọng nhất danh sách: **timer là công cụ duy nhất phát hiện được
sự VẮNG MẶT của sự kiện.** Queue/semaphore chỉ báo được cái *đã xảy ra*.

**③ Timeout guard cho phase** — chính là `xFillTimeoutTimer` của project.

**④ End-of-frame cho giao thức không có ký tự kết thúc** — mỗi byte UART đến
thì reset; im lặng 50 ms = hết gói. Cách chuẩn cho Modbus RTU.

**⑤ Retry với exponential backoff**

```c
void RetryCallback(TimerHandle_t t) {
    uint32_t attempt = (uint32_t)pvTimerGetTimerID(t);
    if (TrySend() != OK && attempt < 5) {
        vTimerSetTimerID(t, (void *)(attempt + 1));
        xTimerChangePeriod(t, pdMS_TO_TICKS(100 << attempt), 0);  /* tự start lại */
    }
}
```

`xTimerChangePeriod()` **tự động start** timer đang dormant — đúng thứ cần ở
đây, nhưng là bẫy nếu bạn chỉ định đổi period.

**⑥ Chuỗi one-shot — state machine chạy bằng timer.** Một callback, Timer ID
làm "bước hiện tại", mỗi lần nổ thì đặt period mới và start bước kế. Thay được
cả một task cho chuỗi hành động đơn giản.

**⑦ Rate limiting / throttle** — timer active nghĩa là "đang trong thời gian
khoá":

```c
if (xTimerIsTimerActive(xThrottleTimer) == pdFALSE) {
    DoExpensiveThing();
    xTimerStart(xThrottleTimer, 0);   /* khoá 1s */
}
```

**⑧ Blink pattern không chặn** — auto-reload timer đảo trạng thái LED. Thay
được toàn bộ các vòng `HAL_Delay()` nhấp nháy.

### 26. Debug timer — công cụ có sẵn nhưng chưa dùng

FreeRTOS có 5 trace hook riêng cho timer (`FreeRTOS.h:860-877`):

| Hook | Bắn khi |
| --- | --- |
| `traceTIMER_CREATE` | Tạo thành công |
| `traceTIMER_CREATE_FAILED` | Hết heap → `xTimerCreate()` trả `NULL` |
| `traceTIMER_COMMAND_SEND` | Gửi lệnh vào queue — **thấy được lệnh bị `pdFAIL`** |
| `traceTIMER_COMMAND_RECEIVED` | Daemon đọc được lệnh — **đo được độ trễ queue** |
| `traceTIMER_EXPIRED` | Timer nổ, ngay trước khi gọi callback |

Cặp `COMMAND_SEND` → `COMMAND_RECEIVED` đo được **chính xác daemon trễ bao
lâu** — trả lời câu "priority daemon đặt thấp có sao không" bằng số liệu thay
vì suy đoán. Project đã có SEGGER SystemView (Part I, Phase 8), chỉ cần nhìn
task `"Tmr Svc"` trên timeline.

**Đo stack thật của daemon:**

```c
TaskHandle_t hDaemon = xTimerGetTimerDaemonTaskHandle();
UBaseType_t freeWords = uxTaskGetStackHighWaterMark(hDaemon);
/* Trả về số WORD còn dư ít nhất từ trước tới giờ. Gần 0 = sắp tràn. */
```

Cách duy nhất trả lời "1040 byte có đủ không" mà không phải đoán.

### 27. Test logic phụ thuộc thời gian (mục F)

**Vấn đề:** chu trình giặt của project là 10 s + 20 s + 15 s + 25 s = 70 s cho
MỘT test case. Test tự động chạy thời gian thật thì không ai chạy nó thường
xuyên → test chết yểu. Tệ hơn: timing thật trên máy build không ổn định →
test **flaky**, dạy người ta bỏ qua kết quả đỏ.

**Nguyên tắc cốt lõi — tách "quyết định" khỏi "chờ đợi":**

```text
❌ Trộn lẫn — không test được
   ┌─────────────────────────────────────────┐
   │ chờ 20s → rồi quyết định chuyển state    │
   └─────────────────────────────────────────┘

✅ Tách đôi
   ┌──────────────┐        ┌──────────────────────────┐
   │ Timer: chờ   │ ─────► │ Logic thuần: nhận sự kiện│
   │ (khó test)   │ sự kiện│ → quyết định (dễ test)   │
   └──────────────┘        └──────────────────────────┘
```

Phần "chờ" giao cho RTOS — không cần test, FreeRTOS đã test rồi. Phần "quyết
định" là code của bạn — test phần đó, và nó không cần thời gian trôi.

**`washing_sm.c` đã testable sẵn — kiểm chứng bằng include chain:**

```text
washing_sm.c → washing_sm.h → app_types.h → <stdint.h>
             → <stddef.h>
```

**Không có một dòng FreeRTOS nào** — đúng Design Rule #3 ("Drivers must not
depend on FreeRTOS") áp dụng cho tầng logic. File này biên dịch và chạy được
trên PC. Nó còn có sẵn dependency injection qua `WashingSM_Callbacks_t` (con
trỏ hàm, doc ghi *"May be NULL"* — test càng đơn giản).

**Test thật, chạy trên PC bằng gcc, mất vài mili giây:**

```c
/* test_washing_sm.c — biên dịch: gcc test_washing_sm.c washing_sm.c -o test */
#include "washing_sm.h"
#include <assert.h>

static int s_drainCalled;
static void FakeOnEnterDrain(void) { s_drainCalled++; }

static void test_wash_done_goes_to_drain(void) {
    WashingSM_Callbacks_t cb = { .onEnterDrain = FakeOnEnterDrain };
    WashingSM_Init(&cb);

    WashingSM_ProcessEvent(WASH_EVT_START_WASH);
    WashingSM_ProcessEvent(WASH_EVT_WATER_FULL);
    assert(WashingSM_GetState() == WASH_STATE_WASHING);

    WashingSM_ProcessEvent(WASH_EVT_WASH_DONE);      /* ← thay cho "chờ 20s" */
    assert(WashingSM_GetState() == WASH_STATE_DRAIN);
    assert(s_drainCalled == 1);
}
```

Dòng mấu chốt: `WASH_EVT_WASH_DONE` **thay thế 20 giây chờ**. Với state
machine, "thời gian trôi" chỉ là một sự kiện như mọi sự kiện khác — sự kiện
thì gửi được tức thì.

> **Kiến thức chuyển giao:** kiến trúc *table-driven state machine + callback
> injection* không chỉ sạch về thiết kế — nó **là điều kiện để test được**.
> Đây là lý do thật khiến kỹ sư giỏi tách tầng: tầng không phụ thuộc RTOS thì
> test được trên PC trong mili giây.

**Năm kỹ thuật, theo thứ tự nên ưu tiên:**

| # | Kỹ thuật | Test được gì | Chi phí |
| --- | --- | --- | --- |
| 1 | Tách logic thuần, test trên PC | Transition table, guard, thứ tự callback | Rẻ nhất nếu kiến trúc đúng |
| 2 | Rút ngắn period khi build test | Tích hợp thật, nhưng nhanh | Rẻ, không test được timing thật |
| 3 | Tiêm đồng hồ giả (fake clock) | Code gọi trực tiếp `xTaskGetTickCount()` | Trung bình |
| 4 | FreeRTOS simulator port (Win/POSIX) | Cả hệ RTOS, trên PC | Cao |
| 5 | Test trên board thật | Mọi thứ kể cả phần cứng | Đắt, chậm, khó tự động hoá |

Kỹ thuật 2:

```c
#ifdef UNIT_TEST
    #define WASH_CYCLE_DURATION_MS   20      /* 20ms thay vì 20000ms */
#else
    #define WASH_CYCLE_DURATION_MS   20000
#endif
```

Bẫy: rút ngắn quá thì **lộ ra race condition không có ở tốc độ thật**, hoặc
ngược lại **che mất** race chỉ xuất hiện khi chậm.

Kỹ thuật 3:

```c
/* ❌ Không test được — cứng nhắc gọi API kernel */
uint32_t elapsed = xTaskGetTickCount() - s_startTick;

/* ✅ Test được */
typedef uint32_t (*GetTickFn_t)(void);
static GetTickFn_t s_getTick = xTaskGetTickCount;   /* test đổi thành hàm giả */
uint32_t elapsed = s_getTick() - s_startTick;
```

Test "du hành thời gian" bằng hàm giả trả về giá trị bất kỳ — kể cả giá trị
sát điểm tràn `0xFFFFFF00`, thứ mà chờ thật phải mất 49.7 ngày mới gặp. Cách
duy nhất test được lớp bug overflow ở mục 16.

Kỹ thuật 4: FreeRTOS có port chạy trên PC (Windows MSVC/MinGW, POSIX/Linux) —
kernel thật chạy trên máy tính, test tương tác nhiều task không cần board.
Trả giá: timing trên PC không giống MCU, chỉ tin phần *logic*, đừng tin phần
*thời gian*.

**Liên quan mục 23:**

```c
#define configCHECK_FOR_STACK_OVERFLOW   0      /* FreeRTOSConfig.h:64 */
```

Project **đang tắt** kiểm tra tràn stack → bẫy "callback ngốn stack daemon" ở
mục 23 sẽ **không có cảnh báo nào**, chỉ HardFault hoặc dữ liệu hỏng ngẫu
nhiên. Khi viết callback phức tạp hơn, nên bật `2` (kiểm tra pattern, chính
xác hơn `1`). Tương tự `configGENERATE_RUN_TIME_STATS = 0` → chưa đo được %
CPU từng task.

### 28. So sánh với hệ khác (mục G)

> Phần này là kiến thức chung về các hệ khác, không trích từ source của
> project này — không gắn `file:dòng`.

**Bare-metal — 3 cách, và vì sao chúng không đủ:**

```c
/* ① Delay chặn — tệ nhất, CPU quay vòng vô ích */
HAL_Delay(20000);

/* ② Polling bộ đếm — không chặn, nhưng phình ra rất nhanh */
if (HAL_GetTick() - washStart >= 20000) { /* xong */ }

/* ③ Hardware timer + ISR — chính xác nhất, tốn 1 peripheral/việc */
void TIM2_IRQHandler(void) { flagWashDone = 1; }
```

Cách ② là cách bare-metal làm "software timer thủ công" — hoạt động, nhưng 10
việc định giờ thì `main()` biến thành khối `if` khổng lồ, mỗi cái một biến
`xxxStart` riêng, không quản lý nổi. **Software Timer của FreeRTOS chính là
cách ② được đóng gói tử tế**: một danh sách sắp xếp + một task duyệt nó. Chú
ý cách ② dùng phép **trừ** — đúng nguyên tắc chống tràn ở mục 16.

**Bảng so sánh quan trọng nhất — callback chạy ở ngữ cảnh nào:**

| Hệ | Cơ chế | Callback chạy trong | Được block? |
| --- | --- | --- | --- |
| **FreeRTOS** | `xTimerCreate` | **Task** ("Tmr Svc") | Không nên (làm trễ timer khác) |
| **Zephyr** | `k_timer` | **ISR** (ngắt hệ thống) | ❌ Tuyệt đối không |
| **Zephyr** | `k_work_delayable` | Thread (workqueue) | Có, tuỳ workqueue |
| **Linux kernel** | `timer_list` | **Softirq** | ❌ Không (không có ngữ cảnh process) |
| **Linux kernel** | `hrtimer` | Hardirq/softirq | ❌ Không |
| **Linux userspace** | `timerfd` | Thread của bạn (qua `poll`) | Có |
| **POSIX** | `timer_create` | Signal handler hoặc thread | Tuỳ `SIGEV_*` |

**Đây là cái bẫy chết người khi port code.** Callback timer viết cho FreeRTOS
(chạy trong task, được phép `xQueueSend`, thậm chí mutex) bê nguyên sang
Zephyr `k_timer` sẽ chạy **trong ISR** → gọi API blocking ở đó là crash hoặc
hỏng ngầm. Ngược lại, người quen Zephyr sang FreeRTOS dễ viết callback kiểu
ISR — dùng nhầm `...FromISR()` (lỗi kinh điển số 2, mục 12).

> **Câu hỏi đầu tiên phải hỏi khi dùng timer của BẤT KỲ hệ nào: "callback này
> chạy ở ngữ cảnh gì?"** Trả lời được thì tự suy ra mọi ràng buộc còn lại —
> được block không, gọi API nào, chia sẻ dữ liệu ra sao. Đây là câu hỏi làm
> trục xuyên suốt DD-04.

**Vì sao Linux cần tới HAI hệ thống timer** — đáng học vì cho thấy một đánh
đổi thiết kế thật:

| | `timer_list` | `hrtimer` |
| --- | --- | --- |
| Cấu trúc | Timer wheel | Red-black tree |
| Độ phân giải | Theo `jiffies` (~1–4 ms) | Nanosecond |
| Chèn | **O(1)** | O(log n) |
| Dùng cho | Timeout mạng, housekeeping — số lượng rất lớn, **đa số bị huỷ trước khi nổ** | Audio, `nanosleep`, POSIX timer — cần chính xác |

Ý tưởng cốt lõi: timeout thì nhiều nhưng hiếm khi nổ (gói tin thường đến
trước timeout, timer bị huỷ) → tối ưu cho *chèn và huỷ nhanh*, không cần
chính xác. Hẹn giờ để làm việc thì ít nhưng phải đúng giờ → chấp nhận O(log
n). FreeRTOS chỉ có một hệ, nghiêng về loại thứ hai, chấp nhận O(n) vì hệ
nhúng nhỏ ít timer.

### 29. Ranh giới của test — seam cũng là vùng mù

**Câu hỏi đặt ra:** unit test cho `washing_sm.c` chạy trên PC, đầy đủ đến đâu
cũng có một loại bug **không bao giờ bắt được**. Loại nào, và vì sao?

**Trả lời — chính là Bug #17 (`sensor_task.c:23-28`):**

```c
if (BSP_Sensor_IsDoorClosed()) {
    xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
} else {
    xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
    xEventGroupSetBits(xSystemEvents, EVT_ERROR);   /* set, KHÔNG BAO GIỜ clear */
}
```

Mở cửa → `EVT_ERROR` = 1 → đóng lại → bit vẫn = 1 → bấm Start → `WaitBits`
thấy bit sẵn → `WASH_EVT_ERROR` được gửi vào state machine.

**`washing_sm.c` xử lý sự kiện đó hoàn toàn đúng** — nhận `WASH_EVT_ERROR` ở
`FILL_WATER`, chuyển sang `ERROR`. Không transition nào sai. Test sẽ viết:

```c
WashingSM_ProcessEvent(WASH_EVT_ERROR);
assert(WashingSM_GetState() == WASH_STATE_ERROR);   /* ✅ PASS */
```

**Test xanh. Máy vẫn hỏng.** Vì bug không nằm ở "xử lý sự kiện ERROR thế
nào", mà ở "**tại sao sự kiện ERROR lại được sinh ra** khi cửa đang đóng" —
câu hỏi nằm hoàn toàn ngoài `washing_sm.c`, file đó không biết Event Group,
cảm biến, hay ai gọi nó tồn tại.

> **Nguyên tắc tổng quát: chỗ bạn tiêm phụ thuộc (seam) cũng chính là ranh
> giới vùng mù của test.** `washing_sm.c` tách khỏi FreeRTOS qua hai seam —
> đầu vào `WashEvent_t`, đầu ra callback. Test qua hai seam đó chỉ trả lời
> được *"cho đúng sự kiện, có chuyển đúng trạng thái không?"* — không trả lời
> được *"có sinh ra đúng sự kiện không?"*, nơi phần lớn bug nhúng thật sự sống.

| Loại bug | Nằm ở đâu | Test `washing_sm.c` bắt được? |
| --- | --- | --- |
| Transition sai, thiếu guard | `washing_sm.c` | ✅ Có |
| **Sự kiện thừa/sai/thiếu** (Bug #17) | `sensor_task.c` | ❌ Không |
| Race do lệnh timer bất đồng bộ (mục 3) | Giữa manager và daemon | ❌ Không |
| Chân GPIO đọc sai (Bug #14/#16) | Phần cứng | ❌ Không |
| Callback thật ghi nhầm chân | `washing_manager.c` | ❌ Không — test dùng callback giả |
| Priority sai, task đói CPU | Cấu hình RTOS | ❌ Không |
| Tràn stack, hết heap | Toàn hệ thống | ❌ Không |

**Cùng hình dạng lỗi còn một chỗ nữa** (`sensor_task.c:30-32`):

```c
if (BSP_Sensor_IsWaterFull()) {
    xEventGroupSetBits(xSystemEvents, EVT_WATER_FULL);
}
/* không có else → không bao giờ clear */
```

Cùng thuộc **Sticky/Stale Level Flag** (DD-03): nước đầy lúc IDLE → bit bật và
ở lại → bấm Start → bỏ qua giai đoạn cấp nước dù bể có thể vừa xả cạn. Xác
nhận nhận định của DD-03: khi một anti-pattern xuất hiện, tìm các thể hiện
khác của cùng hình dạng thay vì chỉ sửa chỗ đầu tiên. (Ghi nhận — chưa sửa,
theo quyết định gác bug lại của người dùng.)

**Tầng bị bỏ sót nên test bằng gì — kim tự tháp test cho hệ nhúng:**

| Tầng | Test cái gì | Chạy ở đâu | Bắt được Bug #17? |
| --- | --- | --- | --- |
| Unit | Logic thuần (`washing_sm.c`) | PC, mili giây | ❌ |
| **Component** | Tầng sinh sự kiện — cảm biến giả, kiểm tra **bit nào set/clear** | PC, cần tách BSP khỏi HAL | ✅ |
| Integration | Nhiều task chạy thật | Simulator / board | ✅ |
| Manual có kịch bản | Toàn hệ + phần cứng | Board | ✅ (cách đã tìm ra Bug #17 thật) |

Tầng Component đang thiếu và đáng đầu tư nhất:

```c
/* Cho cảm biến giả, kiểm tra CHÍNH XÁC bit nào được set/clear */
FakeSensor_SetDoorClosed(false);
SensorTask_PollOnce();                 /* tách vòng lặp khỏi thân xử lý */
assert(GetBits() & EVT_ERROR);

FakeSensor_SetDoorClosed(true);
SensorTask_PollOnce();
assert((GetBits() & EVT_ERROR) == 0);  /* ❌ FAIL → bắt được Bug #17 */
```

Cần một thay đổi kiến trúc nhỏ để viết được: **tách thân xử lý ra khỏi vòng
`while(1) + vTaskDelay()`**. Pattern chung: vòng lặp vô hạn và phần logic nên
là hai hàm khác nhau — vòng lặp không test được, logic thì có.

> **Bài học rút gọn:** test chứng minh code làm đúng điều nó được yêu cầu,
> không chứng minh nó được yêu cầu đúng điều. Test xanh 100% ở tầng state
> machine hoàn toàn tương thích với một cái máy giặt hỏng.

---

## DD-05 — Tick, SysTick & Context Switch (Buổi 2 · 2026-08-23)

> Câu hỏi mở đầu buổi này: *"Software Timer — ai quản lý nó, nguồn của nó từ
> đâu (clock nội hay ngoại)?"* DD-04 mới trả lời tầng **logic** (Timer Service
> Task quản lý danh sách timer). DD-05 trả lời tầng **vật lý**: cái gì tạo ra
> khái niệm "thời gian" cho toàn bộ FreeRTOS.

### 1. Hai tầng "quản lý" — đừng gộp làm một

| Tầng | Ai quản lý | Trách nhiệm |
| --- | --- | --- |
| **Thời gian** (vật lý) | **SysTick** — bộ đếm nằm trong lõi Cortex-M | Ngắt định kỳ, tạo "nhịp đập" cho hệ thống |
| **Logic** (DD-04) | **Timer Service Task** | Dựa vào nhịp đập, biết timer nào tới hạn → gọi callback |

Sai lầm phổ biến: nghĩ "Software Timer tự chạy". Không — nó **phụ thuộc hoàn
toàn vào SysTick**. Tắt SysTick thì mọi timer, mọi `vTaskDelay()`, mọi timeout
đứng im vĩnh viễn.

### 2. SysTick là gì — kiến thức ARM, không phải kiến thức ST

SysTick **không** phải ngoại vi của STM32 (không phải TIM2/TIM3/…). Nó là bộ
đếm ngược **24-bit nằm trong chính lõi ARM Cortex-M**, thuộc chuẩn kiến trúc —
mọi chip Cortex-M của mọi hãng (ST, NXP, TI, Nordic…) đều có.

> **Vì sao điều này quan trọng:** đó là lý do FreeRTOS chỉ cần **một** port cho
> `ARM_CM4F` là chạy được trên mọi board Cortex-M4F, không cần biết hãng nào
> làm chip. Kernel không đụng tới ngoại vi riêng của hãng. Đây là ranh giới
> portability điển hình — đáng học để tự thiết kế lớp trừu tượng sau này.

### 3. Toàn bộ chuỗi — từ dao động điện tử tới lúc callback chạy

```text
HSI — bộ dao động RC NỘI, 16 MHz, nằm trong chip
   │
   ▼
SystemCoreClock = 16 MHz            (không dùng PLL trong project này)
   │
   ▼
SysTick — bộ đếm ngược trong lõi Cortex-M4
   │  nạp = configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ - 1
   │      = 16 000 000 / 1000 - 1 = 15999
   │  → đếm 16000 xung CPU thì tràn → bắn ngắt
   ▼
Ngắt SysTick, đều đặn mỗi 1 ms
   │
   ▼
xPortSysTickHandler()               (chính là SysTick_Handler — xem §5)
   │
   ▼
xTaskIncrementTick()
   │  ① tăng biến đếm tick toàn cục
   │  ② quét danh sách task đang Blocked-with-timeout
   │  ③ task nào hết hạn → chuyển sang Ready
   ▼
Nếu Timer Service Task nằm trong số đó
   → được lên lịch chạy → gọi Timer_PhaseCallback()
```

### 4. Nguồn clock của **project này** — nội, không phải ngoại

Kiểm chứng bằng `SystemClock_Config()` trong `Core/Src/main.c`:

```c
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;  /* ← chọn HSI  */
RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;            /* ← không PLL */
RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
```

→ **HSI = High Speed Internal**, dao động RC **nội** ~16 MHz, không cần thạch
anh ngoài. Board STM32F411E-DISCO *có* chân cho thạch anh ngoài (HSE — `.ioc`
vẫn khai `HSE_VALUE = 25000000` như hằng số dự phòng) nhưng project **không
chọn dùng**. Không PLL nên `SystemCoreClock = 16 MHz` thẳng từ HSI (khớp
`RCC.SYSCLKFreq_VALUE=16000000` trong `Freertos.ioc`).

**HSI vs HSE — bảng cần nhớ khi chọn cho project khác:**

| | HSI (nội) | HSE (thạch anh ngoài) |
| --- | --- | --- |
| Linh kiện ngoài | Không | Thạch anh + 2 tụ |
| Sai số điển hình | ~1 % (trôi theo nhiệt độ) | 20–50 ppm (~0.005 %) |
| Thời gian khởi động | Rất nhanh (µs) | Chậm hơn (ms) |
| Dùng khi | Học tập, timing không khắt khe | USB, CAN, Ethernet, RTC, đo thời gian dài |

Hệ quả cho project này: HSI trôi 1 % → chu trình giặt 20 s có thể lệch ±0.2 s.
Không quan trọng ở quy mô này. Nhưng nếu cần đếm giờ chính xác nhiều ngày, hoặc
cần USB (bắt buộc sai số < 0.25 %), **phải** chuyển sang HSE.

### 5. Bằng chứng bằng source — 3 điểm mấu chốt

**(a) Cấu hình phần cứng SysTick** — `OS/FreeRTOS/portable/GCC/ARM_CM4F/port.c:826-828`:

```c
portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG
                            | portNVIC_SYSTICK_INT_BIT
                            | portNVIC_SYSTICK_ENABLE_BIT );
```

Với `configCPU_CLOCK_HZ = SystemCoreClock` và `configTICK_RATE_HZ = 1000`
(`FreeRTOSConfig.h:53-54`).

**(b) Nối vào vector ngắt thật** — `OS/FreeRTOS/include/FreeRTOSConfig.h:130`:

```c
#define xPortSysTickHandler   SysTick_Handler
```

→ Đây **chính là** pattern weak-symbol đã gặp ở Bug #4: hàm trong port layer
được đặt tên thành symbol mà bảng vector ARM đang trỏ tới. Nếu thiếu dòng
`#define` này, `SysTick_Handler` mặc định (weak, rỗng) của ST sẽ chạy →
**tick không bao giờ tăng → toàn hệ thống treo**. Một trong những lỗi cấu hình
FreeRTOS kinh điển nhất.

**(c) Timer Service Task KHÔNG polling** — `OS/FreeRTOS/timers.c:817`:

```c
vQueueWaitForMessageRestricted( xTimerQueue, ( xNextExpireTime - xTimeNow ), xListWasEmpty );
```

Nó tự tính "còn bao lâu tới hạn" rồi **ngủ đúng bấy nhiêu**, không hề hỏi liên
tục "đã tới giờ chưa". Cơ chế đánh thức là **cơ chế block-with-timeout chung
của kernel** — y hệt `xQueueReceive(..., xTicksToWait)` mà bạn đã biết.

> **Kết luận quan trọng nhất của DD-05:** Timer Service Task **không có phần
> cứng riêng, không có đặc quyền lịch trình nào**. Nó chỉ là một task bình
> thường biết tự tính thời gian ngủ. Mọi "phép màu" đều nằm ở SysTick +
> `xTaskIncrementTick()`.

### 6. `configUSE_TICKLESS_IDLE` — vì sao cần biết

`FreeRTOS.h:2702-2703` cho biết mặc định là `0` nếu không khai, và project này
không khai → **tickless idle KHÔNG bật** → SysTick bắn đều 1000 lần/giây kể cả
khi mọi task đều đang ngủ.

Ở project chạy pin, người ta bật `configUSE_TICKLESS_IDLE = 1`: kernel tính
xem task nào sẽ dậy sớm nhất, **tắt SysTick**, cho MCU vào chế độ ngủ sâu, đặt
một nguồn đánh thức khác (thường là RTC), rồi khi dậy **bù lại số tick đã bỏ
lỡ**. Tiết kiệm điện rất lớn. Trả giá: timing kém chính xác hơn, và debug khó
hơn nhiều.

### 7. Độ phân giải — giới hạn cứng phải nhớ

Mọi thứ liên quan thời gian trong FreeRTOS đều lượng tử hoá theo tick:

- Tick = 1 ms → timer **không bao giờ** hết hạn giữa chừng một tick, chỉ có thể
  hết hạn tại mốc 1 ms, 2 ms, 3 ms…
- `vTaskDelay(pdMS_TO_TICKS(5))` thực tế ngủ **4–5 ms** (tick đầu tiên có thể
  đã trôi một phần).
- Cần chính xác hơn 1 ms (PWM, điều khiển động cơ, giao thức có timing chặt)
  → **phải dùng hardware timer riêng** (TIM2…TIM14), không dùng software timer.

Tăng `configTICK_RATE_HZ` để tăng độ phân giải? Được, nhưng mỗi tick là một
ngắt + quét danh sách → tick 10 kHz tốn ~10× overhead CPU. 1000 Hz là điểm cân
bằng phổ biến nhất.

### 8. Bài toán context switch — phân tích đầy đủ một tình huống thật

**Đề bài:** `xWashTimer` (20 s) vừa hết hạn. `WashingManager` (prio 3) đang
Blocked trong `xEventGroupWaitBits`. `UITask` (prio 2) đang Running. Daemon
(prio 2) đang Blocked trên timer queue. Hỏi thứ tự chạy và số lần context
switch tới khi `Manager_OnEnterDrain()` thực thi.

**Priority thật của project** (`app_config.h:61-70`, `FreeRTOSConfig.h:76`):

| Task | Priority |
| --- | --- |
| WashingManager | 3 |
| UITask | 2 |
| SensorTask | 2 |
| **Tmr Svc (daemon)** | **2** |
| OLEDTask | 1 |

#### Bước 1 — UITask nhường CPU cho daemon: KHÔNG phải vì priority

Daemon **bằng** UITask (2 == 2), không cao hơn. `tasks.c:4787` trong
`xTaskIncrementTick()`:

```c
/* ...a context switch should only be performed if the unblocked task's
 * priority is HIGHER than the currently executing task. */
if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )   /* 2 > 2 → FALSE! */
    xSwitchRequired = pdTRUE;
```

Nhánh này **không** kích hoạt. Cái làm nó switch là khối ngay dưới,
`tasks.c:4810-4813`:

```c
#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
    if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > 1U )
        xSwitchRequired = pdTRUE;     /* ← 2 task cùng prio 2 → round-robin */
```

`configUSE_TIME_SLICING` không khai trong project → mặc định `1`
(`FreeRTOS.h:2734`).

> **Bài học chuyển giao:** nếu tắt `configUSE_TIME_SLICING` (một tối ưu phổ
> biến để giảm switch thừa), daemon **sẽ nằm Ready chờ tới khi UITask tự
> block** → callback trễ tuỳ ý. Muốn timer tất định, **đặt
> `configTIMER_TASK_PRIORITY` cao hơn mọi task dùng timer** — đây chính là
> khuyến nghị chính thức của FreeRTOS, và project này đang **không** tuân thủ
> (xem DD-04 §11).

#### Bước 2 — Daemon bị cướp CPU **giữa chừng callback** (điểm dễ bỏ sót nhất)

Callback (`App/Common/Src/freertos_objects.c:49-53`):

```c
static void Timer_PhaseCallback(TimerHandle_t xTimer) {
    xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT);
}
```

`xEventGroupSetBits()` bọc trong `vTaskSuspendAll()` … `xTaskResumeAll()`
(`event_groups.c:567` và `:639`). Bên trong vùng suspend, WashingManager được
gỡ block nhưng **chưa switch được** — scheduler đang treo — nên chỉ đặt cờ
(`tasks.c:4632-4636`):

```c
if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )   /* 3 > 2 → TRUE */
    xYieldPendings[ 0 ] = pdTRUE;    /* hoãn yield tới khi resume */
```

Rồi `xTaskResumeAll()` thấy cờ → `taskYIELD_WITHIN_API()` → **switch ngay tại
đó**.

→ WashingManager chiếm CPU **trước khi `Timer_PhaseCallback()` kịp return**.
Daemon bị bỏ dở giữa lời gọi hàm và **bắt buộc phải được chạy lại sau** để kết
thúc callback + dọn dẹp `prvProcessExpiredTimer()` + quay lại vòng lặp block.

#### Bảng kết quả

| # | Từ → Đến | Nguyên nhân |
| --- | --- | --- |
| 1 | UITask → daemon | SysTick tick, **time-slicing** (prio 2 == 2) |
| 2 | daemon → WashingManager | `xTaskResumeAll()` trong `xEventGroupSetBits()`, **giữa callback** |
| 3 | WashingManager → (một task prio 2) | `Manager_OnEnterDrain()` xong, `WaitBits` block lại |
| 4 | … → daemon (nếu #3 chưa phải daemon) | round-robin |

→ **Tối thiểu 4 lần**, không phải 2.

#### Ở bước 3, CPU về UITask hay daemon trước?

Phụ thuộc con trỏ `pxIndex` của ready-list — `listGET_OWNER_OF_NEXT_ENTRY`
luân phiên qua danh sách (`tasks.c:189-191`):

```c
/* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of
 * the same priority get an equal share of the processor time. */
listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );
```

> **Nguyên tắc:** thứ tự giữa các task **cùng priority là không tất định** theo
> góc nhìn thiết kế. Đừng bao giờ viết code phụ thuộc vào nó. Cần tất định →
> **tách priority**, không dựa vào round-robin.

Thêm một chi tiết: `Manager_OnEnterDrain()` còn gọi
`xQueueSend(xDisplayQueue, …)`, nhưng OLEDTask prio 1 **thấp hơn** nên không
cướp CPU → màn hình chỉ đổi sau khi cả prio 3 lẫn prio 2 rảnh hết. Đây là lý do
đặt task hiển thị ở priority thấp nhất (Part I, Phase 6).

### 9. Scheduler KHÔNG biết gì về logic ứng dụng — tách bạch hai tầng

Câu hỏi thật đã đặt ra: *"Đang giặt rồi thì nhấn nút phải vô tác dụng chứ, sao
UITask vẫn chạy được?"* Đây là chỗ rất dễ trộn lẫn hai tầng:

| Tầng | Câu hỏi | Ai quyết định |
| --- | --- | --- |
| **Scheduler** | UITask có được cấp CPU không? | Priority + Blocked/Ready. **Không biết gì về "đang giặt"** |
| **Ứng dụng** | Lệnh đó có tác dụng không? | State machine của WashingManager |

Scheduler chỉ thấy: UITask prio 2, có message vào `xButtonQueue` → Ready → tới
lượt thì chạy. Nhấn nút lúc nào cũng vậy, không có ngoại lệ.

**Vậy "vô tác dụng" được cài đặt ở đâu?** Ba mảnh ghép trong code thật:

**(1)** UITask **vẫn chạy đủ**, vẫn gửi lệnh (`ui_task.c:69-81`):

```c
} else if (event == BTN_EVENT_CONFIRM) {
    cmd.command = CMD_START_WASH;
    xQueueSend(xCommandQueue, &cmd, 0);     /* ← vẫn gửi bình thường! */
}
```

**(2)** WashingManager ở state WASHING **không hề đọc `xCommandQueue`**
(`washing_manager.c:291-297`) — nó chỉ đọc ở IDLE / FINISH / ERROR
(`:249-254`). Lệnh **nằm im trong queue**, không ai lấy.

**(3)** Vì nằm im nên nó sẽ "nổ chậm" khi máy về IDLE (bấm nút lúc đang giặt →
giặt xong tự khởi động chu trình mới!). Code chặn đúng chỗ đó
(`washing_manager.c:66-67`, và tương tự trong `Manager_OnEnterFinish()` /
`Manager_OnEnterError()`):

```c
/* Clear any stale commands that queued up while the machine was running. */
(void)xQueueReset(xCommandQueue);
```

> **Pattern tổng quát (dùng được ở mọi project):** *lọc sự kiện ở phía
> consumer (state machine), không phải ở phía producer (task nhập liệu).*
> Producer cứ đơn giản và "ngu ngốc"; toàn bộ tri thức về ngữ cảnh nằm gọn một
> chỗ trong state machine. Đúng Design Rule #1 của project. Nếu làm ngược —
> cho UITask tự hỏi "máy đang giặt à?" — thì logic trạng thái bị rải ra 2 nơi,
> và sẽ có lúc hai nơi bất đồng.

**Hệ quả kiểm chứng được trên board:** nhấn nút NAVIGATE (PA0) lúc đang giặt
thì **con trỏ menu trên OLED vẫn di chuyển**, vì UITask gửi thẳng
`xDisplayQueue` không qua WashingManager (`ui_task.c:60-67`). Đó là hệ quả trực
tiếp của kiến trúc trên — feature hay bug là quyết định thiết kế, không phải
tai nạn.

### 10. Lỗi kinh điển liên quan tick

| Lỗi | Hậu quả | Cách tránh |
| --- | --- | --- |
| Thiếu `#define xPortSysTickHandler SysTick_Handler` | Tick không tăng → treo toàn hệ thống ngay sau `vTaskStartScheduler()` | Kiểm tra `FreeRTOSConfig.h` khi port sang board mới |
| HAL cũng cài `SysTick_Handler` riêng | Hai bên tranh nhau vector | Cho HAL dùng TIM khác làm timebase (`HAL_InitTick`) |
| Dùng số tick thay vì `pdMS_TO_TICKS()` | Đổi `configTICK_RATE_HZ` là sai toàn bộ timing | Luôn dùng `pdMS_TO_TICKS()` |
| Kỳ vọng độ chính xác < 1 tick | Timing sai lệch không giải thích được | Hardware timer cho yêu cầu sub-tick |
| Tràn biến tick (`TickType_t` 32-bit ≈ 49.7 ngày @1 kHz) | So sánh thời gian sai sau ~7 tuần | Dùng phép trừ `(now - then)` chứ đừng so sánh `>` trực tiếp; kernel tự xử lý qua overflow list |

### 11. Câu hỏi phỏng vấn

1. *Điều gì xảy ra nếu quên nối `SysTick_Handler` vào FreeRTOS?* → Tick không
   tăng, không task nào rời trạng thái Blocked, hệ thống đứng im sau khi khởi
   động scheduler.
2. *Software timer có dùng phần cứng timer nào không?* → Không. Nó chỉ là danh
   sách sắp xếp trong RAM, được duyệt bởi một task thường, lấy nhịp từ SysTick.
   Một hệ thống có thể có hàng trăm software timer nhưng chỉ tốn một SysTick.
3. *Vì sao callback timer không được block?* → Nó chạy trong ngữ cảnh Timer
   Service Task; block ở đó làm đóng băng **mọi** timer khác (xem DD-04 §5).
4. *Tick 1 kHz với tick 10 kHz khác nhau thế nào?* → Độ phân giải tốt hơn 10
   lần, overhead ngắt cao hơn ~10 lần. Cân bằng theo yêu cầu thật.
5. *Task A và B cùng priority, A đang chạy, B vừa Ready. Khi nào B chạy?* →
   Nếu `configUSE_TIME_SLICING = 1`: ở tick kế tiếp. Nếu `= 0`: chỉ khi A tự
   block hoặc gọi `taskYIELD()`.

---

## Bài học phương pháp (không thuộc riêng khái niệm nào)

1. **Đọc code → dự đoán hành vi → kiểm chứng trên phần cứng.** Nếu kết quả
   khác dự đoán thì hoặc mình hiểu sai code, hoặc có yếu tố thứ ba — cả hai
   đều đáng tìm hiểu. Đây là kỹ năng cốt lõi của embedded engineer.
2. **Comment không được compiler kiểm tra** → nó trôi khỏi code âm thầm.
   Project này đã có 4 lần comment mô tả sai hành vi thật. Khi đọc code lạ,
   luôn xác minh comment bằng code thật trước khi tin.
3. **Trace theo dữ liệu, không chỉ theo luồng gọi hàm.** Bug #17 chỉ lộ ra
   khi lần theo *giá trị của một bit* qua từng bước thao tác người dùng.
4. **Pattern lặp lại quan trọng hơn từng điểm dữ liệu đơn lẻ.** Hai chân GPIO
   khác hẳn nhau cùng đọc LOW → nghi ngờ yếu tố *không đổi* giữa 2 lần đo
   (dây, nút, thiết bị đo), đừng tiếp tục đổi biến số thứ ba.
5. **Kết luận suy ra gián tiếp phải được gắn nhãn "chưa xác nhận"** — đừng
   trình bày giả thuyết như sự thật đã kiểm chứng.

---

---

## Template — Phase mới

### Problem Being Solved

*(mô tả vấn đề)*

---

### Design Overview

*(sơ đồ hoặc mô tả thiết kế)*

---

### FreeRTOS Concept: [Tên concept]

#### ① Problem it solves
#### ② Tại sao cần trong project này
#### ③ Tại sao alternatives bị loại
#### ④ Runtime behavior
#### ⑤ Task state transitions
#### ⑥ Context switching behavior
#### ⑦ Memory implications
#### ⑧ Common mistakes
#### ⑨ Interview-style

---

### Design Decisions

*(giải thích các quyết định thiết kế quan trọng)*

---

### Code Review

**Strengths:**
**Weaknesses:**
**Future improvements:**

---

### What I Should Learn From This Phase

*(bullet points — kiến thức cốt lõi)*

---
