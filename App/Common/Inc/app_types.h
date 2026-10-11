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
 *   FINISH      → IDLE        (user acknowledge — CMD_CONFIRM)
 *   Any         → ERROR       (door opened while running)
 *   ERROR       → IDLE        (user acknowledge — CMD_CONFIRM)
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
 * The ISR captures the GPIO edge; the BSP applies a quiet-window filter
 * and posts one of these values to the Button Queue on the press edge.
 * No press duration is measured — each pin carries exactly one gesture.
 *
 * UITask interprets a ButtonEvent_t in the context of the current
 * menu state to generate a Command_t.  The ISR never generates
 * commands directly.
 * -------------------------------------------------------------------------- */
typedef enum
{
    BTN_EVENT_NAVIGATE = 0, /**< PA0 pressed — move selection. */
    BTN_EVENT_CONFIRM,      /**< PD2 pressed — confirm/start/reset. */
} ButtonEvent_t;

/* --------------------------------------------------------------------------
 * Command_t
 *
 * High-level intent sent from UITask to WashingManager via Command Queue.
 *
 * UITask is a pure translator — it renames a gesture into an intent and
 * holds no state. It does NOT know where the menu cursor is or which state
 * the machine is in; WashingManager owns both and interprets the intent:
 *
 *   BTN_EVENT_NAVIGATE (PA0)  →  CMD_NAVIGATE
 *   BTN_EVENT_CONFIRM  (PD2)  →  CMD_CONFIRM
 *
 * How WashingManager reads them (see Manager_HandleCommand()):
 *
 *   CMD_NAVIGATE  any state that reads the queue (IDLE/FINISH/ERROR)
 *                 → cursor advances, display refreshed
 *   CMD_CONFIRM   IDLE          → start the program under the cursor
 *                 FINISH/ERROR  → acknowledge, return to IDLE
 *
 * While a cycle is running WashingManager does not read this queue, so both
 * commands are ignored; whatever piled up is discarded on the next return
 * to IDLE/FINISH/ERROR (xQueueReset).
 * -------------------------------------------------------------------------- */
typedef enum
{
    CMD_NAVIGATE = 0, /**< Move the menu cursor to the next item. */
    CMD_CONFIRM,      /**< Start the selected program, or acknowledge FINISH/ERROR. */
} Command_t;

#endif /* APP_COMMON_APP_TYPES_H */
