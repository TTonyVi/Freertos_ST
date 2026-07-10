/**
 * @file    washing_sm.h
 * @brief   Washing machine state machine — public interface.
 *
 * This module implements a pure-logic, table-driven state machine.
 * It has no FreeRTOS dependency and no GPIO dependency.
 * Hardware actions are decoupled via entry-action callbacks registered
 * at init time by the WashingManager task.
 *
 * Typical usage in WashingManager task:
 *
 * @code
 *   static WashingSM_Callbacks_t cbs = {
 *       .onEnterFillWater = Manager_OnEnterFillWater,
 *       .onEnterWashing   = Manager_OnEnterWashing,
 *       // ... etc
 *   };
 *   WashingSM_Init(&cbs);
 *
 *   // Later, when a command arrives:
 *   WashState_t newState = WashingSM_ProcessEvent(WASH_EVT_START_WASH);
 * @endcode
 */

#ifndef APP_WASHINGMANAGER_WASHING_SM_H
#define APP_WASHINGMANAGER_WASHING_SM_H

#include "app_types.h"

/* --------------------------------------------------------------------------
 * WashEvent_t
 *
 * The SM's own event vocabulary. Lives here, not in app_types.h, because
 * nothing outside the WashingManager module ever constructs or names these.
 *
 * The WashingManager task translates inbound signals into WashEvent_t:
 *
 *   CommandMsg_t  CMD_START_WASH  → WASH_EVT_START_WASH
 *   CommandMsg_t  CMD_START_SPIN  → WASH_EVT_START_SPIN
 *   CommandMsg_t  CMD_RESET       → WASH_EVT_RESET
 *   EVT_WATER_FULL (event group)  → WASH_EVT_WATER_FULL
 *   EVT_TIMEOUT   (event group)   → WASH_EVT_TIMEOUT or WASH_EVT_WASH_DONE
 *                                   depending on current state
 *
 * Transition table (all valid combinations):
 *
 *   State          Event          Next State
 *   -------------- -------------- --------------
 *   IDLE           START_WASH     FILL_WATER
 *   IDLE           START_SPIN     SPIN
 *   FILL_WATER     WATER_FULL     WASHING
 *   FILL_WATER     TIMEOUT        ERROR
 *   WASHING        WASH_DONE      DRAIN
 *   DRAIN          DRAIN_DONE     SPIN
 *   SPIN           SPIN_DONE      FINISH
 *   FINISH         RESET          IDLE
 *   ERROR          RESET          IDLE
 *
 * Any (state, event) pair not listed above is silently ignored.
 * -------------------------------------------------------------------------- */
typedef enum
{
    WASH_EVT_START_WASH = 0, /**< User started the full wash program. */
    WASH_EVT_START_SPIN,     /**< User started the spin-only program. */
    WASH_EVT_WATER_FULL,     /**< Sensor confirmed water level reached. */
    WASH_EVT_WASH_DONE,      /**< Wash timer expired — agitation complete. */
    WASH_EVT_DRAIN_DONE,     /**< Drain timer expired — drain complete. */
    WASH_EVT_SPIN_DONE,      /**< Spin timer expired — spin complete. */
    WASH_EVT_TIMEOUT,        /**< Fill-water timeout — safety error condition. */
    WASH_EVT_ERROR,          /**< Fault detected mid-cycle (door open, etc.). */
    WASH_EVT_RESET,          /**< User requested reset from FINISH or ERROR. */
    WASH_EVT_COUNT,          /**< Sentinel — total number of events. */
} WashEvent_t;

/* --------------------------------------------------------------------------
 * WashingSM_Callbacks_t
 *
 * Entry-action callbacks: called once each time the SM enters a new state.
 * All fields are nullable — pass NULL for any callback not yet implemented.
 * The SM checks for NULL before calling.
 *
 * Implementors (WashingManager task) perform hardware control here:
 *   - GPIO writes (motor, valves)
 *   - Software timer start/stop
 *   - Display queue sends
 * -------------------------------------------------------------------------- */
typedef struct
{
    void (*onEnterIdle)(void);       /**< Machine halted; all outputs off. */
    void (*onEnterFillWater)(void);  /**< Open inlet valve; arm fill timeout. */
    void (*onEnterWashing)(void);    /**< Close inlet valve; start motor; start wash timer. */
    void (*onEnterDrain)(void);      /**< Stop motor; open drain valve; start drain timer. */
    void (*onEnterSpin)(void);       /**< Start motor at spin speed; start spin timer. */
    void (*onEnterFinish)(void);     /**< All outputs off; notify user. */
    void (*onEnterError)(void);      /**< All outputs off; display error. */
} WashingSM_Callbacks_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialize the state machine.
 *
 * Sets the initial state to IDLE and copies the caller's callback table.
 * Does NOT call the onEnterIdle callback — the assumption is the system
 * starts in a known-off condition.
 *
 * @param  callbacks  Pointer to a callback struct. May be NULL (all
 *                    callbacks disabled, useful for early bring-up).
 */
void WashingSM_Init(const WashingSM_Callbacks_t *callbacks);

/**
 * @brief  Process one event against the current state.
 *
 * Searches the transition table for a row matching (currentState, event).
 * If found:
 *   1. Advances to the next state.
 *   2. Calls the new state's entry-action callback (if registered).
 * If not found:
 *   - Event is silently ignored; state is unchanged.
 *
 * @param  event  The event to process.
 * @return        The current state after processing (new state if a
 *                transition fired, unchanged state if event was ignored).
 */
WashState_t WashingSM_ProcessEvent(WashEvent_t event);

/**
 * @brief  Return the current state without triggering any transition.
 *
 * Safe to call from any context. Does not modify state.
 *
 * @return  Current WashState_t value.
 */
WashState_t WashingSM_GetState(void);

#endif /* APP_WASHINGMANAGER_WASHING_SM_H */
