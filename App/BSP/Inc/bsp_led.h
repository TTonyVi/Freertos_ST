/**
 * @file    bsp_led.h
 * @brief   Status LED BSP — controls PD15 (LD6 Blue).
 *
 * Owns GPIO pin PD15. No other module writes to this pin.
 * No FreeRTOS dependency. No application logic.
 *
 * Hardware mapping:
 *   Pin  : PD15
 *   LED  : LD6 (Blue)
 *   HIGH : Machine active
 *   LOW  : Machine idle
 *
 * Logic Analyzer: CH3 → PD15
 */

#ifndef APP_BSP_BSP_LED_H
#define APP_BSP_BSP_LED_H

/**
 * @brief  Ensure PD15 is driven LOW.
 *         GPIO already configured as output by MX_GPIO_Init().
 *         Must be called after MX_GPIO_Init().
 */
void BSP_LED_Init(void);

/**
 * @brief  Drive PD15 HIGH — LED on.
 */
void BSP_LED_On(void);

/**
 * @brief  Drive PD15 LOW — LED off.
 */
void BSP_LED_Off(void);

/**
 * @brief  Toggle PD15 output state.
 */
void BSP_LED_Toggle(void);

#endif /* APP_BSP_BSP_LED_H */
