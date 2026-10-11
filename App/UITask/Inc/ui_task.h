/**
 * @file    ui_task.h
 * @brief   UITask — translates button gestures into commands.
 *
 * Responsibilities:
 *   - Initialises BSP button and registers the ISR callback.
 *   - Reads ButtonEvent_t from xButtonQueue (posted by the ISR callback).
 *   - Renames each gesture into a Command_t and posts it to xCommandQueue
 *     for WashingManager:
 *
 *       BTN_EVENT_NAVIGATE (PA0)  →  CMD_NAVIGATE
 *       BTN_EVENT_CONFIRM  (PD2)  →  CMD_CONFIRM
 *
 * What it deliberately does NOT do:
 *   - Keep the menu cursor. WashingManager owns it (single owner).
 *   - Send anything to the OLED. WashingManager is the only producer of
 *     DisplayMsg_t, so every screen update is complete and consistent.
 *
 * Priority: TASK_PRIORITY_UI (2) — below WashingManager, above Idle.
 */

#ifndef APP_UITASK_UI_TASK_H
#define APP_UITASK_UI_TASK_H

/**
 * @brief  UITask FreeRTOS task function.
 *
 * Initialises BSP button then blocks on xButtonQueue; each ButtonEvent_t
 * received is forwarded as one Command_t.
 *
 * @param  pvParameters  Unused. Pass NULL to xTaskCreate().
 */
void UITask(void *pvParameters);

#endif /* APP_UITASK_UI_TASK_H */
