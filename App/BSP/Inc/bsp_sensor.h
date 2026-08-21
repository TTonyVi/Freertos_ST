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
 * Hardware mapping:
 *   Door sensor  : PD5 — external button, active-low (pull-up enabled).
 *                  Each button press toggles s_doorClosed between 1 (closed)
 *                  and 0 (open).
 *   Water sensor : PD6 — external button, active-low (pull-up enabled).
 *                  Each button press toggles s_waterFull between 0 (empty)
 *                  and 1 (full).
 *
 * Why polling instead of EXTI?
 *   PD5 uses EXTI5, PD6 uses EXTI6 — no conflict with PA0 (EXTI0).
 *   Polling inside SensorTask (100 ms) is preferred for simplicity.
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
