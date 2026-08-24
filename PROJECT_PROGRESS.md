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

Initial fix: door state became a plain static flag (`s_doorClosed`, default
`1` = closed), changed via debugger Live Expressions.

Final fix (TD-12): sensors are now driven by dedicated physical buttons
(PD5 = door, PD6 = water) — no debugger interaction needed at all.

**What to learn:** never let a UI input pin double as a safety-sensor input.
Simulate sensors via dedicated inputs, not repurposed UI pins.

### TD-12 — Physical Button Hardware Upgrade for Sensor & UI Simulation

All simulation inputs are now driven by dedicated external push-buttons,
eliminating the need for a debugger connection during testing.

| Pin | Role | Interface | Behavior |
|-----|------|-----------|----------|
| PA0 | Navigate menu | EXTI0, both edges, pull-down | Short press → next menu item |
| PD4 | Start / Confirm | EXTI4, falling edge, pull-up | Any press → start selected mode |
| PD5 | Door sensor sim | GPIO Input, pull-up, polling 100 ms | Toggle press → open/close door |
| PD6 | Water sensor sim | GPIO Input, pull-up, polling 100 ms | Toggle press → full/empty water |

**Why EXTI for PD4, polling for PD5/PD6?**
PD4 uses EXTI4 — no conflict with EXTI0 (PA0). Using EXTI gives immediate,
ISR-driven response for the Start button. PD5/PD6 are toggle switches polled
every 100 ms by SensorTask — latency is acceptable and avoids adding two
more ISR handlers.

**Why PD4 for Start instead of a second gesture on PA0?**
Separating UI input (PA0) from confirmation (PD4) eliminates long-press
timing complexity and makes the gesture model unambiguous. Any press on PD4
immediately fires `BTN_EVENT_LONG_PRESS`, which UITask already handles
to send the start command — no UITask code change required.

**Default sensor states at startup:**
- Door: CLOSED (`s_doorClosed = 1`) — machine starts in safe state
- Water: EMPTY (`s_waterFull = 0`) — machine waits for water fill signal

Files changed: `App/BSP/Inc/bsp_button.h`, `App/BSP/Src/bsp_button.c`,
`App/BSP/Inc/bsp_sensor.h`, `App/BSP/Src/bsp_sensor.c`,
`App/SensorTask/Src/sensor_task.c`, `Core/Src/stm32f4xx_it.c`,
`App/UITask/Src/ui_task.c`, `Core/Src/main.c`.

### Known open issue — Bug #9 (see HARDWARE_BUGS.md)

`UITask`'s `s_statusCache` is initialized once to `"IDLE"` and never updated,
so a short-press during a running cycle briefly overwrites the OLED status
line with a stale `"IDLE"`. Not yet fixed — decision on approach pending.

### Resolved issue — Bug #10 (see HARDWARE_BUGS.md)

After a full Wash cycle reaches `FINISH`, the machine stops responding to button input entirely because `UITask` never sends `CMD_RESET`. Fixed by having `Manager_HandleCommand()` check `WashingSM_GetState()` first and interpret any incoming command as `WASH_EVT_RESET` when in `FINISH`/`ERROR` states. This keeps `UITask` unaware of machine state as originally designed.

### Change Record — Syncing Code & Diagrams (2026-08-21)

- **Goal:** Fix critical bugs and synchronize codebase with system diagrams.
- **Files changed:**
  - [`sensor_task.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/SensorTask/Src/sensor_task.c)
  - [`washing_manager.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Src/washing_manager.c)
  - [`SYSTEM_DIAGRAMS.md`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/SYSTEM_DIAGRAMS.md)
- **What changed:**
  - Added missing call to `BSP_Sensor_Poll()` inside `SensorTask` loop.
  - Implemented automatic mapping of incoming commands to `WASH_EVT_RESET` in `Manager_HandleCommand()` when in `FINISHED` or `ERROR` states.
  - Added `xQueueReset(xCommandQueue)` calls in `Manager_OnEnterIdle()`, `Manager_OnEnterFinish()`, and `Manager_OnEnterError()` to clear queued commands.
  - Updated Mermaid state diagram, OLED row coordinates (from Row 0–3 to Row 2–5), and sequence diagrams in `SYSTEM_DIAGRAMS.md` to match actual implementation.
- **Why it changed:**
  - Without `BSP_Sensor_Poll()`, the physical buttons for simulated sensors (PD5 and PD6) were never read, preventing water level and door state simulation.
  - Without the reset mapping, the machine was stuck in terminal states because `UITask` could not send `CMD_RESET`.
  - Stale command presses made while running were piling up in the command queue, causing unintended double starts or resets when the cycle completed. Resetting the queue prevents this.
  - The diagrams were out of date and mismatched with the working codebase.
- **What I should learn from it:**
  - Ensure all driver/BSP polling functions are actually called in their corresponding tasks.
  - Context-sensitive command translation inside the manager task preserves module decoupling.
  - Keep project diagrams synchronized with the implementation to prevent documentation decay.

### Change Record — Refactoring Button Events (2026-08-21)

- **Goal:** Rename outdated button event enums to match current decoupled hardware setup.
- **Files changed:**
  - [`app_types.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc/app_types.h)
  - [`bsp_button.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc/bsp_button.h)
  - [`bsp_button.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Src/bsp_button.c)
  - [`ui_task.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Src/ui_task.c)
  - [`SYSTEM_DIAGRAMS.md`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/SYSTEM_DIAGRAMS.md)
  - [`SYSTEM_DIAGRAMS.html`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/SYSTEM_DIAGRAMS.html)
  - [`LEARNING_NOTES.md`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/LEARNING_NOTES.md)
  - [`HARDWARE_BUGS.md`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/HARDWARE_BUGS.md)
- **What changed:**
  - Renamed `BTN_EVENT_SHORT_PRESS` to `BTN_EVENT_NAVIGATE`.
  - Renamed `BTN_EVENT_LONG_PRESS` to `BTN_EVENT_CONFIRM`.
  - Updated all C source files, header files, and markdown/HTML documentation to use the new names.
- **Why it changed:**
  - When buttons were decoupled (TD-12), the long-press duration calculation on PA0 was removed. PD4 became a dedicated Start/Confirm button.
  - Keeping "SHORT_PRESS" and "LONG_PRESS" in code was misleading, as PD4 detects simple falling edges immediately (not long presses), and PA0 only navigates.
- **What I should learn from it:**
  - Code names should strictly reflect physical behavior and domain model realities to prevent cognitive load and technical debt.
  - Maintain documentation consistency across all formats (Markdown, HTML, learning logs) when performing codebase-wide refactoring.

### Change Record — Code Review Fixes + Move Start/Confirm Button PE1 → PD4 (2026-08-22)

- **Goal:** Fix bugs found during a full source review, remove dead code, and
  resolve a real hardware conflict discovered while wiring the Start/Confirm
  button on actual hardware.
- **Files changed:**
  - [`stm32f4xx_it.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Core/Src/stm32f4xx_it.c)
  - [`stm32f4xx_it.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Core/Inc/stm32f4xx_it.h)
  - [`main.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Core/Src/main.c)
  - [`Freertos.ioc`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Freertos.ioc)
  - [`bsp_button.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc/bsp_button.h)
  - [`bsp_button.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Src/bsp_button.c)
  - [`bsp_sensor.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Src/bsp_sensor.c)
  - [`washing_manager.c`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Src/washing_manager.c)
  - [`washing_sm.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc/washing_sm.h)
  - [`app_msg.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc/app_msg.h)
  - [`app_types.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc/app_types.h)
  - `docs/GPIO_CONFIG.md`, `docs/HARDWARE_BUGS.md`, `docs/SYSTEM_DIAGRAMS.md`,
    `docs/SYSTEM_DIAGRAMS.html`, `PROJECT_SPEC.md`
- **What changed:**
  - Removed the dead `case CMD_RESET:` branch in `Manager_HandleCommand()` —
    no producer ever builds `CMD_RESET`; the FINISH/ERROR check above it
    already handles the reset path (Bug #10 fix). Updated doc comments in
    `washing_sm.h`, `app_msg.h`, `app_types.h` to match.
  - Fixed 5 factual errors in `SYSTEM_DIAGRAMS.md`/`.html` found during a
    flow-vs-code review: `xEventGroupSetBitsFromISR` → `xEventGroupSetBits`
    for the Timer Service Task callback (not real ISR context), a stale node
    name, a missing UITask→display-queue edge, an inaccurate error-scenario
    sequence diagram, and a debug tip referencing deleted functions.
  - Diagnosed a real hardware bug on actual board bring-up: PE1 (originally
    chosen for Start/Confirm) is tied on-board to the LIS3DSH accelerometer's
    `MEMS_INT2` output — confirmed via schematic and via logic-analyzer
    measurement staying LOW regardless of internal pull direction. Moved the
    Start/Confirm button to **PD4** across `.ioc`, `main.c`, `stm32f4xx_it.c`/`.h`,
    BSP comments, and all docs (Bug #14).
  - While regenerating code for the PD4 change, CubeMX reset PA0's edge
    trigger and both EXTI0/EXTI4 NVIC priorities back to defaults, and the
    new `EXTI4_IRQHandler()` was missing the `BSP_StartButton_IRQHandler()`
    call — a regression of Bug #12/#13/#11 that had already been fixed once
    before. Re-applied all three fixes and documented the regression as
    Bug #15.
- **Why it changed:**
  - Dead code and stale comments left the state-machine documentation
    inconsistent with actual runtime behavior — a review turned these up
    before they caused confusion during debugging.
  - PE1 is not a free GPIO on this board; wiring a button there can never
    work correctly no matter how the firmware is configured, because the
    on-board accelerometer's interrupt output physically fights the pin.
  - `.ioc` had silently drifted from the previously-fixed state between
    sessions; any subsequent "Generate Code" reverts main.c to whatever the
    (stale) `.ioc` says, regardless of which pin was actually being edited.
- **What I should learn from it:**
  - "Generate Code" rewrites the *entire* `MX_GPIO_Init()`, not just the pin
    you touched in the GUI — always re-verify unrelated pins after every
    regenerate, not just the one you meant to change.
  - Renaming an EXTI line (changing pins) renames its `USER CODE` block too;
    CubeMX does not carry old block content into the new block name — any
    manual code inside must be re-added by hand every time.
  - When a board is a "Discovery kit", assume nothing on the header is a
    free GPIO until checked against the schematic — evaluation boards
    routinely multiplex pins with on-board sensors/audio/USB.
  - A logic-analyzer reading that stays constant regardless of internal
    pull-up/pull-down direction is a strong, fast diagnostic for "something
    external is driving this pin" — worth checking before assuming a wiring
    fault.

### Change Record — Move Start/Confirm Button PD4 → PD2 (2026-08-22, same session)

- **Goal:** PD4 also measured stuck LOW on real hardware; move the
  Start/Confirm button to a third pin at the user's request, without running
  the isolation test that was recommended first.
- **Files changed:** same set as the PE1 → PD4 change above
  (`Freertos.ioc`, `main.c`, `stm32f4xx_it.c`/`.h`, `bsp_button.h`/`.c`,
  `app_types.h`, `app_msg.h`, `docs/GPIO_CONFIG.md`, `docs/HARDWARE_BUGS.md`,
  `docs/SYSTEM_DIAGRAMS.md`/`.html`, `PROJECT_SPEC.md`, `CLAUDE.md`).
- **What changed:**
  - Renamed PD4 → PD2 across the same chain (EXTI4 → EXTI2,
    `GPIO_PIN_4` → `GPIO_PIN_2`) everywhere the previous change touched.
  - Documented the PD4 failure as **Bug #16** with root cause explicitly
    marked *unconfirmed* — the PD4-is-safe reasoning in Bug #14 was based on
    the STM32F4-Discovery (407) schematic, not the actual 411E (MB1115) one;
    automated schematic fetches for the correct board timed out twice.
  - While generating code for PD2 in the CubeMX GUI, the exact same
    regression as Bug #15 happened a third time (PA0 lost its FALLING edge
    and both EXTI0/EXTI2 dropped to NVIC priority 0) — fixed again the same
    way, re-synced `.ioc` and `main.c`.
- **Why it changed:** user's explicit choice to prioritize moving forward
  over diagnosing PD4, given time constraints.
- **What I should learn from it:**
  - Two different pins reading the exact same "stuck LOW" symptom is a
    stronger signal that something *constant* across both attempts (wiring,
    the button itself, or the measurement setup) is the real cause, not
    two independent pin conflicts — flagged this to the user but respected
    their decision to move on without confirming.
  - A conclusion inferred from a similar-but-different board revision must
    be labeled as an unconfirmed hypothesis, not stated as fact — this
    would have prevented presenting the PD4 recommendation with more
    confidence than the evidence supported.
  - The Bug #15-style regression is now a proven repeat pattern (3 times in
    a row) tied specifically to using the CubeMX GUI to change a pin —
    every future pin change on this project should budget for re-checking
    PA0 and NVIC priorities immediately after, not as an afterthought.

### Change Record — Fix 3 Stale Doc Comments in app_events.h (2026-08-23)

- **Goal:** During a mentoring session on Event Groups, three doc comments in
  `app_events.h` were found to describe behavior the code does not have.
  Fixed them so the header stops teaching the wrong thing.
- **Files changed:**
  - [`app_events.h`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc/app_events.h)
  - [`QA_LOG.md`](file:///c:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/docs/QA_LOG.md) (Session 09 added)
- **What changed:**
  - `EVT_TIMEOUT` comment claimed the timer callback sets the bit *"from ISR
    context via `xEventGroupSetBitsFromISR()`"*. Corrected: `Timer_PhaseCallback()`
    runs in the Timer Service Task (a real task), and uses the plain
    `xEventGroupSetBits()`. Added a note that the bit is shared by all four
    phase timers (TD-10) so its meaning is state-dependent.
  - The file's "Usage example" demonstrated a **blocking** guard
    (`WaitBits(EVT_DOOR_CLOSED, portMAX_DELAY)`) — an anti-pattern, since a
    blocked WashingManager cannot tell the user why it refused to start.
    Replaced with an example contrasting the two correct uses: `WaitBits`
    for awaiting a future event, `GetBits` for a non-blocking precondition
    check. The old example also named a handle that does not exist
    (`hSystemEvents` vs the real `xSystemEvents`).
  - `EVT_ERROR` comment said it *"clears only on CMD_RESET"* — wrong twice
    over: the `CMD_RESET` case was deleted earlier as dead code, and the bit
    is actually cleared both by `Manager_OnEnterIdle()` and automatically by
    `WaitBits(..., xClearOnExit = pdTRUE)`.
- **Why it changed:** these comments are the first thing a reader consults to
  understand the event group; leaving them stale actively teaches incorrect
  FreeRTOS semantics (particularly the ISR-vs-task distinction, which is a
  core learning goal of this project).
- **What I should learn from it:**
  - Comments are not compiler-checked, so they drift silently. This project
    has now had four separate cases of a comment describing behavior the code
    never had (Bug #12, plus these three) — always verify a comment against
    the code before trusting it.
  - `xEventGroupSetBitsFromISR()` is a *deferred* operation routed through
    the timer daemon task and can return `pdFAIL`; it is not a drop-in
    equivalent of `xQueueSendFromISR()`.
  - Guard conditions must be checked non-blocking (`GetBits`) at the single
    point every path must pass through — not inside one particular state.
    The current design still lacks such a guard: the `IDLE → SPIN` path
    performs no door check at all (documented, not yet fixed).
