/**
 * @file    ui_task.h
 * @brief   UITask — button input handler and menu navigator.
 *
 * Responsibilities:
 *   - Initialises BSP button and registers the ISR callback.
 *   - Reads ButtonEvent_t from xButtonQueue (posted by the ISR callback).
 *   - Manages the menu cursor (menuIndex: 0 = Wash, 1 = Spin).
 *   - On long press, translates (menuIndex) into a CommandMsg_t and posts
 *     it to xCommandQueue for WashingManager.
 *
 * Menu logic:
 *   Short press  →  advance menuIndex (cycles 0 → 1 → 0)
 *   Long press   →  send CommandMsg_t to xCommandQueue, do not change index
 *
 * Priority: TASK_PRIORITY_UI (2) — below WashingManager, above Idle.
 */

#ifndef APP_UITASK_UI_TASK_H
#define APP_UITASK_UI_TASK_H

/**
 * @brief  UITask FreeRTOS task function.
 *
 * Initialises BSP button then blocks on xButtonQueue.
 * Each received ButtonEvent_t is processed against the current menuIndex.
 *
 * @param  pvParameters  Unused. Pass NULL to xTaskCreate().
 */
void UITask(void *pvParameters);

#endif /* APP_UITASK_UI_TASK_H */
