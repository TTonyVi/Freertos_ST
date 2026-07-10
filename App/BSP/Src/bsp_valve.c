/**
 * @file    bsp_valve.c
 * @brief   Valve BSP implementation.
 */

#include "bsp_valve.h"
#include "stm32f4xx_hal.h"

#define VALVE_PORT        GPIOD
#define VALVE_PIN_INLET   GPIO_PIN_13
#define VALVE_PIN_DRAIN   GPIO_PIN_14

/* Maps BspValve_t enum to GPIO pin mask. */
static const uint16_t kValvePin[] = {
    [BSP_VALVE_INLET] = VALVE_PIN_INLET,
    [BSP_VALVE_DRAIN] = VALVE_PIN_DRAIN,
};

void BSP_Valve_Init(void)
{
    HAL_GPIO_WritePin(VALVE_PORT, VALVE_PIN_INLET | VALVE_PIN_DRAIN, GPIO_PIN_RESET);
}

void BSP_Valve_Open(BspValve_t valve)
{
    HAL_GPIO_WritePin(VALVE_PORT, kValvePin[valve], GPIO_PIN_SET);
}

void BSP_Valve_Close(BspValve_t valve)
{
    HAL_GPIO_WritePin(VALVE_PORT, kValvePin[valve], GPIO_PIN_RESET);
}

uint8_t BSP_Valve_IsOpen(BspValve_t valve)
{
    return (HAL_GPIO_ReadPin(VALVE_PORT, kValvePin[valve]) == GPIO_PIN_SET) ? 1u : 0u;
}
