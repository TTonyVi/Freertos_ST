/**
 * @file    ui_task.c
 * @brief   UITask implementation — translates button gestures into commands.
 *
 * UITask holds no state. It does not know where the menu cursor is or what
 * the machine is doing; WashingManager owns both. That is deliberate: when
 * UITask kept its own copy of the cursor and also drew to the OLED, the two
 * copies drifted apart and the status line showed stale text.
 */

#include "ui_task.h"
#include "app_msg.h"
#include "app_types.h"
#include "bsp_button.h"
#include "freertos_objects.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* --------------------------------------------------------------------------
 * ISR callback — runs in EXTI interrupt context
 *
 * Called by BSP_Button_IRQHandler() after gesture classification.
 * Must use xQueueSendFromISR() and portYIELD_FROM_ISR() — never a blocking
 * FreeRTOS call.
 * -------------------------------------------------------------------------- */

static void Button_EventCallback(ButtonEvent_t event) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* --------------------------------------------------------------------------
 * UITask
 * -------------------------------------------------------------------------- */

void UITask(void *pvParameters) {
  (void)pvParameters;

  BSP_Button_RegisterCallback(Button_EventCallback);

  ButtonEvent_t event;

  while (1) {
    xQueueReceive(xButtonQueue, &event, portMAX_DELAY);

    CommandMsg_t cmd;

    switch (event) {
    case BTN_EVENT_NAVIGATE:
      cmd.command = CMD_NAVIGATE;
      break;

    case BTN_EVENT_CONFIRM:
      cmd.command = CMD_CONFIRM;
      break;

    default:
      continue; /* unknown gesture: nothing to forward */
    }

    /* Timeout 0: never stall on a full queue. If the user mashes the buttons
     * faster than WashingManager reads, the surplus presses are dropped. */
    xQueueSend(xCommandQueue, &cmd, 0);
  }
}
