/**
 * @file    ui_task.c
 * @brief   UITask implementation — button input handler and menu navigator.
 */

#include "ui_task.h"
#include "bsp_button.h"
#include "freertos_objects.h"
#include "app_types.h"
#include "app_msg.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>

/* --------------------------------------------------------------------------
 * Menu configuration
 * -------------------------------------------------------------------------- */

#define MENU_ITEM_COUNT  2u   /* 0 = Wash, 1 = Spin */

/* --------------------------------------------------------------------------
 * ISR callback — runs in EXTI interrupt context
 *
 * Called by BSP_Button_IRQHandler() after gesture classification.
 * Must use xQueueSendFromISR() and portYIELD_FROM_ISR() — never a blocking
 * FreeRTOS call.
 * -------------------------------------------------------------------------- */

static void Button_EventCallback(ButtonEvent_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* --------------------------------------------------------------------------
 * UITask
 * -------------------------------------------------------------------------- */

/* Cached status text from last WashingManager display update.
 * UITask piggybacks this when it sends its own cursor-update messages. */
static char s_statusCache[24] = "IDLE";

void UITask(void *pvParameters)
{
    (void)pvParameters;

    BSP_Button_Init(Button_EventCallback);

    uint8_t       menuIndex = 0u;
    ButtonEvent_t event;

    while (1)
    {
        xQueueReceive(xButtonQueue, &event, portMAX_DELAY);

        if (event == BTN_EVENT_SHORT_PRESS)
        {
            menuIndex = (uint8_t)((menuIndex + 1u) % MENU_ITEM_COUNT);

            /* Send display update so user sees cursor move immediately. */
            DisplayMsg_t disp;
            disp.menuIndex = menuIndex;
            (void)strncpy(disp.statusText, s_statusCache, sizeof(disp.statusText) - 1u);
            disp.statusText[sizeof(disp.statusText) - 1u] = '\0';
            xQueueSend(xDisplayQueue, &disp, 0);
        }
        else if (event == BTN_EVENT_LONG_PRESS)
        {
            CommandMsg_t cmd;
            cmd.menuIndex = menuIndex;

            if (menuIndex == 0u)
            {
                cmd.command = CMD_START_WASH;
                cmd.mode    = WASH_MODE_WASH;
            }
            else
            {
                cmd.command = CMD_START_SPIN;
                cmd.mode    = WASH_MODE_SPIN;
            }

            xQueueSend(xCommandQueue, &cmd, 0);
        }
    }
}
