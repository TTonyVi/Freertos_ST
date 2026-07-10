/**
 * @file    bsp_motor.c
 * @brief   Motor BSP implementation.
 */

#include "bsp_motor.h"
#include "stm32f4xx_hal.h"

#define MOTOR_PORT   GPIOD
#define MOTOR_PIN    GPIO_PIN_12

void BSP_Motor_Init(void)
{
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN, GPIO_PIN_RESET);
}

void BSP_Motor_Start(void)
{
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN, GPIO_PIN_SET);
}

void BSP_Motor_Stop(void)
{
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN, GPIO_PIN_RESET);
}

uint8_t BSP_Motor_IsRunning(void)
{
    return (HAL_GPIO_ReadPin(MOTOR_PORT, MOTOR_PIN) == GPIO_PIN_SET) ? 1u : 0u;
}
