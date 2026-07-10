/**
 * @file    sensor_task.c
 * @brief   SensorTask implementation — periodic sensor polling.
 */

#include "sensor_task.h"
#include "bsp_sensor.h"
#include "freertos_objects.h"
#include "app_events.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

void SensorTask(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        if (BSP_Sensor_IsDoorClosed())
        {
            xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
        }
        else
        {
            xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
            xEventGroupSetBits(xSystemEvents, EVT_ERROR);
        }

        if (BSP_Sensor_IsWaterFull())
        {
            xEventGroupSetBits(xSystemEvents, EVT_WATER_FULL);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
}
