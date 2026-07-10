/**
 * @file    washing_sm.c
 * @brief   Washing machine state machine — implementation.
 *
 * Table-driven design: all valid transitions are encoded in kTransitions[].
 * The dispatcher loop (WashingSM_ProcessEvent) is universal and never
 * changes when new transitions are added — only the table grows.
 *
 * No FreeRTOS headers. No HAL headers. Pure C logic.
 */

#include "washing_sm.h"
#include <stddef.h>  /* size_t, NULL */

/* --------------------------------------------------------------------------
 * Transition table entry
 * -------------------------------------------------------------------------- */

/**
 * One row in the state transition table.
 * The dispatcher searches for a row where fromState and event both match
 * the current context, then moves to toState.
 */
typedef struct
{
    WashState_t fromState; /**< State in which this transition is valid. */
    WashEvent_t event;     /**< Event that triggers the transition. */
    WashState_t toState;   /**< State to enter when the transition fires. */
} SmTransition_t;

/* --------------------------------------------------------------------------
 * Transition table
 *
 * This is the complete, authoritative list of every valid
 * (state, event) → nextState combination. Stored in ROM (const).
 *
 * To add a transition: append one row. Nothing else changes.
 * To remove a transition: delete the row. Nothing else changes.
 * -------------------------------------------------------------------------- */
static const SmTransition_t kTransitions[] =
{
    /*  fromState               event                   toState              */
    { WASH_STATE_IDLE,       WASH_EVT_START_WASH,  WASH_STATE_FILL_WATER },
    { WASH_STATE_IDLE,       WASH_EVT_START_SPIN,  WASH_STATE_SPIN       },
    { WASH_STATE_FILL_WATER, WASH_EVT_WATER_FULL,  WASH_STATE_WASHING    },
    { WASH_STATE_FILL_WATER, WASH_EVT_TIMEOUT,     WASH_STATE_ERROR      },
    { WASH_STATE_WASHING,    WASH_EVT_WASH_DONE,   WASH_STATE_DRAIN      },
    { WASH_STATE_WASHING,    WASH_EVT_ERROR,       WASH_STATE_ERROR      },
    { WASH_STATE_DRAIN,      WASH_EVT_DRAIN_DONE,  WASH_STATE_SPIN       },
    { WASH_STATE_DRAIN,      WASH_EVT_ERROR,       WASH_STATE_ERROR      },
    { WASH_STATE_SPIN,       WASH_EVT_SPIN_DONE,   WASH_STATE_FINISH     },
    { WASH_STATE_SPIN,       WASH_EVT_ERROR,       WASH_STATE_ERROR      },
    { WASH_STATE_FILL_WATER, WASH_EVT_ERROR,       WASH_STATE_ERROR      },
    { WASH_STATE_FINISH,     WASH_EVT_RESET,       WASH_STATE_IDLE       },
    { WASH_STATE_ERROR,      WASH_EVT_RESET,       WASH_STATE_IDLE       },
};

#define TRANSITION_COUNT  (sizeof(kTransitions) / sizeof(kTransitions[0]))

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

/** Current state of the machine. */
static WashState_t           s_currentState;

/** Entry-action callbacks registered by the caller at init. */
static WashingSM_Callbacks_t s_callbacks;

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief  Call the entry-action callback for the given state.
 *
 * Each case guards against NULL before calling, so partial callback
 * registration (some fields NULL) is safe.
 *
 * @param  state  The state being entered.
 */
static void SM_ExecuteEntryAction(WashState_t state)
{
    switch (state)
    {
        case WASH_STATE_IDLE:
            if (s_callbacks.onEnterIdle)       { s_callbacks.onEnterIdle();       }
            break;

        case WASH_STATE_FILL_WATER:
            if (s_callbacks.onEnterFillWater)  { s_callbacks.onEnterFillWater();  }
            break;

        case WASH_STATE_WASHING:
            if (s_callbacks.onEnterWashing)    { s_callbacks.onEnterWashing();    }
            break;

        case WASH_STATE_DRAIN:
            if (s_callbacks.onEnterDrain)      { s_callbacks.onEnterDrain();      }
            break;

        case WASH_STATE_SPIN:
            if (s_callbacks.onEnterSpin)       { s_callbacks.onEnterSpin();       }
            break;

        case WASH_STATE_FINISH:
            if (s_callbacks.onEnterFinish)     { s_callbacks.onEnterFinish();     }
            break;

        case WASH_STATE_ERROR:
            if (s_callbacks.onEnterError)      { s_callbacks.onEnterError();      }
            break;

        default:
            break;
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void WashingSM_Init(const WashingSM_Callbacks_t *callbacks)
{
    s_currentState = WASH_STATE_IDLE;

    if (callbacks != NULL)
    {
        s_callbacks = *callbacks;
    }
    else
    {
        /* Zero all function pointers — SM runs without any callbacks. */
        s_callbacks = (WashingSM_Callbacks_t){0};
    }
}

WashState_t WashingSM_ProcessEvent(WashEvent_t event)
{
    size_t i;

    for (i = 0; i < TRANSITION_COUNT; i++)
    {
        if (kTransitions[i].fromState == s_currentState &&
            kTransitions[i].event     == event)
        {
            s_currentState = kTransitions[i].toState;
            SM_ExecuteEntryAction(s_currentState);
            break;  /* Only the first matching row fires. */
        }
    }

    /*
     * Return the current state regardless of whether a transition fired.
     * Caller can compare against the value before the call to detect
     * whether the event was consumed.
     */
    return s_currentState;
}

WashState_t WashingSM_GetState(void)
{
    return s_currentState;
}
