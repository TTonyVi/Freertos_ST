/**
 * @file    sensor_task.c
 * @brief   SensorTask implementation — periodic sensor polling.
 */

#include "sensor_task.h"
#include "app_config.h"
#include "app_events.h"
#include "bsp_sensor.h"
#include "freertos_objects.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"

void SensorTask(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    /* Poll physical simulated sensor buttons (PD5 and PD6) to detect edge changes */
    BSP_Sensor_Poll();

    if (BSP_Sensor_IsDoorClosed()) {
      xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
    } else {
      xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
      xEventGroupSetBits(xSystemEvents, EVT_ERROR);
    }

    /* Level-style: mirror the sensor, so a stale FULL bit cannot survive into
     * the next cycle's FILL_WATER. */
    if (BSP_Sensor_IsWaterFull()) {
      xEventGroupSetBits(xSystemEvents, EVT_WATER_FULL);
    } else {
      xEventGroupClearBits(xSystemEvents, EVT_WATER_FULL);
    }

    vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
  }
}
