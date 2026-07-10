/**
 * @file    bsp_button.h
 * @brief   Button BSP — gesture detection on PA0 (B1 User Button).
 *
 * Owns GPIO pin PA0 and EXTI0. Detects short and long press gestures
 * by measuring press duration across rising and falling EXTI edges.
 * Reports gestures via a registered callback — no FreeRTOS dependency.
 *
 * Hardware mapping:
 *   Pin   : PA0
 *   EXTI  : EXTI0, both edges (rising = press, falling = release)
 *   Pull  : none (external pull-down on DISCO board)
 *
 * Wiring into the ISR (stm32f4xx_it.c, USER CODE BEGIN EXTI0_IRQn 0):
 *
 *   void EXTI0_IRQHandler(void) {
 *       BSP_Button_IRQHandler();          // gesture detection
 *       HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);  // clears EXTI flag
 *   }
 *
 * Callback is invoked from ISR context — the implementor must use
 * FromISR FreeRTOS variants if posting to a queue.
 *
 * Priority note:
 *   BSP_Button_Init() sets EXTI0 preempt priority to 5 so that
 *   xQueueSendFromISR() is safe to call from the callback.
 *   FreeRTOS requires ISRs that call FromISR APIs to run at or below
 *   configMAX_SYSCALL_INTERRUPT_PRIORITY (typically mapped to priority 5).
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
 * @brief  Configure PA0 for both-edge EXTI and register the event callback.
 *
 * Reconfigures PA0 from rising-only (CubeMX default) to rising + falling
 * so that press duration can be measured.
 * Sets EXTI0 preempt priority to 5 (FreeRTOS-safe).
 *
 * @param  callback  Function called on each detected gesture. May be NULL
 *                   (disables event reporting — useful during early bring-up).
 */
void BSP_Button_Init(BSP_Button_EventCb_t callback);

/**
 * @brief  Process one EXTI edge event.
 *
 * Must be called at the start of EXTI0_IRQHandler(), before
 * HAL_GPIO_EXTI_IRQHandler() clears the pending bit.
 *
 * Behaviour:
 *   Rising edge  (PA0 HIGH) → records press start timestamp.
 *   Falling edge (PA0 LOW)  → computes duration:
 *     duration < BUTTON_DEBOUNCE_MS            → ignored (noise)
 *     duration < BUTTON_LONG_PRESS_MS          → callback(BTN_EVENT_SHORT_PRESS)
 *     duration >= BUTTON_LONG_PRESS_MS         → callback(BTN_EVENT_LONG_PRESS)
 */
void BSP_Button_IRQHandler(void);

#endif /* APP_BSP_BSP_BUTTON_H */
