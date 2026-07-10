/**
 * @file    bsp_led.c
 * @brief   System status LED BSP implementation.
 */

#include "bsp_led.h"
#include "stm32f4xx_hal.h"

#define LED_PORT   GPIOD
#define LED_PIN    GPIO_PIN_15

void BSP_LED_Init(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}
