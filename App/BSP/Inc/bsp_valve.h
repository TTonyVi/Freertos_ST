/**
 * @file    bsp_valve.h
 * @brief   Valve BSP — controls PD13 (inlet) and PD14 (drain).
 *
 * Owns GPIO pins PD13 and PD14. No other module writes to these pins.
 * No FreeRTOS dependency. No application logic.
 *
 * Hardware mapping:
 *   BSP_VALVE_INLET → PD13, LD3 (Orange) — HIGH = filling water
 *   BSP_VALVE_DRAIN → PD14, LD5 (Red)    — HIGH = draining
 *
 * Logic Analyzer: CH1 → PD13 (inlet), CH2 → PD14 (drain)
 */

#ifndef APP_BSP_BSP_VALVE_H
#define APP_BSP_BSP_VALVE_H

#include <stdint.h>

/**
 * @brief  Valve selector.
 */
typedef enum
{
    BSP_VALVE_INLET = 0, /**< Water inlet valve — PD13. */
    BSP_VALVE_DRAIN,     /**< Drain valve — PD14. */
} BspValve_t;

/**
 * @brief  Drive PD13 and PD14 LOW (both valves closed).
 *         Must be called after MX_GPIO_Init().
 */
void BSP_Valve_Init(void);

/**
 * @brief  Drive the selected valve pin HIGH — valve open.
 * @param  valve  BSP_VALVE_INLET or BSP_VALVE_DRAIN.
 */
void BSP_Valve_Open(BspValve_t valve);

/**
 * @brief  Drive the selected valve pin LOW — valve closed.
 * @param  valve  BSP_VALVE_INLET or BSP_VALVE_DRAIN.
 */
void BSP_Valve_Close(BspValve_t valve);

/**
 * @brief  Read the current output state of the selected valve pin.
 * @param  valve  BSP_VALVE_INLET or BSP_VALVE_DRAIN.
 * @return 1 if valve is open, 0 if closed.
 */
uint8_t BSP_Valve_IsOpen(BspValve_t valve);

#endif /* APP_BSP_BSP_VALVE_H */
