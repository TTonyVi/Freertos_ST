/**
 * @file    bsp_motor.h
 * @brief   Motor BSP — controls PD12 (LD4 Green).
 *
 * Owns GPIO pin PD12. No other module writes to this pin.
 * No FreeRTOS dependency. No application logic.
 *
 * Hardware mapping:
 *   Pin  : PD12
 *   LED  : LD4 (Green)
 *   HIGH : Motor running
 *   LOW  : Motor stopped
 *
 * Logic Analyzer: CH0 → PD12
 */

#ifndef APP_BSP_BSP_MOTOR_H
#define APP_BSP_BSP_MOTOR_H

#include <stdint.h>

/**
 * @brief  Ensure PD12 is driven LOW.
 *         GPIO already configured as output by MX_GPIO_Init().
 *         Must be called after MX_GPIO_Init().
 */
void BSP_Motor_Init(void);

/**
 * @brief  Drive PD12 HIGH — motor running.
 */
void BSP_Motor_Start(void);

/**
 * @brief  Drive PD12 LOW — motor stopped.
 */
void BSP_Motor_Stop(void);

/**
 * @brief  Read the current output state of PD12.
 * @return 1 if motor is running, 0 if stopped.
 */
uint8_t BSP_Motor_IsRunning(void);

#endif /* APP_BSP_BSP_MOTOR_H */
