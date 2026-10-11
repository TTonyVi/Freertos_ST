# Washing Machine Simulator — Visual Reference

> Đọc file này để nắm nhanh toàn bộ luồng hoạt động của project.
> Mỗi sơ đồ tập trung vào một khía cạnh cụ thể để dễ debug.

---

## 1. Kiến trúc tổng thể — Tasks & Communication Objects

Mô tả: **Ai** giao tiếp với **ai** thông qua **cơ chế nào**.

Hệ thống có ba luồng dữ liệu độc lập, đều hội tụ ở **WashingManager**. Mỗi luồng
một sơ đồ riêng để đọc cho dễ; chú giải màu dùng chung cho cả ba:

| Màu / hình | Loại đối tượng |
| --- | --- |
| 🟠 bầu dục | **ISR** — chạy trong ngắt, chỉ ghi vào queue rồi thoát |
| 🔵 hộp chữ nhật | **Task** |
| 🟢 hình trụ | **Queue** (FIFO, sao chép theo giá trị) |
| 🟣 hộp đôi | **Event Group** |
| ⚪ hình thoi nghiêng | **Software Timer** |
| ⬜ hộp xám | **Phần cứng** (nút bấm, LED, OLED) |

### 1a. Luồng lệnh người dùng — từ nút bấm tới màn hình

Nút bấm → ISR → `xButtonQueue` → UITask → `xCommandQueue` → WashingManager →
`xDisplayQueue` → OLEDTask → màn hình. UITask **không giữ trạng thái nào** và không
gửi gì tới OLED; WashingManager sở hữu con trỏ menu và là bên duy nhất dựng
`DisplayMsg_t`.

```mermaid
%%{init: {"flowchart": {"rankSpacing": 38, "nodeSpacing": 40}}}%%
flowchart TB
    HW_BTN["<b>B1 (PA0)</b><br/>nhấn: chuyển mục menu"]
    HW_PD2["<b>Start (PD2)</b><br/>nhấn: xác nhận / bắt đầu"]

    ISR_BTN(["<b>EXTI0 ISR</b><br/>BSP_Button_IRQHandler<br/>chống dội · NAVIGATE"])
    ISR_PD2(["<b>EXTI2 ISR</b><br/>BSP_StartButton_IRQHandler<br/>chống dội · CONFIRM"])

    Q_BTN[("<b>xButtonQueue</b><br/>ButtonEvent_t · sâu 5<br/>ISR → UITask")]
    UI["<b>UITask</b> · Prio 2<br/>đổi tên nút bấm thành lệnh<br/>(không giữ trạng thái)"]
    Q_CMD[("<b>xCommandQueue</b><br/>CommandMsg_t · sâu 5<br/>UITask → WashingManager")]
    WASH["<b>WashingManager</b> · Prio 3<br/>chủ máy trạng thái<br/>chủ con trỏ menu"]
    Q_DSP[("<b>xDisplayQueue</b><br/>DisplayMsg_t · sâu 5<br/>WashingManager → OLEDTask")]
    OLED["<b>OLEDTask</b> · Prio 1<br/>chỉ vẽ, không logic"]
    HW_OLED["<b>OLED SSD1306</b><br/>I2C1"]

    HW_BTN -->|"cạnh lên/xuống"| ISR_BTN
    HW_PD2 -->|"cạnh xuống"| ISR_PD2
    ISR_BTN -->|"xQueueSendFromISR"| Q_BTN
    ISR_PD2 -->|"xQueueSendFromISR"| Q_BTN
    Q_BTN -->|"xQueueReceive<br/>(chặn)"| UI
    UI -->|"xQueueSend<br/>CMD_NAVIGATE / CMD_CONFIRM"| Q_CMD
    Q_CMD -->|"xQueueReceive<br/>(chỉ ở IDLE · FINISH · ERROR)"| WASH
    WASH -->|"xQueueSend<br/>(mỗi khi đổi trạng thái / con trỏ)"| Q_DSP
    Q_DSP -->|"xQueueReceive<br/>(chặn)"| OLED
    OLED -->|"I2C write"| HW_OLED

    classDef hw    fill:#eceff3,stroke:#6b7280,stroke-width:2px,color:#1f2328
    classDef isr   fill:#fff1e0,stroke:#e8590c,stroke-width:2px,color:#1f2328
    classDef task  fill:#e3f0ff,stroke:#1c64f2,stroke-width:2px,color:#1f2328
    classDef queue fill:#e3f6e8,stroke:#2f9e44,stroke-width:2px,color:#1f2328
    classDef hub   fill:#cfe3ff,stroke:#1c64f2,stroke-width:4px,color:#1f2328
    class HW_BTN,HW_PD2,HW_OLED hw
    class ISR_BTN,ISR_PD2 isr
    class UI,OLED task
    class Q_BTN,Q_CMD,Q_DSP queue
    class WASH hub
```

> **Lưu ý khi đọc:** WashingManager chỉ đọc `xCommandQueue` ở `IDLE`, `FINISH`,
> `ERROR`. Lúc đang chạy chu trình nó chờ trên event group (sơ đồ 1b), nên nút bấm
> trong thời gian đó bị bỏ qua — chủ ý thiết kế, xem START_HERE Tầng 3 §3.6.

### 1b. Luồng sự kiện hệ thống — cảm biến, timer và event group

SensorTask (định kỳ) và các Software Timer (khi hết giờ) **set bit** vào
`xSystemEvents`; WashingManager **chờ** đúng những bit có nghĩa ở trạng thái hiện
tại. WashingManager cũng là bên *khởi động* timer (trong entry-action của từng
trạng thái) và tự set `EVT_FINISHED` / `EVT_ERROR`.

```mermaid
%%{init: {"flowchart": {"rankSpacing": 42, "nodeSpacing": 40}}}%%
flowchart TB
    HW_PD5["<b>Door (PD5)</b><br/>bật/tắt cửa"]
    HW_PD6["<b>Water (PD6)</b><br/>bật/tắt mực nước"]

    SENSOR["<b>SensorTask</b> · Prio 2<br/>đọc PD5 · PD6 mỗi 100 ms"]
    EG[["<b>xSystemEvents</b><br/>Bit0 DOOR_CLOSED · Bit1 WATER_FULL<br/>Bit2 TIMEOUT · Bit3 FINISHED · Bit4 ERROR"]]
    WASH["<b>WashingManager</b> · Prio 3<br/>chờ đúng bit có nghĩa<br/>ở trạng thái hiện tại"]
    TMRS[/"<b>4 Software Timer</b> (one-shot)<br/>Fill 10 s (timeout an toàn) · Wash 20 s<br/>Drain 5 s · Spin 10 s"/]
    TMR_CB["<b>Timer_PhaseCallback()</b><br/>chạy trong Timer Service Task<br/>(không phải ISR)"]

    HW_PD5 -->|"đọc mỗi 100 ms"| SENSOR
    HW_PD6 -->|"đọc mỗi 100 ms"| SENSOR
    SENSOR -->|"set DOOR_CLOSED / WATER_FULL<br/>set ERROR khi cửa mở"| EG
    EG -->|"xEventGroupWaitBits<br/>(chặn giữa chu trình)"| WASH
    WASH -->|"xTimerStart<br/>(entry-action)"| TMRS
    TMRS -->|"hết giờ"| TMR_CB
    TMR_CB -->|"set EVT_TIMEOUT"| EG
    WASH -->|"set FINISHED / ERROR"| EG

    classDef hw    fill:#eceff3,stroke:#6b7280,stroke-width:2px,color:#1f2328
    classDef task  fill:#e3f0ff,stroke:#1c64f2,stroke-width:2px,color:#1f2328
    classDef event fill:#f1e6ff,stroke:#9333ea,stroke-width:2px,color:#1f2328
    classDef timer fill:#eceff3,stroke:#6b7280,stroke-width:2px,color:#1f2328
    classDef hub   fill:#cfe3ff,stroke:#1c64f2,stroke-width:4px,color:#1f2328
    class HW_PD5,HW_PD6 hw
    class SENSOR,TMR_CB task
    class EG event
    class TMRS timer
    class WASH hub
```

### 1c. Luồng điều khiển ngoại vi — WashingManager → BSP → chân GPIO

WashingManager **không bao giờ chạm HAL**: nó gọi các hàm `BSP_*`, lớp BSP mới biết
chân nào là chân nào. Đổi chân hay đổi phần cứng chỉ sửa trong `App/BSP/`.

```mermaid
%%{init: {"flowchart": {"rankSpacing": 40, "nodeSpacing": 30}}}%%
flowchart TB
    WASH["<b>WashingManager</b> · Prio 3<br/>entry-action của từng trạng thái"]
    BSP["<b>Lớp BSP</b><br/>bsp_motor · bsp_valve · bsp_led"]
    MOTOR["<b>Motor</b><br/>PD12 · LD4 xanh lá"]
    INLET["<b>Van cấp nước</b><br/>PD13 · LD3 cam"]
    DRAIN["<b>Van xả</b><br/>PD14 · LD5 đỏ"]
    LED["<b>Đèn báo bận</b><br/>PD15 · LD6 xanh dương"]

    WASH -->|"BSP_Motor_Start/Stop<br/>BSP_Valve_Open/Close<br/>BSP_LED_On/Off"| BSP
    BSP -->|"HAL_GPIO_WritePin"| MOTOR & INLET & DRAIN & LED

    classDef hw    fill:#eceff3,stroke:#6b7280,stroke-width:2px,color:#1f2328
    classDef task  fill:#e3f0ff,stroke:#1c64f2,stroke-width:2px,color:#1f2328
    classDef hub   fill:#cfe3ff,stroke:#1c64f2,stroke-width:4px,color:#1f2328
    class MOTOR,INLET,DRAIN,LED hw
    class BSP task
    class WASH hub
```

---

## 2. State Machine — Tất cả trạng thái và chuyển tiếp

```mermaid
stateDiagram-v2
    [*] --> IDLE : Power on

    IDLE --> FILL_WATER : CMD_CONFIRM (con trỏ = Wash)\n[xCommandQueue]
    IDLE --> SPIN       : CMD_CONFIRM (con trỏ = Spin)\n[xCommandQueue]

    FILL_WATER --> WASHING : EVT_WATER_FULL\n[xSystemEvents Bit1]
    FILL_WATER --> ERROR   : EVT_TIMEOUT (10 s)\n[xFillTimeoutTimer → Bit2]
    FILL_WATER --> ERROR   : EVT_ERROR\n[xSystemEvents Bit4]

    WASHING --> DRAIN : EVT_TIMEOUT (20 s)\n[xWashTimer → Bit2]
    WASHING --> ERROR : EVT_ERROR\n[xSystemEvents Bit4]

    DRAIN --> SPIN  : EVT_TIMEOUT (5 s)\n[xDrainTimer → Bit2]
    DRAIN --> ERROR : EVT_ERROR\n[xSystemEvents Bit4]

    SPIN --> FINISH : EVT_TIMEOUT (10 s)\n[xSpinTimer → Bit2]
    SPIN --> ERROR  : EVT_ERROR\n[xSystemEvents Bit4]

    FINISH --> IDLE : CMD_CONFIRM (xác nhận)\n[xCommandQueue]
    ERROR  --> IDLE : CMD_CONFIRM (xác nhận)\n[xCommandQueue]

    note right of FILL_WATER
        Inlet valve OPEN
        xFillTimeoutTimer starts
        blocks: EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR
    end note

    note right of WASHING
        Inlet valve CLOSE
        Motor START
        xWashTimer starts
        blocks: EVT_TIMEOUT | EVT_ERROR
    end note

    note right of DRAIN
        Motor STOP
        Drain valve OPEN
        xDrainTimer starts
        blocks: EVT_TIMEOUT | EVT_ERROR
    end note

    note right of SPIN
        Drain valve CLOSE
        Motor START
        xSpinTimer starts
        blocks: EVT_TIMEOUT | EVT_ERROR
    end note
```

---

## 3. Kịch bản A — Full Wash Cycle (Bình thường)

**Trigger:** User nhấn PD2 (Start button) khi cursor ở "Wash"

> **Thứ tự "Render":** `xQueueSend` chỉ đưa OLEDTask (Prio 1) sang `Ready`; WashingManager (Prio 3) vẫn chạy tiếp cho tới khi tự block (`xQueueReceive` / `xEventGroupWaitBits`). Chỉ lúc đó OLEDTask mới được chạy và vẽ — nên trong sơ đồ, "Render" nằm **sau** dòng block của WashingManager.

```mermaid
sequenceDiagram
    actor User
    participant ISR0 as EXTI0 ISR (PA0)
    participant ISR1 as EXTI2 ISR (PD2)
    participant UI   as UITask
    participant WASH as WashingManager
    participant SENS as SensorTask
    participant TMR  as Timer Callback
    participant OLED as OLEDTask

    Note over User,OLED: ── Khởi động ──
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")<br/>menuIndex=0
    Note right of OLED: OLEDTask Ready nhưng CHƯA chạy (Prio 1 < 3)
    WASH->>WASH: xQueueReceive(xCommandQueue) → BLOCKED
    OLED->>OLED: Render: title + Wash/Spin menu + "IDLE"<br/>(chạy khi WashingManager đã block)

    Note over User,OLED: ── User điều hướng menu (nhấn PA0) ──
    User->>ISR0: Nhấn PA0
    ISR0->>UI: xQueueSendFromISR(BTN_EVENT_NAVIGATE)
    UI->>WASH: xQueueSend(CMD_NAVIGATE)
    WASH->>WASH: menuIndex = 1 (Spin)
    WASH->>OLED: xQueueSend(DisplayMsg menuIndex=1 "IDLE")
    WASH->>WASH: quay lại xQueueReceive → BLOCKED
    OLED->>OLED: Render: cursor trên "Spin"

    User->>ISR0: Nhấn PA0 lần 2
    ISR0->>UI: xQueueSendFromISR(BTN_EVENT_NAVIGATE)
    UI->>WASH: xQueueSend(CMD_NAVIGATE)
    WASH->>WASH: menuIndex = 0 (Wash)
    WASH->>OLED: xQueueSend(DisplayMsg menuIndex=0 "IDLE")
    WASH->>WASH: quay lại xQueueReceive → BLOCKED
    OLED->>OLED: Render: cursor trở về "Wash"

    Note over User,OLED: ── User xác nhận (nhấn PD2) ──
    User->>ISR1: Nhấn PD2 (Start button)
    ISR1->>UI: xQueueSendFromISR(BTN_EVENT_CONFIRM)
    UI->>WASH: xQueueSend(CMD_CONFIRM)
    WASH->>WASH: IDLE + menuIndex == 0 → WASH_EVT_START_WASH

    Note over User,OLED: ── FILL_WATER phase ──
    WASH->>WASH: SM: IDLE → FILL_WATER
    WASH->>WASH: BSP_Valve_Open(INLET) + BSP_LED_On()
    WASH->>WASH: xTimerStart(xFillTimeoutTimer, 10 s)
    WASH->>OLED: xQueueSend(DisplayMsg "FILL WATER")
    Note right of OLED: OLEDTask chuyển Ready nhưng CHƯA chạy:<br/>Prio 1 < WashingManager Prio 3 → không preempt

    WASH->>WASH: xEventGroupWaitBits(EVT_WATER_FULL|EVT_TIMEOUT|EVT_ERROR)
    Note right of WASH: BLOCKED — waiting for water<br/>CPU rảnh → OLEDTask mới được chạy

    OLED->>OLED: Render "FILL WATER"

    loop Polling every 100 ms
        SENS->>SENS: BSP_Sensor_Poll() → PD6 toggle detected
        SENS->>SENS: BSP_Sensor_IsWaterFull() → true
        SENS->>WASH: xEventGroupSetBits(EVT_WATER_FULL)
    end

    WASH->>WASH: Unblocked: EVT_WATER_FULL received
    WASH->>WASH: xTimerStop(xFillTimeoutTimer)
    WASH->>WASH: SM: FILL_WATER → WASHING

    Note over User,OLED: ── WASHING phase ──
    WASH->>WASH: BSP_Valve_Close(INLET) + BSP_Motor_Start()
    WASH->>WASH: xTimerStart(xWashTimer, 20 s)
    WASH->>OLED: xQueueSend(DisplayMsg "WASHING")

    WASH->>WASH: xEventGroupWaitBits(EVT_TIMEOUT|EVT_ERROR)
    Note right of WASH: BLOCKED — waiting 20 s
    OLED->>OLED: Render "WASHING" (chạy khi WashingManager đã block)

    TMR-->>WASH: xWashTimer fires → xEventGroupSetBits(EVT_TIMEOUT)<br/>(Timer Service Task context, không phải ISR)
    WASH->>WASH: Unblocked: EVT_TIMEOUT
    WASH->>WASH: SM: WASHING → DRAIN

    Note over User,OLED: ── DRAIN phase ──
    WASH->>WASH: BSP_Motor_Stop() + BSP_Valve_Open(DRAIN)
    WASH->>WASH: xTimerStart(xDrainTimer, 5 s)
    WASH->>OLED: xQueueSend(DisplayMsg "DRAIN")

    TMR-->>WASH: xDrainTimer fires → EVT_TIMEOUT
    WASH->>WASH: SM: DRAIN → SPIN

    Note over User,OLED: ── SPIN phase ──
    WASH->>WASH: BSP_Valve_Close(DRAIN) + BSP_Motor_Start()
    WASH->>WASH: xTimerStart(xSpinTimer, 10 s)
    WASH->>OLED: xQueueSend(DisplayMsg "SPINNING")

    TMR-->>WASH: xSpinTimer fires → EVT_TIMEOUT
    WASH->>WASH: SM: SPIN → FINISH

    Note over User,OLED: ── FINISH ──
    WASH->>WASH: BSP_Motor_Stop() + BSP_LED_Off()
    WASH->>WASH: xEventGroupSetBits(EVT_FINISHED)
    WASH->>WASH: xQueueReset(xCommandQueue) (clear stale commands)
    WASH->>OLED: xQueueSend(DisplayMsg "FINISHED")
    WASH->>WASH: xQueueReceive(xCommandQueue) → BLOCKED
    OLED->>OLED: Render "FINISHED" (chạy khi WashingManager đã block)

    Note over User,OLED: ── User acknowledge (nhấn PD2) ──
    User->>ISR1: Nhấn PD2
    ISR1->>UI: BTN_EVENT_CONFIRM
    UI->>WASH: xQueueSend(CMD_CONFIRM)
    WASH->>WASH: CMD_CONFIRM ở FINISH → WASH_EVT_RESET
    WASH->>WASH: SM: FINISH → IDLE
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")
```

---

## 4. Kịch bản B — Spin-Only Cycle

**Trigger:** User nhấn PD2 (Start) khi cursor ở "Spin"

```mermaid
sequenceDiagram
    actor User
    participant ISR1 as EXTI2 ISR (PD2)
    participant UI   as UITask
    participant WASH as WashingManager
    participant TMR  as Timer Callback
    participant OLED as OLEDTask

    User->>ISR1: Nhấn PD2 (cursor = Spin)
    ISR1->>UI: BTN_EVENT_CONFIRM
    UI->>WASH: xQueueSend(CMD_CONFIRM)
    WASH->>WASH: IDLE + menuIndex == 1 → WASH_EVT_START_SPIN

    Note over WASH: SM: IDLE → SPIN<br/>(skip FILL_WATER, WASHING, DRAIN)

    WASH->>WASH: BSP_Valve_Close(DRAIN) + BSP_Motor_Start()
    WASH->>WASH: xTimerStart(xSpinTimer, 10 s)
    WASH->>OLED: xQueueSend(DisplayMsg "SPINNING")

    WASH->>WASH: xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)
    Note right of WASH: BLOCKED 10 s
    OLED->>OLED: Render "SPINNING" (chạy khi WashingManager đã block)

    TMR-->>WASH: xSpinTimer fires → EVT_TIMEOUT
    WASH->>WASH: SM: SPIN → FINISH
    WASH->>WASH: BSP_Motor_Stop()
    WASH->>WASH: xEventGroupSetBits(EVT_FINISHED)
    WASH->>OLED: xQueueSend(DisplayMsg "FINISHED")

    User->>ISR1: Nhấn PD2
    ISR1->>UI: BTN_EVENT_CONFIRM
    UI->>WASH: xQueueSend(CMD_CONFIRM)
    WASH->>WASH: CMD_CONFIRM ở FINISH → WASH_EVT_RESET
    WASH->>WASH: SM: FINISH → IDLE
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")
```

---

## 5. Kịch bản C — Error: Cửa Mở Giữa Chu Trình

**Trigger:** SensorTask phát hiện cửa mở khi đang chạy

```mermaid
sequenceDiagram
    participant SENS as SensorTask
    participant EG   as xSystemEvents
    participant WASH as WashingManager
    participant OLED as OLEDTask
    participant UI   as UITask

    Note over WASH: Đang ở bất kỳ state nào đang chạy
    Note over WASH: (FILL_WATER / WASHING / DRAIN / SPIN)
    WASH->>EG: xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)
    Note right of WASH: BLOCKED — chờ timer hoặc lỗi

    Note over SENS: SensorTask poll mỗi 100 ms
    SENS->>SENS: BSP_Sensor_IsDoorClosed() → FALSE
    SENS->>EG: xEventGroupClearBits(EVT_DOOR_CLOSED)
    SENS->>EG: xEventGroupSetBits(EVT_ERROR)

    EG-->>WASH: Unblocked: EVT_ERROR bit set
    WASH->>WASH: xTimerStop(current phase timer)
    WASH->>WASH: SM: current state → ERROR

    Note over WASH: onEnterError() callback:
    WASH->>WASH: BSP_Motor_Stop()
    WASH->>WASH: BSP_Valve_Close(INLET)
    WASH->>WASH: BSP_Valve_Close(DRAIN)
    WASH->>WASH: BSP_LED_Off()
    WASH->>EG: xEventGroupSetBits(EVT_ERROR)
    WASH->>WASH: xQueueReset(xCommandQueue) (clear stale commands)
    WASH->>OLED: xQueueSend(DisplayMsg "ERROR")

    Note over WASH: SM blocks on xCommandQueue — chờ lệnh từ UITask
    WASH->>WASH: xQueueReceive(xCommandQueue, portMAX_DELAY)
    OLED->>OLED: Render "ERROR" (chạy khi WashingManager đã block)

    Note over SENS: User đóng cửa lại (nhấn PD5) + nhấn PD2 để reset
    actor User
    User->>UI: Nhấn PD2 → BTN_EVENT_CONFIRM
    UI->>WASH: xQueueSend(CMD_CONFIRM)
    Note right of WASH: UITask không biết máy đang ở trạng thái nào —<br/>luôn gửi CMD_CONFIRM, WashingManager tự diễn giải
    WASH->>WASH: CMD_CONFIRM ở ERROR → WASH_EVT_RESET
    WASH->>WASH: SM: ERROR → IDLE
    WASH->>EG: xEventGroupClearBits(EVT_FINISHED | EVT_ERROR)
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")
```

---

## 6. Kịch bản D — Fill Timeout (Nước Không Lên)

**Trigger:** Nước không đạt mức trong 10 giây

```mermaid
flowchart TD
    A([IDLE]) -->|"CMD_CONFIRM (con trỏ = Wash)"| B

    B["onEnterFillWater()\n──────────────────\nBSP_Valve_Open INLET\nBSP_LED_On\nxTimerStart xFillTimeoutTimer 10s"]

    B --> C{"xEventGroupWaitBits\nEVT_WATER_FULL\nEVT_TIMEOUT\nEVT_ERROR\n[BLOCKED]"}

    C -->|"EVT_WATER_FULL\n(bình thường)"| D["xTimerStop(xFillTimeoutTimer)\nSM → WASHING"]

    C -->|"EVT_TIMEOUT\n(10s, nước không lên)"| E["SM → ERROR\nTIMEOUT path"]

    C -->|"EVT_ERROR\n(cửa mở)"| F["xTimerStop(xFillTimeoutTimer)\nSM → ERROR"]

    D --> G([WASHING])
    E --> H([ERROR])
    F --> H

    H --> I{"xQueueReceive\nxCommandQueue\n[BLOCKED]"}
    I -->|"CMD_CONFIRM (xác nhận)"| J([IDLE])

    style B fill:#1b4332,stroke:#2ea043,color:#ffffff
    style C fill:#4a3b00,stroke:#d4a72c,color:#ffffff
    style H fill:#5c1d24,stroke:#f85149,color:#ffffff
    style D fill:#1b4332,stroke:#2ea043,color:#ffffff
    style E fill:#5c1d24,stroke:#f85149,color:#ffffff
    style F fill:#5c1d24,stroke:#f85149,color:#ffffff
```

---

## 7. Button Press Flow — Từ Phần Cứng Đến State Machine

Cả hai nút dùng chung một cơ chế chống dội — **lọc theo khoảng lặng**: một cạnh chỉ
thành cử chỉ nếu đường tín hiệu đã im ít nhất `BUTTON_DEBOUNCE_MS` (50 ms) trước
đó, và **mọi cạnh** (kể cả cạnh bị bỏ) đều nạp lại đồng hồ im lặng. Cả hai nút bắn
sự kiện ngay lúc **nhấn**, không đợi thả tay. UITask chỉ đổi tên cử chỉ thành lệnh;
WashingManager giữ con trỏ và diễn giải lệnh.

```mermaid
%%{init: {"flowchart": {"rankSpacing": 34, "nodeSpacing": 36}}}%%
flowchart TB
    PA0(["<b>PA0 — B1</b><br/>chuyển mục menu"])
    PD2(["<b>PD2 — Start</b><br/>xác nhận / bắt đầu"])

    PA0 -->|"cạnh lên và xuống"| ISR0
    PD2 -->|"cạnh xuống"| ISR1

    ISR0["<b>EXTI0_IRQHandler</b><br/>BSP_Button_IRQHandler()<br/>nạp lại s_lastMenuEdgeMs ở MỌI cạnh"]
    ISR1["<b>EXTI2_IRQHandler</b><br/>BSP_StartButton_IRQHandler()<br/>nạp lại s_lastStartEdgeMs ở MỌI cạnh"]

    ISR0 --> Q0{"đã im ≥ 50 ms<br/>và là cạnh nhấn?"}
    ISR1 --> Q1{"đã im ≥ 50 ms?"}

    Q0 -->|"không"| NOISE0["Nhiễu hoặc cạnh nhả<br/>→ bỏ qua"]
    Q1 -->|"không"| NOISE1["Nhiễu<br/>→ bỏ qua"]
    Q0 -->|"có"| SEND0["xQueueSendFromISR<br/>BTN_EVENT_NAVIGATE<br/>+ portYIELD_FROM_ISR"]
    Q1 -->|"có"| SEND1["xQueueSendFromISR<br/>BTN_EVENT_CONFIRM<br/>+ portYIELD_FROM_ISR"]

    SEND0 --> UI_RECV
    SEND1 --> UI_RECV

    UI_RECV["<b>UITask</b><br/>xQueueReceive (chặn)<br/>đổi tên cử chỉ thành lệnh — không giữ trạng thái"]
    UI_RECV -->|"NAVIGATE → CMD_NAVIGATE<br/>CONFIRM → CMD_CONFIRM"| CMDQ

    CMDQ["xQueueSend(xCommandQueue)"]
    CMDQ --> MGR["<b>WashingManager</b><br/>xQueueReceive(xCommandQueue)<br/>Manager_HandleCommand()<br/>chỉ đọc ở IDLE · FINISH · ERROR"]

    MGR -->|"CMD_NAVIGATE"| CUR["s_menuIndex++ % MENU_ITEM_COUNT<br/>Manager_PostDisplay(trạng thái hiện tại)<br/>→ OLED vẽ lại, chữ trạng thái đúng"]
    MGR -->|"CMD_CONFIRM ở IDLE"| START["con trỏ = Wash → WASH_EVT_START_WASH<br/>con trỏ = Spin → WASH_EVT_START_SPIN"]
    MGR -->|"CMD_CONFIRM ở FINISH / ERROR"| RESET["WASH_EVT_RESET → IDLE"]

    classDef hw    fill:#fff1e0,stroke:#e8590c,stroke-width:2px,color:#1f2328
    classDef isr   fill:#fff1e0,stroke:#e8590c,stroke-width:2px,color:#1f2328
    classDef task  fill:#e3f0ff,stroke:#1c64f2,stroke-width:2px,color:#1f2328
    classDef hub   fill:#cfe3ff,stroke:#1c64f2,stroke-width:4px,color:#1f2328
    classDef queue fill:#e3f6e8,stroke:#2f9e44,stroke-width:2px,color:#1f2328
    classDef bad   fill:#fde8e8,stroke:#d1242f,stroke-width:2px,color:#1f2328
    classDef out   fill:#eceff3,stroke:#6b7280,stroke-width:2px,color:#1f2328
    class PA0,PD2 hw
    class ISR0,ISR1,SEND0,SEND1 isr
    class UI_RECV task
    class CMDQ queue
    class MGR hub
    class NOISE0,NOISE1 bad
    class CUR,START,RESET out
```

> **Giới hạn đã biết của PD2:** EXTI2 chỉ cấu hình cạnh xuống nên ISR không thấy cạnh
> nhả; một chùm dội lúc nhả vẫn có thể lọt thành một `CMD_CONFIRM` thừa. Chi tiết và
> cách khắc phục trong `App/BSP/Src/bsp_button.c`.

---

## 8. Software Timer Flow — Cách Timer Lái State Machine

```mermaid
flowchart LR
    subgraph WashMgr["WashingManager Task (Prio=3)"]
        direction TB
        ENA["xTimerStart(xWashTimer, 20s)\ncalled from onEnterWashing()"]
        WAIT_T["xEventGroupWaitBits\n(EVT_TIMEOUT | EVT_ERROR)\nBLOCKED"]
        PROC["WashingSM_ProcessEvent\n(WASH_EVT_WASH_DONE)\n→ WASHING → DRAIN"]
    end

    subgraph TimerSvc["Timer Service Task (Prio=2, internal FreeRTOS)"]
        direction TB
        COUNT["Tick counter\n20000 ms elapsed"]
        CB["Timer_PhaseCallback()\ncalled by Timer Service Task\n─────────────────────\n(void)xTimer;\nxEventGroupSetBits(EVT_TIMEOUT)"]
    end

    subgraph EvtGrp["xSystemEvents (shared)"]
        BIT["Bit2: EVT_TIMEOUT\n0 → 1"]
    end

    ENA -->|"posts timer command\nto timer queue"| COUNT
    COUNT -->|"calls callback"| CB
    CB -->|"xEventGroupSetBits (plain, KHÔNG phải FromISR)\nTimer Svc Task là task thật, không phải ISR"| BIT
    BIT -->|"unblocks WashingManager\nif waiting on EVT_TIMEOUT"| WAIT_T
    WAIT_T --> PROC
    PROC -->|"onEnterDrain() calls\nxTimerStart(xDrainTimer, 5s)"| ENA

    style CB fill:#4a3b00,stroke:#d4a72c,color:#ffffff
    style BIT fill:#1b4332,stroke:#2ea043,color:#ffffff
    style WAIT_T fill:#0d3b66,stroke:#388bfd,color:#ffffff
```

---

## 9. Task Blocking Map — Mỗi Task Đang Chờ Gì

Bảng này dùng để debug: nếu chương trình treo, xem task nào đang block và tại sao.

```mermaid
flowchart TD
    subgraph "Trong mỗi phase, WashingManager block tại:"
        IDLE_B["STATE: IDLE / FINISH / ERROR\n──────────────────────\nxQueueReceive(xCommandQueue)\nportMAX_DELAY\n─\nChờ user nhấn nút (PA0 chuyển mục · PD2 xác nhận)"]

        FILL_B["STATE: FILL_WATER\n──────────────────────\nxEventGroupWaitBits\n(EVT_WATER_FULL OR EVT_TIMEOUT OR EVT_ERROR)\nportMAX_DELAY\n─\nChờ sensor OR timer 10s OR cửa mở"]

        WASH_B["STATE: WASHING\n──────────────────────\nxEventGroupWaitBits\n(EVT_TIMEOUT OR EVT_ERROR)\nportMAX_DELAY\n─\nChờ timer 20s OR cửa mở"]

        DRAIN_B["STATE: DRAIN\n──────────────────────\nxEventGroupWaitBits\n(EVT_TIMEOUT OR EVT_ERROR)\nportMAX_DELAY\n─\nChờ timer 5s OR cửa mở"]

        SPIN_B["STATE: SPIN\n──────────────────────\nxEventGroupWaitBits\n(EVT_TIMEOUT OR EVT_ERROR)\nportMAX_DELAY\n─\nChờ timer 10s OR cửa mở"]
    end

    subgraph "Các task còn lại luôn block tại:"
        UI_B["UITask\n──────────────────────\nxQueueReceive(xButtonQueue)\nportMAX_DELAY\n─\nChờ button press"]

        SENS_B["SensorTask\n──────────────────────\nvTaskDelay(100ms)\n─\nPeriodic poll (không block event)"]

        OLED_B["OLEDTask\n──────────────────────\nxQueueReceive(xDisplayQueue)\nportMAX_DELAY\n─\nChờ display message"]
    end
```

**Debug tip:** Nếu màn hình không update → OLEDTask đang chờ message ở xDisplayQueue. Kiểm tra WashingManager có gọi `Manager_PostDisplay()` không.

**Debug tip:** Nếu machine không start khi nhấn PD2 → kiểm tra theo thứ tự: (1) `Freertos.ioc`/`main.c` — `MX_GPIO_Init()` có `HAL_NVIC_EnableIRQ(EXTI2_IRQn)` với priority 5 không; (2) `EXTI2_IRQHandler()` trong `stm32f4xx_it.c` có thực sự gọi `BSP_StartButton_IRQHandler()` trong `USER CODE BEGIN EXTI2_IRQn 0` không (từng bị thiếu dòng này — xem [HARDWARE_BUGS.md § Bug #11](HARDWARE_BUGS.md#bug-11--pe1-startconfirm-chết-hoàn-toàn-quên-gọi-irq-handler)). GPIO/NVIC của PA0 và PD2 hiện do CubeMX generate hoàn toàn — không còn hàm `BSP_Button_Init()`/`BSP_StartButton_Init()` nữa; `ui_task.c` chỉ gọi `BSP_Button_RegisterCallback()`.

---

## 10. OLED Screen Layout

```
Pixel coordinates: x=0..127 (left→right), y=0..63 (top→bottom)
Font: 6×8 px per character → 21 chars/row, 8 rows max

┌────────────────────────────────────────────┐  y=0
│ (empty — always black)                     │  Rows 0–1 (y pixel 0–15)
├────────────────────────────────────────────┤  y=16
│   Washing Machine                          │  Row 2 (y pixel 16–23)
├────────────────────────────────────────────┤  y=24
│ > Wash    ← menuIndex == 0                 │  Row 3 (y pixel 24–31)
│   Wash    ← menuIndex != 0                 │
├────────────────────────────────────────────┤  y=32
│ > Spin    ← menuIndex == 1                 │  Row 4 (y pixel 32–39)
│   Spin    ← menuIndex != 1                 │
├────────────────────────────────────────────┤  y=40
│ IDLE / FILL WATER / WASHING /              │  Row 5 (y pixel 40–47)
│ DRAIN / SPINNING / FINISHED / ERROR        │
├────────────────────────────────────────────┤  y=48
│ (empty — always black)                     │  Rows 6–7 (y pixel 48–63)
└────────────────────────────────────────────┘  y=63

Owner phân quyền:
  Row 2: hardcoded trong oled_task.c
  Row 3–4: menuIndex từ DisplayMsg_t (owner: WashingManager — con trỏ menu)
  Row 5: statusText từ DisplayMsg_t (owner: WashingManager)
```

---

## 11. FreeRTOS Object Summary

| Object | Type | Depth/Bits | Producer | Consumer | Mechanism |
|--------|------|-----------|----------|----------|-----------|
| `xButtonQueue` | Queue | 5 items | ISR EXTI0 (PA0) + ISR EXTI2 (PD2) | UITask | `xQueueSendFromISR` / `xQueueReceive` |
| `xCommandQueue` | Queue | 5 items | UITask | WashingManager | `xQueueSend` / `xQueueReceive` |
| `xDisplayQueue` | Queue | 5 items | WashingManager (duy nhất) | OLEDTask | `xQueueSend` / `xQueueReceive` |
| `xSystemEvents` | Event Group | 5 bits | SensorTask, TimerCb, WashMgr | WashingManager | `xEventGroupSetBits` / `xEventGroupWaitBits` |
| `xFillTimeoutTimer` | SW Timer | One-shot 10 s (timeout an toàn) | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xWashTimer` | SW Timer | One-shot 20 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xDrainTimer` | SW Timer | One-shot 5 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xSpinTimer` | SW Timer | One-shot 10 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |

**Không có Mutex/Semaphore trong project hiện tại:**
- OLED access: chỉ có OLEDTask mới gọi BSP_OLED_* → không cần mutex
- State machine: chỉ có WashingManager mới gọi WashingSM_* → không cần mutex
- Sensor data: SensorTask write, WashingManager read qua Event Group (atomic) → không cần mutex
