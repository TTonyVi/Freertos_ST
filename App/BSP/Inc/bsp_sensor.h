/**
 * @file    bsp_sensor.h
 * @brief   Sensor BSP — simulated door and water-level inputs.
 *
 * In the real system these would be read from GPIO pins wired to physical
 * sensors. In this simulation both values are plain static variables that
 * default to a safe/idle reading and are changed directly via the debugger
 * (Live Expressions / Variables view) while the target is running.
 *
 * No FreeRTOS dependency — safe to call from any context.
 *
 * Hardware mapping (simulation):
 *   Door sensor    : not wired; s_doorClosed defaults to 1 (closed)
 *   Water sensor   : not wired; s_waterFull defaults to 0 (empty)
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
