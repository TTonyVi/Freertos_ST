/**
 * @file    bsp_sensor.c
 * @brief   Sensor BSP implementation — simulated door and water-level inputs.
 *
 * Simulation strategy:
 *   Door   : read PA0 (user button). Button held = door closed.
 *   Water  : software flag toggled by the debugger or hardcoded for testing.
 *            Set s_waterFull = 1 in the debugger to simulate a full tank.
 */

#include "bsp_sensor.h"
#include "stm32f4xx_hal.h"

#define DOOR_PORT   GPIOA
#define DOOR_PIN    GPIO_PIN_0

static volatile uint8_t s_waterFull = 0u;

uint8_t BSP_Sensor_IsDoorClosed(void)
{
    return (HAL_GPIO_ReadPin(DOOR_PORT, DOOR_PIN) == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t BSP_Sensor_IsWaterFull(void)
{
    return s_waterFull;
}
