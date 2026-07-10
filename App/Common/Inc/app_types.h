/**
 * @file    app_types.h
 * @brief   Application-wide enumeration types.
 *
 * Defines all domain enums shared across tasks.
 * No FreeRTOS dependency — safe to include from BSP and driver layers.
 */

#ifndef APP_COMMON_APP_TYPES_H
#define APP_COMMON_APP_TYPES_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * WashState_t
 *
 * Represents every state in the washing machine state machine.
 * WashingManager is the sole owner of the current state variable.
 *
 * State transitions:
 *   IDLE        → FILL_WATER  (Start Wash)
 *   IDLE        → SPIN        (Start Spin)
 *   FILL_WATER  → WASHING     (EVT_WATER_FULL)
 *   FILL_WATER  → ERROR       (EVT_TIMEOUT — fill timeout)
 *   WASHING     → DRAIN       (EVT_TIMEOUT — wash complete)
 *   DRAIN       → SPIN        (EVT_TIMEOUT — drain complete)
 *   SPIN        → FINISH      (EVT_TIMEOUT — spin complete)
 *   FINISH      → IDLE        (user acknowledge)
 *   Any         → ERROR       (door opened while running)
 *   ERROR       → IDLE        (CMD_RESET)
 * -------------------------------------------------------------------------- */
typedef enum
{
    WASH_STATE_IDLE        = 0, /**< Machine is stopped, awaiting user input. */
    WASH_STATE_FILL_WATER,      /**< Inlet valve open; waiting for water level. */
    WASH_STATE_WASHING,         /**< Motor running; agitation cycle active. */
    WASH_STATE_DRAIN,           /**< Drain valve open; removing water. */
    WASH_STATE_SPIN,            /**< Motor running at high speed; spin cycle. */
    WASH_STATE_FINISH,          /**< Cycle complete; awaiting user acknowledge. */
    WASH_STATE_ERROR,           /**< Fault detected; machine halted. */
} WashState_t;

/* --------------------------------------------------------------------------
 * WashMode_t
 *
 * The program selected by the user before starting.
 * Determines which state machine path WashingManager follows.
 *
 *   WASH_MODE_WASH  → IDLE → FILL_WATER → WASHING → DRAIN → SPIN → FINISH
 *   WASH_MODE_SPIN  → IDLE → SPIN → FINISH
 * -------------------------------------------------------------------------- */
typedef enum
{
    WASH_MODE_WASH = 0, /**< Full wash cycle: fill, wash, drain. */
    WASH_MODE_SPIN,     /**< Spin-only cycle: no water. */
} WashMode_t;

/* --------------------------------------------------------------------------
 * ButtonEvent_t
 *
 * Raw gesture produced by the BSP button layer after debouncing.
 * The ISR captures the GPIO edge; the BSP measures duration and posts
 * one of these values to the Button Queue.
 *
 * UITask interprets a ButtonEvent_t in the context of the current
 * menu state to generate a Command_t.  The ISR never generates
 * commands directly.
 * -------------------------------------------------------------------------- */
typedef enum
{
    BTN_EVENT_SHORT_PRESS = 0, /**< Press < BUTTON_LONG_PRESS_MS — move selection. */
    BTN_EVENT_LONG_PRESS,      /**< Press ≥ BUTTON_LONG_PRESS_MS — confirm/start. */
} ButtonEvent_t;

/* --------------------------------------------------------------------------
 * Command_t
 *
 * High-level intent sent from UITask to WashingManager via Command Queue.
 * UITask maps (ButtonEvent_t + menu state) → Command_t.
 *
 *   Short press on menu   →  UITask moves cursor (no command sent)
 *   Long press on "Wash"  →  CMD_START_WASH
 *   Long press on "Spin"  →  CMD_START_SPIN
 *   Any state + error     →  CMD_RESET
 * -------------------------------------------------------------------------- */
typedef enum
{
    CMD_START_WASH = 0, /**< Start the full wash cycle. */
    CMD_START_SPIN,     /**< Start the spin-only cycle. */
    CMD_RESET,          /**< Clear ERROR state; return to IDLE. */
} Command_t;

#endif /* APP_COMMON_APP_TYPES_H */
