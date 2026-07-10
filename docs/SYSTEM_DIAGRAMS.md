# Washing Machine Simulator — Visual Reference

> Đọc file này để nắm nhanh toàn bộ luồng hoạt động của project.
> Mỗi sơ đồ tập trung vào một khía cạnh cụ thể để dễ debug.

---

## 1. Kiến trúc tổng thể — Tasks & Communication Objects

Mô tả: **Ai** giao tiếp với **ai** thông qua **cơ chế nào**.

```mermaid
flowchart TD
    %% --- Hardware layer ---
    HW_BTN["B1 Button (PA0)"]
    HW_OLED["OLED SSD1306 (I2C)"]
    HW_MOTOR["Motor GPIO (PD12)"]
    HW_INLET["Inlet Valve GPIO (PD13)"]
    HW_DRAIN["Drain Valve GPIO (PD14)"]
    HW_LED["Status LED GPIO (PD15)"]

    %% --- ISR layer ---
    ISR_BTN["EXTI0_IRQHandler\n─────────────────\nBSP_Button_IRQHandler()\nDebounce + classify\n[ISR CONTEXT]"]
    ISR_TMR["Timer_PhaseCallback()\n─────────────────\nset EVT_TIMEOUT\n[Timer Service Task]"]

    %% --- Queue / Event objects ---
    Q_BTN[("xButtonQueue\n─────────────\nItem: ButtonEvent_t\nDepth: 5\nISR → UITask")]
    Q_CMD[("xCommandQueue\n─────────────\nItem: CommandMsg_t\nDepth: 5\nUITask → WashMgr")]
    Q_DSP[("xDisplayQueue\n─────────────\nItem: DisplayMsg_t\nDepth: 5\nWashMgr → OLED")]
    EG[["xSystemEvents\n─────────────────────\nBit0 EVT_DOOR_CLOSED\nBit1 EVT_WATER_FULL\nBit2 EVT_TIMEOUT\nBit3 EVT_FINISHED\nBit4 EVT_ERROR"]]

    %% --- Timers ---
    T1[/"xFillTimeoutTimer\nOne-shot 10 s"/]
    T2[/"xWashTimer\nOne-shot 20 s"/]
    T3[/"xDrainTimer\nOne-shot 5 s"/]
    T4[/"xSpinTimer\nOne-shot 10 s"/]

    %% --- Tasks ---
    UI["UITask  Prio=2\n─────────────────\nMenu cursor\nButton interpreter"]
    WASH["WashingManager  Prio=3\n─────────────────\nState machine owner\nActuator control"]
    SENSOR["SensorTask  Prio=2\n─────────────────\nPoll door + water\nevery 100 ms"]
    OLED["OLEDTask  Prio=1\n─────────────────\nOnly renderer\nNever blocks others"]

    %% --- Connections ---
    HW_BTN -->|"RISING/FALLING edge"| ISR_BTN
    ISR_BTN -->|"xQueueSendFromISR"| Q_BTN
    Q_BTN -->|"xQueueReceive\nblocks"| UI
    UI -->|"xQueueSend"| Q_CMD

    Q_CMD -->|"xQueueReceive\nblocks (IDLE state)"| WASH
    WASH -->|"xQueueSend"| Q_DSP
    Q_DSP -->|"xQueueReceive\nblocks"| OLED
    OLED -->|"I2C write"| HW_OLED

    SENSOR -->|"xEventGroupSetBits\nDOOR_CLOSED / WATER_FULL"| EG
    SENSOR -->|"xEventGroupSetBits\nEVT_ERROR (door open)"| EG
    EG -->|"xEventGroupWaitBits\nblocks mid-cycle"| WASH

    WASH -->|"xTimerStart\n(entry action)"| T1 & T2 & T3 & T4
    T1 & T2 & T3 & T4 -->|"fires callback"| ISR_TMR
    ISR_TMR -->|"xEventGroupSetBitsFromISR\nEVT_TIMEOUT"| EG
    WASH -->|"xEventGroupSetBits\nEVT_FINISHED / EVT_ERROR"| EG

    WASH -->|"BSP calls"| HW_MOTOR & HW_INLET & HW_DRAIN & HW_LED
```

---

## 2. State Machine — Tất cả trạng thái và chuyển tiếp

```mermaid
stateDiagram-v2
    [*] --> IDLE : Power on

    IDLE --> FILL_WATER : CMD_START_WASH\n[xCommandQueue]
    IDLE --> SPIN       : CMD_START_SPIN\n[xCommandQueue]

    FILL_WATER --> WASHING : EVT_WATER_FULL\n[xSystemEvents Bit1]
    FILL_WATER --> ERROR   : EVT_TIMEOUT (10 s)\n[xFillTimeoutTimer → Bit2]

    WASHING --> DRAIN : EVT_TIMEOUT (20 s)\n[xWashTimer → Bit2]
    WASHING --> ERROR : EVT_ERROR\n[xSystemEvents Bit4]

    DRAIN --> SPIN  : EVT_TIMEOUT (5 s)\n[xDrainTimer → Bit2]
    DRAIN --> ERROR : EVT_ERROR\n[xSystemEvents Bit4]

    SPIN --> FINISH : EVT_TIMEOUT (10 s)\n[xSpinTimer → Bit2]
    SPIN --> ERROR  : EVT_ERROR\n[xSystemEvents Bit4]

    FINISH --> IDLE : CMD_RESET\n[xCommandQueue]
    ERROR  --> IDLE : CMD_RESET\n[xCommandQueue]

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

**Trigger:** User long-press button khi cursor ở "Wash"

```mermaid
sequenceDiagram
    actor User
    participant ISR  as EXTI0 ISR
    participant UI   as UITask
    participant WASH as WashingManager
    participant SENS as SensorTask
    participant TMR  as Timer Callback
    participant OLED as OLEDTask

    Note over User,OLED: ── Khởi động ──
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")<br/>menuIndex=0
    OLED->>OLED: Render: title + Wash/Spin menu + "IDLE"

    Note over User,OLED: ── User điều hướng menu (short press) ──
    User->>ISR: Nhấn nhanh (< 1500 ms)
    ISR->>UI: xQueueSendFromISR(BTN_EVENT_SHORT_PRESS)
    UI->>UI: menuIndex = 1 (Spin)
    UI->>OLED: xQueueSend(DisplayMsg menuIndex=1 "IDLE")
    OLED->>OLED: Render: cursor trên "Spin"

    User->>ISR: Nhấn nhanh lần 2
    ISR->>UI: xQueueSendFromISR(BTN_EVENT_SHORT_PRESS)
    UI->>UI: menuIndex = 0 (Wash)
    UI->>OLED: xQueueSend(DisplayMsg menuIndex=0 "IDLE")
    OLED->>OLED: Render: cursor trở về "Wash"

    Note over User,OLED: ── User xác nhận (long press ≥ 1500 ms) ──
    User->>ISR: Giữ nút ≥ 1500 ms rồi thả
    ISR->>UI: xQueueSendFromISR(BTN_EVENT_LONG_PRESS)
    UI->>WASH: xQueueSend(CMD_START_WASH, WASH_MODE_WASH, menuIndex=0)

    Note over User,OLED: ── FILL_WATER phase ──
    WASH->>WASH: SM: IDLE → FILL_WATER
    WASH->>WASH: BSP_Valve_Open(INLET) + BSP_LED_On()
    WASH->>WASH: xTimerStart(xFillTimeoutTimer, 10 s)
    WASH->>OLED: xQueueSend(DisplayMsg "FILL WATER")
    OLED->>OLED: Render "FILL WATER"

    WASH->>WASH: xEventGroupWaitBits(EVT_WATER_FULL|EVT_TIMEOUT|EVT_ERROR)
    Note right of WASH: BLOCKED — waiting for water

    loop Polling every 100 ms
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
    OLED->>OLED: Render "WASHING"

    WASH->>WASH: xEventGroupWaitBits(EVT_TIMEOUT|EVT_ERROR)
    Note right of WASH: BLOCKED — waiting 20 s

    TMR-->>WASH: xWashTimer fires → xEventGroupSetBitsFromISR(EVT_TIMEOUT)
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
    WASH->>OLED: xQueueSend(DisplayMsg "FINISHED")
    OLED->>OLED: Render "FINISHED"

    Note over User,OLED: ── User acknowledge ──
    User->>ISR: Long press
    ISR->>UI: BTN_EVENT_LONG_PRESS
    UI->>WASH: xQueueSend(CMD_RESET)
    WASH->>WASH: SM: FINISH → IDLE
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")
```

---

## 4. Kịch bản B — Spin-Only Cycle

**Trigger:** User long-press khi cursor ở "Spin"

```mermaid
sequenceDiagram
    actor User
    participant UI   as UITask
    participant WASH as WashingManager
    participant TMR  as Timer Callback
    participant OLED as OLEDTask

    User->>UI: Long press (cursor = Spin)
    UI->>WASH: xQueueSend(CMD_START_SPIN, WASH_MODE_SPIN, menuIndex=1)

    Note over WASH: SM: IDLE → SPIN<br/>(skip FILL_WATER, WASHING, DRAIN)

    WASH->>WASH: BSP_Valve_Close(DRAIN) + BSP_Motor_Start()
    WASH->>WASH: xTimerStart(xSpinTimer, 10 s)
    WASH->>OLED: xQueueSend(DisplayMsg "SPINNING")
    OLED->>OLED: Render "SPINNING"

    WASH->>WASH: xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)
    Note right of WASH: BLOCKED 10 s

    TMR-->>WASH: xSpinTimer fires → EVT_TIMEOUT
    WASH->>WASH: SM: SPIN → FINISH
    WASH->>WASH: BSP_Motor_Stop()
    WASH->>WASH: xEventGroupSetBits(EVT_FINISHED)
    WASH->>OLED: xQueueSend(DisplayMsg "FINISHED")

    User->>UI: Long press
    UI->>WASH: CMD_RESET
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
    WASH->>OLED: xQueueSend(DisplayMsg "ERROR")
    OLED->>OLED: Render "ERROR"

    Note over WASH: SM blocks on xCommandQueue — chờ CMD_RESET
    WASH->>WASH: xQueueReceive(xCommandQueue, portMAX_DELAY)

    Note over SENS: User đóng cửa lại + reset
    actor User
    User->>WASH: Long press → CMD_RESET
    WASH->>WASH: SM: ERROR → IDLE
    WASH->>EG: xEventGroupClearBits(EVT_FINISHED | EVT_ERROR)
    WASH->>OLED: xQueueSend(DisplayMsg "IDLE")
```

---

## 6. Kịch bản D — Fill Timeout (Nước Không Lên)

**Trigger:** Nước không đạt mức trong 10 giây

```mermaid
flowchart TD
    A([IDLE]) -->|CMD_START_WASH| B

    B["onEnterFillWater()\n──────────────────\nBSP_Valve_Open INLET\nBSP_LED_On\nxTimerStart xFillTimeoutTimer 10s"]

    B --> C{"xEventGroupWaitBits\nEVT_WATER_FULL\nEVT_TIMEOUT\nEVT_ERROR\n[BLOCKED]"}

    C -->|"EVT_WATER_FULL\n(bình thường)"| D["xTimerStop(xFillTimeoutTimer)\nSM → WASHING"]

    C -->|"EVT_TIMEOUT\n(10s, nước không lên)"| E["SM → ERROR\nTIMEOUT path"]

    C -->|"EVT_ERROR\n(cửa mở)"| F["xTimerStop(xFillTimeoutTimer)\nSM → ERROR"]

    D --> G([WASHING])
    E --> H([ERROR])
    F --> H

    H --> I{"xQueueReceive\nxCommandQueue\n[BLOCKED]"}
    I -->|CMD_RESET| J([IDLE])

    style B fill:#d4edda,stroke:#28a745
    style C fill:#fff3cd,stroke:#ffc107
    style H fill:#f8d7da,stroke:#dc3545
    style D fill:#d4edda,stroke:#28a745
    style E fill:#f8d7da,stroke:#dc3545
    style F fill:#f8d7da,stroke:#dc3545
```

---

## 7. Button Press Flow — Từ Phần Cứng Đến State Machine

```mermaid
flowchart TD
    HW(["B1 Button PA0\nPhysical press"])

    HW -->|"RISING edge\nPA0 = HIGH"| ISR1

    ISR1["EXTI0_IRQHandler\n──────────────────\nBSP_Button_IRQHandler()\nHAL_GPIO_ReadPin = HIGH\ns_pressStartMs = HAL_GetTick()\ns_pressed = 1"]

    ISR1 -->|"returns from ISR\nwaits for release"| WAIT((" "))

    WAIT -->|"FALLING edge\nPA0 = LOW"| ISR2

    ISR2["EXTI0_IRQHandler\n──────────────────\nBSP_Button_IRQHandler()\nHAL_GPIO_ReadPin = LOW\nduration = HAL_GetTick() - s_pressStartMs"]

    ISR2 --> DEB{"duration < 50ms?"}
    DEB -->|YES| NOISE["Noise — ignore\nNo queue send"]
    DEB -->|NO| LONG{"duration ≥ 1500ms?"}

    LONG -->|YES = Long Press| L_SEND["xQueueSendFromISR\n(BTN_EVENT_LONG_PRESS)\n+ portYIELD_FROM_ISR"]
    LONG -->|NO = Short Press| S_SEND["xQueueSendFromISR\n(BTN_EVENT_SHORT_PRESS)\n+ portYIELD_FROM_ISR"]

    L_SEND --> UI_RECV
    S_SEND --> UI_RECV

    UI_RECV["UITask\nxQueueReceive\n(blocks until event)"]

    UI_RECV -->|BTN_EVENT_SHORT_PRESS| CURSOR["menuIndex++\n% MENU_ITEM_COUNT\n────────────────────\nxQueueSend(xDisplayQueue)\n{ menuIndex, s_statusCache }"]

    UI_RECV -->|BTN_EVENT_LONG_PRESS| CMD_BUILD{"menuIndex == 0?"}

    CMD_BUILD -->|"YES (Wash)"| WASH_CMD["cmd = { CMD_START_WASH\nWASH_MODE_WASH\nmenuIndex }"]
    CMD_BUILD -->|"NO (Spin)"| SPIN_CMD["cmd = { CMD_START_SPIN\nWASH_MODE_SPIN\nmenuIndex }"]

    WASH_CMD --> SEND_CMD["xQueueSend(xCommandQueue)"]
    SPIN_CMD --> SEND_CMD

    SEND_CMD --> MGR["WashingManager\nxQueueReceive(xCommandQueue)\nManager_HandleCommand()"]

    MGR -->|CMD_START_WASH| SM_W["WashingSM_ProcessEvent\n(WASH_EVT_START_WASH)\n→ IDLE → FILL_WATER"]
    MGR -->|CMD_START_SPIN| SM_S["WashingSM_ProcessEvent\n(WASH_EVT_START_SPIN)\n→ IDLE → SPIN"]

    CURSOR --> OLED_UPD["OLEDTask\nxQueueReceive(xDisplayQueue)\nRender cursor move"]

    style NOISE fill:#f8d7da,stroke:#dc3545
    style L_SEND fill:#cce5ff,stroke:#004085
    style S_SEND fill:#cce5ff,stroke:#004085
    style CURSOR fill:#d4edda,stroke:#28a745
    style SEND_CMD fill:#cce5ff,stroke:#004085
```

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
    CB -->|"xEventGroupSetBitsFromISR\n(safe from Timer Svc context)"| BIT
    BIT -->|"unblocks WashingManager\nif waiting on EVT_TIMEOUT"| WAIT_T
    WAIT_T --> PROC
    PROC -->|"onEnterDrain() calls\nxTimerStart(xDrainTimer, 5s)"| ENA

    style CB fill:#fff3cd,stroke:#ffc107
    style BIT fill:#d4edda,stroke:#28a745
    style WAIT_T fill:#cce5ff,stroke:#004085
```

---

## 9. Task Blocking Map — Mỗi Task Đang Chờ Gì

Bảng này dùng để debug: nếu chương trình treo, xem task nào đang block và tại sao.

```mermaid
flowchart TD
    subgraph "Trong mỗi phase, WashingManager block tại:"
        IDLE_B["STATE: IDLE / FINISH / ERROR\n──────────────────────\nxQueueReceive(xCommandQueue)\nportMAX_DELAY\n─\nChờ user long-press"]

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

**Debug tip:** Nếu machine không start khi long-press → WashingManager đang block đúng chỗ không? Dùng SEGGER SystemView xem task state.

---

## 10. OLED Screen Layout

```
Pixel coordinates: x=0..127 (left→right), y=0..63 (top→bottom)
Font: 6×8 px per character → 21 chars/row, 8 rows max

┌────────────────────────────────────────────┐  y=0
│ Washing Machine                            │  Row 0 (y pixel 0–7)
├────────────────────────────────────────────┤  y=8
│ > Wash    ← menuIndex == 0                 │  Row 1 (y pixel 8–15)
│   Wash    ← menuIndex != 0                 │
├────────────────────────────────────────────┤  y=16
│ > Spin    ← menuIndex == 1                 │  Row 2 (y pixel 16–23)
│   Spin    ← menuIndex != 1                 │
├────────────────────────────────────────────┤  y=24
│ IDLE / FILL WATER / WASHING /              │  Row 3 (y pixel 24–31)
│ DRAIN / SPINNING / FINISHED / ERROR        │
├────────────────────────────────────────────┤  y=32
│ (empty — always black)                     │  Rows 4–7 (y pixel 32–63)
└────────────────────────────────────────────┘  y=63

Owner phân quyền:
  Row 0: hardcoded trong oled_task.c
  Row 1–2: menuIndex từ DisplayMsg_t (owner: UITask → truyền qua CommandMsg_t)
  Row 3: statusText từ DisplayMsg_t (owner: WashingManager)
```

---

## 11. FreeRTOS Object Summary

| Object | Type | Depth/Bits | Producer | Consumer | Mechanism |
|--------|------|-----------|----------|----------|-----------|
| `xButtonQueue` | Queue | 5 items | ISR (EXTI0) | UITask | `xQueueSendFromISR` / `xQueueReceive` |
| `xCommandQueue` | Queue | 5 items | UITask | WashingManager | `xQueueSend` / `xQueueReceive` |
| `xDisplayQueue` | Queue | 5 items | WashingManager, UITask | OLEDTask | `xQueueSend` / `xQueueReceive` |
| `xSystemEvents` | Event Group | 5 bits | SensorTask, TimerCb, WashMgr | WashingManager | `xEventGroupSetBits` / `xEventGroupWaitBits` |
| `xFillTimeoutTimer` | SW Timer | One-shot 10 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xWashTimer` | SW Timer | One-shot 20 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xDrainTimer` | SW Timer | One-shot 5 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |
| `xSpinTimer` | SW Timer | One-shot 10 s | WashingManager | TimerServiceTask | `xTimerStart` / callback |

**Không có Mutex/Semaphore trong project hiện tại:**
- OLED access: chỉ có OLEDTask mới gọi BSP_OLED_* → không cần mutex
- State machine: chỉ có WashingManager mới gọi WashingSM_* → không cần mutex
- Sensor data: SensorTask write, WashingManager read qua Event Group (atomic) → không cần mutex
