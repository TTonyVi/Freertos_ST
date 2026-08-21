/**
 * @file    bsp_button.h
 * @brief   Button BSP — two physical buttons with separate responsibilities.
 *
 * Button 1 — Menu Navigate (PA0, B1 on-board):
 *   Detects short press only. Generates BTN_EVENT_SHORT_PRESS on release.
 *   Long-press detection has been removed; PA0 is now a single-gesture input.
 *
 *   Hardware mapping:
 *     Pin   : PA0
 *     EXTI  : EXTI0, both edges (rising = press, falling = release)
 *     Pull  : none (external pull-down on DISCO board — active HIGH)
 *
 * Button 2 — Start / Confirm (PE1, external button):
 *   Generates BTN_EVENT_LONG_PRESS on falling edge (button press).
 *   No duration measurement needed — any press confirms the selection.
 *
 *   Hardware mapping:
 *     Pin   : PE1
 *     EXTI  : EXTI1, falling edge only
 *     Pull  : internal pull-up (active LOW — button connects PE1 to GND)
 *
 * Both ISRs route to the same xButtonQueue via the same registered callback,
 * so UITask requires no changes: SHORT_PRESS = navigate, LONG_PRESS = start.
 *
 * Priority note:
 *   Both EXTI0 and EXTI1 are set to preempt priority 5 so that
 *   xQueueSendFromISR() is safe to call from the callback.
 */

#ifndef APP_BSP_BSP_BUTTON_H
#define APP_BSP_BSP_BUTTON_H

#include "app_types.h"  /* ButtonEvent_t */

/**
 * @brief  Callback type invoked from ISR context when a gesture is detected.
 * @param  event  BTN_EVENT_SHORT_PRESS or BTN_EVENT_LONG_PRESS.
 */
typedef void (*BSP_Button_EventCb_t)(ButtonEvent_t event);

/**
 * @brief  Configure PA0 for short-press detection and register the callback.
 *
 * Reconfigures PA0 from rising-only (CubeMX default) to rising + falling
 * so that release can be detected. Only SHORT_PRESS is generated.
 * Sets EXTI0 preempt priority to 5 (FreeRTOS-safe).
 *
 * @param  callback  Function called on short press. May be NULL.
 */
void BSP_Button_Init(BSP_Button_EventCb_t callback);

/**
 * @brief  Process one EXTI0 edge event (PA0 — Menu Navigate).
 *
 * Must be called at the start of EXTI0_IRQHandler().
 *
 * Behaviour:
 *   Rising edge  (PA0 HIGH) → records press start timestamp.
 *   Falling edge (PA0 LOW)  → if duration >= BUTTON_DEBOUNCE_MS
 *                              → callback(BTN_EVENT_SHORT_PRESS)
 */
void BSP_Button_IRQHandler(void);

/**
 * @brief  Configure PE1 for Start/Confirm press detection.
 *
 * Configures PE1 as falling-edge EXTI with internal pull-up.
 * Reuses the same callback registered in BSP_Button_Init().
 * Sets EXTI1 preempt priority to 5 (FreeRTOS-safe).
 *
 * Call after BSP_Button_Init() so the callback is already registered.
 */
void BSP_StartButton_Init(void);

/**
 * @brief  Process one EXTI1 edge event (PE1 — Start/Confirm).
 *
 * Must be called at the start of EXTI1_IRQHandler().
 * Fires callback(BTN_EVENT_LONG_PRESS) on every falling edge.
 */
void BSP_StartButton_IRQHandler(void);

#endif /* APP_BSP_BSP_BUTTON_H */
