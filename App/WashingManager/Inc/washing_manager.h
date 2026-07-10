/**
 * @file    washing_manager.h
 * @brief   WashingManager task — state machine owner and system coordinator.
 *
 * Responsibilities:
 *   - Owns the washing machine state machine (washing_sm).
 *   - Receives CommandMsg_t from xCommandQueue (producer: UITask).
 *   - Waits on xSystemEvents for EVT_WATER_FULL and EVT_TIMEOUT (set by SensorTask
 *     and software timer callbacks respectively).
 *   - Translates inbound signals into WashEvent_t and drives WashingSM_ProcessEvent().
 *   - Controls BSP actuators (motor, valves, LED) via SM entry-action callbacks.
 *   - Sends DisplayMsg_t to xDisplayQueue on every state transition.
 *
 * Priority: TASK_PRIORITY_WASH_MANAGER (3) — highest application priority.
 *
 * This module does not access the OLED driver, button hardware, or sensors
 * directly.  Those concerns belong to OLEDTask, BSP button, and SensorTask.
 */

#ifndef APP_WASHINGMANAGER_WASHING_MANAGER_H
#define APP_WASHINGMANAGER_WASHING_MANAGER_H

/**
 * @brief  WashingManager FreeRTOS task function.
 *
 * Initialises the state machine and BSP actuator layers, then enters the
 * state-dependent blocking loop.  Each state blocks on exactly the signal
 * it is waiting for — no polling, no missed events.
 *
 * Blocking strategy per state:
 *   IDLE / FINISH / ERROR  →  xQueueReceive(xCommandQueue, portMAX_DELAY)
 *   FILL_WATER             →  xEventGroupWaitBits(EVT_WATER_FULL | EVT_TIMEOUT)
 *   WASHING / DRAIN / SPIN →  vTaskDelay (Phase 7: replaced by timer event wait)
 *
 * @param  pvParameters  Unused. Pass NULL to xTaskCreate().
 */
void WashingManager_Task(void *pvParameters);

#endif /* APP_WASHINGMANAGER_WASHING_MANAGER_H */
