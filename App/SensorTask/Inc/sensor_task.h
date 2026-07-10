/**
 * @file    sensor_task.h
 * @brief   SensorTask — periodic sensor polling and event group updates.
 *
 * Responsibilities:
 *   - Polls BSP sensor layer every SENSOR_TASK_PERIOD_MS (100 ms).
 *   - Sets EVT_DOOR_CLOSED in xSystemEvents when door is closed.
 *   - Clears EVT_DOOR_CLOSED when door opens mid-cycle (sets EVT_ERROR).
 *   - Sets EVT_WATER_FULL when water sensor reports full.
 *
 * Priority: TASK_PRIORITY_SENSOR (2) — same as UITask, below WashingManager.
 */

#ifndef APP_SENSORTASK_SENSOR_TASK_H
#define APP_SENSORTASK_SENSOR_TASK_H

/**
 * @brief  SensorTask FreeRTOS task function.
 * @param  pvParameters  Unused. Pass NULL to xTaskCreate().
 */
void SensorTask(void *pvParameters);

#endif /* APP_SENSORTASK_SENSOR_TASK_H */
