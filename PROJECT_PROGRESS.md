# Washing Machine Simulator — Project Progress

## Project Overview

**Board:** STM32F411E-DISCO (Cortex-M4F, 512 KB Flash, 128 KB RAM)
**RTOS:** FreeRTOS (heap_4, configTICK_RATE_HZ = 1000)
**IDE:** STM32CubeIDE
**Debug:** SEGGER SystemView + ST-Link + Logic Analyzer

---

## Overall Progress

**50 / 59 deliverables complete — 85%**

```
Phase 1  [DONE]         ████████████████████  7/7
Phase 2  [DONE]         ████████████████████  7/7
Phase 3  [DONE]         ████████████████████  5/5
Phase 4  [DONE]         ████████████████████  5/5
Phase 5  [DONE]         ████████████████████  9/9
Phase 6  [DONE]         ████████████████████  4/4
Phase 7  [DONE]         ████████████████████  6/6
Phase 8  [DONE]         ████████████████████  6/6
Phase 9  [IN_PROGRESS]  ████░░░░░░░░░░░░░░░░  1/5
Phase 10 [TODO]         ░░░░░░░░░░░░░░░░░░░░  0/5
```

---

## Phase Checklist

### Phase 1 — Project Setup & Folder Structure `[DONE]` 7/7

**Goal:** Establish a buildable project with correct RTOS configuration and folder layout.

- [x] STM32CubeIDE project created (`Freertos.ioc`)
- [x] FreeRTOS integrated and building
- [x] GPIO configured — PA0 (button input), PD12–PD15 (LED outputs)
- [x] SEGGER SystemView initialized in `main.c` (`SEGGER_SYSVIEW_Conf` / `SEGGER_SYSVIEW_Start`)
- [x] DWT cycle counter enabled in `main.c` (`SCB_DEMCR`, `DWT_CYCCNT`, `DWT_CONTROL`)
- [x] Proof-of-life: 3 blink tasks running on PD12 / PD13 / PD14
- [x] Folder structure established: `App/Common/Inc`, `App/Common/Src`, `App/WashingManager/Inc`, `App/WashingManager/Src`

---

### Phase 2 — Application Data Types & RTOS Objects `[DONE]` 6/7

**Goal:** Define all shared types, message structures, event masks, and FreeRTOS
communication objects before writing any task logic.

- [x] `App/Common/Inc/app_types.h` — `WashState_t`, `WashMode_t`, `ButtonEvent_t`, `Command_t`
- [x] `App/Common/Inc/app_msg.h` — `CommandMsg_t`, `DisplayMsg_t`
- [x] `App/Common/Inc/app_events.h` — `EVT_DOOR_CLOSED`, `EVT_WATER_FULL`, `EVT_TIMEOUT`, `EVT_FINISHED`, `EVT_ERROR`
- [x] `App/Common/Inc/app_config.h` — timing constants, task priorities, queue lengths
- [x] `App/Common/Inc/freertos_objects.h` — extern handle declarations + `App_FreeRTOSObjectsInit()` prototype
- [x] `App/Common/Src/freertos_objects.c` — `xButtonQueue`, `xCommandQueue`, `xDisplayQueue`, `xSystemEvents`, `App_FreeRTOSObjectsInit()`
- [ ] `App_FreeRTOSObjectsInit()` called in `main.c` before `vTaskStartScheduler()`

---

### Phase 3 — UI Task `[DONE]` 5/5

**Goal:** Process button events, manage menu navigation, and generate commands
for WashingManager.

- [x] `App/BSP/Inc/bsp_button.h` — button abstraction (debounce, long-press detection)
- [x] `App/BSP/Src/bsp_button.c` — EXTI ISR handler, posts `ButtonEvent_t` to `xButtonQueue`
- [x] `App/UITask/Inc/ui_task.h` — task prototype
- [x] `App/UITask/Src/ui_task.c` — reads `xButtonQueue`, manages `menuIndex`, posts `CommandMsg_t`
- [x] `UITask` registered with `xTaskCreate()` in `main.c`

---

### Phase 4 — Sensor Task `[DONE]` 5/5

**Goal:** Poll door and water-level sensors periodically and update the system
event group.

- [x] `App/BSP/Inc/bsp_sensor.h` — sensor abstraction
- [x] `App/BSP/Src/bsp_sensor.c` — reads GPIO for simulated door / water level
- [x] `App/SensorTask/Inc/sensor_task.h` — task prototype
- [x] `App/SensorTask/Src/sensor_task.c` — 100 ms periodic loop, sets `EVT_DOOR_CLOSED` / `EVT_WATER_FULL`
- [x] `SensorTask` registered with `xTaskCreate()` in `main.c`

---

### Phase 5 — WashingManager Task `[DONE]` 9/9

**Goal:** Own the state machine, coordinate actuators, and drive the display queue.

- [x] `App/WashingManager/Inc/washing_sm.h` — `WashEvent_t`, `WashingSM_Callbacks_t`, public API
- [x] `App/WashingManager/Src/washing_sm.c` — table-driven SM, 9 transitions, entry-action callbacks
- [x] `App/BSP/Inc/bsp_motor.h` + `App/BSP/Src/bsp_motor.c` — PD12 motor abstraction (see TD-08)
- [x] `App/BSP/Inc/bsp_valve.h` + `App/BSP/Src/bsp_valve.c` — PD13/PD14 valve abstraction (see TD-08)
- [x] `App/BSP/Inc/bsp_led.h` + `App/BSP/Src/bsp_led.c` — PD15 status LED abstraction (see TD-08)
- [x] `App/WashingManager/Inc/washing_manager.h` — task prototype and blocking strategy documentation
- [x] `App/WashingManager/Src/washing_manager.c` — state-dependent blocking loop, SM callbacks, BSP control, display queue updates
- [x] `WashingManagerTask` registered with `xTaskCreate()` in `main.c`
- [x] Blink placeholder tasks removed from `main.c`

---

### Phase 6 — OLED Integration `[DONE]` 4/4

**Goal:** Render system state on the SSD1306 OLED via the Display Queue.

- [x] SSD1306 driver verified and building (I2C1 on PB8/PB9)
- [x] `App/OLEDTask/Inc/oled_task.h` — task prototype
- [x] `App/OLEDTask/Src/oled_task.c` — reads `xDisplayQueue`, renders screen from `DisplayMsg_t`
- [x] `OLEDTask` registered with `xTaskCreate()` in `main.c`

---

### Phase 7 — Software Timers `[DONE]` 6/6

**Goal:** Drive wash / drain / spin durations and fill-water timeout using
FreeRTOS software timers.

- [x] `configUSE_TIMERS 1` enabled in `FreeRTOSConfig.h`
- [x] 4 one-shot timers created in `App_FreeRTOSObjectsInit()`: `xFillTimeoutTimer`, `xWashTimer`, `xDrainTimer`, `xSpinTimer`
- [x] Single shared `Timer_PhaseCallback` sets `EVT_TIMEOUT` on `xSystemEvents`
- [x] `xTimerStart()` called from SM entry-action callbacks in `washing_manager.c`
- [x] `vTaskDelay` placeholders replaced with `xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)` in WASHING / DRAIN / SPIN
- [x] `WASH_EVT_ERROR` added to SM; error transitions added for FILL_WATER / WASHING / DRAIN / SPIN → ERROR

---

### Phase 8 — SystemView Analysis `[DONE]` 6/6

**Goal:** Observe task scheduling, queue operations, and context switches in
the SystemView timeline.

- [x] `SEGGER_SYSVIEW_Conf()` and `SEGGER_SYSVIEW_Start()` called in `main.c`
- [x] DWT cycle counter enabled in `main.c`
- [x] `configQUEUE_REGISTRY_SIZE = 8` in `FreeRTOSConfig.h` (đã có từ trước)
- [x] `vQueueAddToRegistry()` cho `ButtonQ`, `CommandQ`, `DisplayQ` trong `App_FreeRTOSObjectsInit()`
- [x] SystemView kết nối qua ST-Link/SWD — hướng dẫn runtime đã ghi
- [x] Queue names hiện đúng trên timeline nhờ registry

---

### Phase 9 — CPU Load Measurement `[IN_PROGRESS]` 1/5

**Goal:** Measure per-task CPU utilisation using DWT and FreeRTOS run-time stats.

- [x] `DWT_CYCCNT`, `DWT_CONTROL`, `SCB_DEMCR` defined and enabled in `main.c`
- [ ] `configGENERATE_RUN_TIME_STATS = 1` in `FreeRTOSConfig.h`
- [ ] `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()` macro wired to DWT reset
- [ ] `portGET_RUN_TIME_COUNTER_VALUE()` macro wired to `DWT_CYCCNT`
- [ ] `vTaskGetRunTimeStats()` output verified via RTT viewer or UART

---

### Phase 10 — Advanced Features `[TODO]` 0/5

**Goal:** Extend the system with diagnostic and robustness improvements.

- [ ] Stack high-watermark logging (`uxTaskGetStackHighWaterMark`)
- [ ] UART Debug Task — periodic runtime stats output
- [ ] Watchdog integration
- [ ] SD Card logging
- [ ] CLI Interface

---

## Current Focus

### Phase 9 — CPU Load Measurement

Phases 1–8 hoàn thành. Tiếp theo: đo CPU utilization per-task bằng DWT + FreeRTOS run-time stats.

---

## Next Action

1. Enable `configGENERATE_RUN_TIME_STATS = 1` trong `FreeRTOSConfig.h`.
2. Wire `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()` và `portGET_RUN_TIME_COUNTER_VALUE()` vào DWT.
3. Gọi `vTaskGetRunTimeStats()` và xuất kết quả qua SEGGER RTT.

---

## Technical Decisions

### TD-01 — Folder Convention: always `Inc/` and `Src/`
Every module folder contains exactly two subdirectories: `Inc/` for headers
and `Src/` for source files. Headers and source files are never placed in
the same directory.

### TD-02 — FreeRTOS IPC Channel Selection
- **Queue** for Button, Command, and Display channels — data-carrying, FIFO,
  depth > 1, ISR-safe via `FromISR` variants.
- **Event Group** for system-wide status flags — persistent bits, multi-bit
  wait in a single blocking call, broadcast to multiple waiters.
- **Task Notification** reserved for future 1:1 lightweight wake-up signals
  where no data payload and no buffering depth are needed.

### TD-03 — State Machine Pattern: Table-Driven with Entry-Action Callbacks
The SM is a pure-C module with zero FreeRTOS and zero HAL dependency.
All hardware actions are performed via nullable function pointers registered
at init. Adding a transition requires only adding one row to `kTransitions[]`.

### TD-04 — `WashEvent_t` Scoped to `washing_sm.h` Only
`WashEvent_t` is not promoted to `app_types.h` because nothing outside the
WashingManager module constructs or names these values. The WashingManager
task translates `Command_t` and FreeRTOS event bits into `WashEvent_t`
before calling `WashingSM_ProcessEvent()`.

### TD-05 — No Heap Strings in Queue Messages
`DisplayMsg_t.statusText` is a fixed-size `char[24]` array copied by value
into the queue. No `malloc`, no pointer-into-heap, no fragmentation risk.

### TD-06 — `WashingSM_ProcessEvent()` Returns `WashState_t`
Returns the current state after processing so the caller can build a
`DisplayMsg_t` in one step without a second `GetState()` call.

### TD-07 — SEGGER SystemView Initialized Before `vTaskStartScheduler()`
`SEGGER_SYSVIEW_Conf()` and `SEGGER_SYSVIEW_Start()` are called in `main.c`
after hardware init and before task creation, capturing the scheduler start
event and all subsequent task activity from t=0.

### TD-08 — Separate BSP Modules Per Peripheral (not unified actuator module)

`bsp_motor`, `bsp_valve`, and `bsp_led` remain as three independent modules
rather than being merged into a single `bsp_actuators` table-driven module.
Reason: readability and learning value are preferred over abstraction. Each
module's responsibility and pin ownership are immediately clear from the file
name alone. The project is small and educational — the consolidation benefit
does not outweigh the added indirection.

### TD-09 — WashingManager Uses State-Dependent Blocking

The task loop blocks on a different FreeRTOS primitive depending on the
current state rather than using a unified event dispatcher.

- IDLE/FINISH/ERROR: `xQueueReceive(xCommandQueue, portMAX_DELAY)`
- FILL_WATER: `xEventGroupWaitBits(EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR)`
- WASHING/DRAIN/SPIN: `xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)`

Each state waits for exactly the signal it needs, making the task loop
readable as a narrative of machine behaviour.

### TD-10 — One Shared Timer Callback for All Phase Timers

All four software timers (`xWashTimer`, `xDrainTimer`, `xSpinTimer`,
`xFillTimeoutTimer`) share a single `Timer_PhaseCallback` function that sets
`EVT_TIMEOUT`. WashingManager identifies which timer fired by reading the
current SM state — no timer ID needed. This is simpler and equally correct
because only one timer runs at any given time.

### TD-11 — Door Sensor Decoupled from PA0 (Bug #8 fix, see HARDWARE_BUGS.md)

`BSP_Sensor_IsDoorClosed()` no longer reads PA0. First on-hardware manual
test of Phase 5 (long-press to start Wash) showed the machine entering
`FILL_WATER` then immediately faulting to `ERROR` on button release — PA0
was shared between `bsp_button.c` (gesture timing, requires release to fire
long-press) and `bsp_sensor.c` (door simulation, release read as "door
open"). Releasing the button to confirm a command always looked like a
door-open fault within one `SensorTask` poll (100 ms).

Fix: door state is now a plain static flag (`s_doorClosed`, default `1` =
closed), matching how `s_waterFull` already simulates the water sensor —
changed via debugger Live Expressions while the target runs, no GPIO
involved. PA0 is now owned exclusively by `bsp_button.c`.

Files changed: `App/BSP/Src/bsp_sensor.c`, `App/BSP/Inc/bsp_sensor.h`.

Verified after fix: pressing B1 to start Wash now reaches `FILL_WATER` and
stays there for the full 10 s (`WASH_FILL_DURATION_MS`) before timing out to
`ERROR` — the correct designed behavior when `s_waterFull` is never set.

**What to learn:** never let a UI input pin double as a safety-sensor input
— the gesture that confirms a command (button release) and the gesture that
signals danger (door release) can require opposite readings of the same
edge. Simulate ungated sensors as debugger-writable variables, not by
repurposing an existing physical pin.

### Known open issue — Bug #9 (see HARDWARE_BUGS.md)

`UITask`'s `s_statusCache` is initialized once to `"IDLE"` and never updated,
so a short-press during a running cycle briefly overwrites the OLED status
line with a stale `"IDLE"`. Identified during the same test session as
TD-11; not yet fixed — decision on approach still pending.

### Known open issue — Bug #10 (see HARDWARE_BUGS.md)

After a full Wash cycle reaches `FINISH`, the machine stops responding to
button input entirely. `UITask` never sends `CMD_RESET` — its long-press
handler only ever builds `CMD_START_WASH`/`CMD_START_SPIN`, which the SM
silently ignores while in `FINISH`/`ERROR` (no matching transition row).
Recommended fix (not yet applied, pending confirmation): have
`Manager_HandleCommand()` check `WashingSM_GetState()` first and treat any
incoming command as `WASH_EVT_RESET` while in `FINISH`/`ERROR`, keeping
`UITask` unaware of machine state as originally designed.
