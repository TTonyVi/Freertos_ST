/**
 * @file    bsp_sensor.h
 * @brief   Sensor BSP — simulated door and water-level inputs.
 *
 * In the real system these would be read from GPIO pins wired to physical
 * sensors.  In this simulation the values are controlled by toggling
 * GPIO output pins with a logic analyser or by writing to the state
 * variables directly via the debugger.
 *
 * No FreeRTOS dependency — safe to call from any context.
 *
 * Hardware mapping (simulation):
 *   Door sensor    : PA0 user button — pressed = door closed
 *   Water sensor   : not wired; always returns a fixed simulated value
 */

#ifndef APP_BSP_BSP_SENSOR_H
#define APP_BSP_BSP_SENSOR_H

#include <stdint.h>

/**
 * @brief  Read the simulated door sensor.
 * @return 1 if door is closed, 0 if door is open.
 */
uint8_t BSP_Sensor_IsDoorClosed(void);

/**
 * @brief  Read the simulated water-level sensor.
 * @return 1 if water has reached the target level, 0 otherwise.
 */
uint8_t BSP_Sensor_IsWaterFull(void);

#endif /* APP_BSP_BSP_SENSOR_H */
