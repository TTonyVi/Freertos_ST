/**
 * @file    oled_task.c
 * @brief   OLEDTask implementation — OLED display renderer.
 */

#include "oled_task.h"
#include "bsp_oled.h"
#include "freertos_objects.h"
#include "app_msg.h"
#include "app_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

static void OLED_Render(const DisplayMsg_t *msg)
{
    char line[22];

    BSP_OLED_Clear();

    /* Row 0 — static title */
    BSP_OLED_DrawText(10, 2, "Washing Machine");

    /* Row 1 — Wash menu item with cursor */
    BSP_OLED_DrawText(10, 3, (msg->menuIndex == 0u) ? "> Wash" : "  Wash");

    /* Row 2 — Spin menu item with cursor */
    BSP_OLED_DrawText(10, 4, (msg->menuIndex == 1u) ? "> Spin" : "  Spin");

    /* Row 3 — status text from WashingManager */
    BSP_OLED_DrawText(10, 5, msg->statusText);

    BSP_OLED_Refresh();
}

/* --------------------------------------------------------------------------
 * OLEDTask
 * -------------------------------------------------------------------------- */

void OLEDTask(void *pvParameters)
{
    (void)pvParameters;

    BSP_OLED_Init();

    DisplayMsg_t msg;

    while (1)
    {
        xQueueReceive(xDisplayQueue, &msg, portMAX_DELAY);
        OLED_Render(&msg);
    }
}
