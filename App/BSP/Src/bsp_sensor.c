/**
 * @file    bsp_sensor.c
 * @brief   Sensor BSP implementation — simulated door and water-level inputs.
 *
 * Simulation strategy:
 *   Door   : software flag toggled by the debugger. Defaults to closed (1)
 *            so the machine can run unattended. Set s_doorClosed = 0 in the
 *            debugger to simulate the door opening mid-cycle.
 *   Water  : software flag toggled by the debugger or hardcoded for testing.
 *            Set s_waterFull = 1 in the debugger to simulate a full tank.
 *
 * Door was previously read from PA0 (the B1 user button), but that pin is
 * also used by bsp_button.c for menu/start gestures. Releasing the button
 * to confirm a long-press command was indistinguishable from "door opened",
 * so the two responsibilities were split: PA0 stays dedicated to button
 * input, and the door is now a pure software simulation like water.
 */

#include "bsp_sensor.h"

static volatile uint8_t s_doorClosed = 1u;
static volatile uint8_t s_waterFull  = 1u;

uint8_t BSP_Sensor_IsDoorClosed(void)
{
    return s_doorClosed;
}

uint8_t BSP_Sensor_IsWaterFull(void)
{
    return s_waterFull;
}
