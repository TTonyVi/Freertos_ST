/**
 * @file    bsp_button.h
 * @brief   Button BSP — two physical buttons with separate responsibilities.
 *
 * Button 1 — Menu Navigate (PA0, B1 on-board):
 *   Generates BTN_EVENT_NAVIGATE on the press edge. Single-gesture input:
 *   long-press detection was removed when Confirm moved to its own pin, and
 *   no press duration is measured any more.
 *
 *   Hardware mapping:
 *     Pin   : PA0
 *     EXTI  : EXTI0, both edges (rising = press, falling = release)
 *     Pull  : none (external pull-down on DISCO board — active HIGH)
 *
 * Button 2 — Start / Confirm (PD2, external):
 *   Generates BTN_EVENT_CONFIRM on the press edge (falling). Any press
 *   confirms the current menu selection; no duration measurement.
 *
 *   Hardware mapping:
 *     Pin   : PD2 — third pin tried for this button. Moved off PE1 first
 *             (HARDWARE_BUGS.md Bug #14: PE1 is tied on-board to the
 *             LIS3DSH accelerometer's MEMS_INT2 output), then off PD4
 *             (Bug #16: also measured stuck LOW on this board; root cause
 *             not confirmed — moved on without full diagnosis at the
 *             user's request). If PD2 also misbehaves, do NOT guess a
 *             fourth pin — get the actual MB1115 schematic confirmed first.
 *     EXTI  : EXTI2, falling edge only
 *     Pull  : internal pull-up (active LOW — button connects PD2 to GND)
 *
 * Both ISRs route to the same xButtonQueue via the same registered callback,
 * so UITask requires no changes: NAVIGATE = navigate, CONFIRM = start.
 *
 * Debounce:
 *   Both handlers use the same quiet-window filter — an edge counts as a
 *   gesture only after BUTTON_DEBOUNCE_MS of silence, and every edge reloads
 *   that window. Each button owns its own timestamp. See bsp_button.c for the
 *   rationale and for the known limit on PD2 (falling-edge-only EXTI).
 *
 * Priority note:
 *   Both EXTI0 and EXTI2 are set to preempt priority 5 so that
 *   xQueueSendFromISR() is safe to call from the callback.
 */

#ifndef APP_BSP_BSP_BUTTON_H
#define APP_BSP_BSP_BUTTON_H

#include "app_types.h" /* ButtonEvent_t */

/**
 * @brief  Callback type invoked from ISR context when a gesture is detected.
 * @param  event  BTN_EVENT_NAVIGATE or BTN_EVENT_CONFIRM.
 */
typedef void (*BSP_Button_EventCb_t)(ButtonEvent_t event);

/**
 * @brief  Register the event callback used by both button ISRs.
 *
 * All GPIO and NVIC configuration is done by CubeMX (MX_GPIO_Init).
 * This is the only BSP init needed — call from UITask before its loop.
 *
 * @param  callback  Called from ISR on BTN_EVENT_NAVIGATE (PA0) or
 *                   BTN_EVENT_CONFIRM (PD2). May be NULL to disable.
 */
void BSP_Button_RegisterCallback(BSP_Button_EventCb_t callback);

/** @brief Call from EXTI0_IRQHandler. Debounces PA0 → BTN_EVENT_NAVIGATE. */
void BSP_Button_IRQHandler(void);

/** @brief Call from EXTI2_IRQHandler. Debounces PD2 → BTN_EVENT_CONFIRM. */
void BSP_StartButton_IRQHandler(void);

#endif /* APP_BSP_BSP_BUTTON_H */
