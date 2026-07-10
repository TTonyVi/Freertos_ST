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
                    ? BTN_EVENT_LONG_PRESS
                    : BTN_EVENT_SHORT_PRESS;
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
