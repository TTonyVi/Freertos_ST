/**
 * @file    washing_manager.c
 * @brief   WashingManager task implementation.
 */

#include "washing_manager.h"
#include "washing_sm.h"
#include "freertos_objects.h"   /* xCommandQueue, xDisplayQueue, xSystemEvents  */
#include "app_events.h"         /* EVT_WATER_FULL, EVT_TIMEOUT, EVT_FINISHED    */
#include "bsp_motor.h"
#include "bsp_valve.h"
#include "bsp_led.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "timers.h"

#include <string.h>

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */

static WashMode_t s_currentMode  = WASH_MODE_WASH;

/* The menu cursor (0 = Wash, 1 = Spin). This is the ONLY copy: UITask holds
 * none, so what the OLED shows and what Start will run can never disagree. */
static uint8_t    s_menuIndex    = 0u;

/* --------------------------------------------------------------------------
 * Private function declarations
 * -------------------------------------------------------------------------- */

static const char *Manager_StatusText(WashState_t state);
static void Manager_PostDisplay(WashState_t state);
static void Manager_HandleCommand(const CommandMsg_t *cmd);
static void Manager_StartPhaseTimer(TimerHandle_t timer);

/* SM entry-action callbacks — one per state */
static void Manager_OnEnterIdle(void);
static void Manager_OnEnterFillWater(void);
static void Manager_OnEnterWashing(void);
static void Manager_OnEnterDrain(void);
static void Manager_OnEnterSpin(void);
static void Manager_OnEnterFinish(void);
static void Manager_OnEnterError(void);

/* --------------------------------------------------------------------------
 * SM entry-action callbacks
 *
 * Called once by WashingSM_ProcessEvent() when the SM enters a new state.
 * Each callback:
 *   1. Drives BSP actuators to the correct state for that phase.
 *   2. Sends a DisplayMsg_t to xDisplayQueue so OLEDTask can update.
 *   3. Sets or clears relevant event group bits if needed.
 *
 * These run in WashingManager task context (not ISR).
 * -------------------------------------------------------------------------- */

/*
 * Start the one-shot timer of a timed phase (fill / wash / drain / spin).
 *
 * All four timers share ONE bit, EVT_TIMEOUT, so the bit carries no
 * information about WHICH timer fired. A bit left over from the previous
 * phase would therefore be mistaken for the new phase finishing at once.
 * Clearing it here, right before the new timer starts, removes any such
 * stale bit; the new timer cannot have fired yet.
 *
 * Residual window: xTimerStop() of the previous timer only queues a command,
 * and the timer service task handles an already-expired timer BEFORE it reads
 * its command queue. If the old timer expires in that gap its callback sets
 * EVT_TIMEOUT after this clear. Closing it fully needs one bit per timer.
 */
static void Manager_StartPhaseTimer(TimerHandle_t timer)
{
    xEventGroupClearBits(xSystemEvents, EVT_TIMEOUT);
    xTimerStart(timer, 0);
}

static void Manager_OnEnterIdle(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_INLET);
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    /* Clear status bits left over from a completed or reset cycle. */
    xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);

    /* Clear any stale commands that queued up while the machine was running. */
    (void)xQueueReset(xCommandQueue);

    Manager_PostDisplay(WASH_STATE_IDLE);
}

static void Manager_OnEnterFillWater(void)
{
    BSP_Valve_Open(BSP_VALVE_INLET);
    BSP_LED_On();
    Manager_StartPhaseTimer(xFillTimeoutTimer);
    Manager_PostDisplay(WASH_STATE_FILL_WATER);
}

static void Manager_OnEnterWashing(void)
{
    BSP_Valve_Close(BSP_VALVE_INLET);   /* stop filling before motor starts */
    BSP_Motor_Start();
    Manager_StartPhaseTimer(xWashTimer);
    Manager_PostDisplay(WASH_STATE_WASHING);
}

static void Manager_OnEnterDrain(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Open(BSP_VALVE_DRAIN);
    Manager_StartPhaseTimer(xDrainTimer);
    Manager_PostDisplay(WASH_STATE_DRAIN);
}

static void Manager_OnEnterSpin(void)
{
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_Motor_Start();
    Manager_StartPhaseTimer(xSpinTimer);
    Manager_PostDisplay(WASH_STATE_SPIN);
}

static void Manager_OnEnterFinish(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    xEventGroupSetBits(xSystemEvents, EVT_FINISHED);

    /* Clear any stale commands that queued up while the machine was running. */
    (void)xQueueReset(xCommandQueue);

    Manager_PostDisplay(WASH_STATE_FINISH);
}

static void Manager_OnEnterError(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_INLET);
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    xEventGroupSetBits(xSystemEvents, EVT_ERROR);

    /* Clear any stale commands that queued up while the machine was running. */
    (void)xQueueReset(xCommandQueue);

    Manager_PostDisplay(WASH_STATE_ERROR);
}

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

/*
 * Status line shown on the OLED for each state. One table, one place: every
 * display message is built from the state, so the text can never disagree
 * with the state the way a hand-passed (or cached) string could.
 */
static const char *Manager_StatusText(WashState_t state)
{
    static const char *const kText[] = {
        [WASH_STATE_IDLE]       = "IDLE",
        [WASH_STATE_FILL_WATER] = "FILL WATER",
        [WASH_STATE_WASHING]    = "WASHING",
        [WASH_STATE_DRAIN]      = "DRAIN",
        [WASH_STATE_SPIN]       = "SPINNING",
        [WASH_STATE_FINISH]     = "FINISHED",
        [WASH_STATE_ERROR]      = "ERROR",
    };

    if ((unsigned)state < (sizeof(kText) / sizeof(kText[0])) && kText[state] != NULL)
    {
        return kText[state];
    }
    return "?";
}

/*
 * Build a complete DisplayMsg_t — state, status text, mode AND cursor — and
 * send it to OLEDTask. WashingManager is the only producer of this message,
 * which is what guarantees every field is consistent.
 *
 * Uses timeout 0 — if the display queue is full, the update is silently
 * dropped rather than blocking WashingManager.  OLEDTask will render the
 * next queued message instead.
 */
static void Manager_PostDisplay(WashState_t state)
{
    DisplayMsg_t msg;
    msg.state        = state;
    msg.selectedMode = s_currentMode;
    msg.menuIndex    = s_menuIndex;

    (void)strncpy(msg.statusText, Manager_StatusText(state), sizeof(msg.statusText) - 1u);
    msg.statusText[sizeof(msg.statusText) - 1u] = '\0';

    xQueueSend(xDisplayQueue, &msg, 0);
}

/*
 * Interpret a CommandMsg_t from xCommandQueue.
 *
 * Only reached from IDLE / FINISH / ERROR — the other states block on the
 * event group and never read this queue (see the task loop).
 *
 *   CMD_NAVIGATE  advance the cursor and refresh the screen. The status line
 *                 is rebuilt from the CURRENT state, not guessed.
 *   CMD_CONFIRM   FINISH / ERROR → acknowledge, back to IDLE (Bug #10 fix:
 *                 whatever the user presses in a terminal state means
 *                 "I have seen it").
 *                 IDLE → start the program under the cursor.
 */
static void Manager_HandleCommand(const CommandMsg_t *cmd)
{
    const WashState_t state = WashingSM_GetState();

    switch (cmd->command)
    {
        case CMD_NAVIGATE:
            s_menuIndex = (uint8_t)((s_menuIndex + 1u) % MENU_ITEM_COUNT);
            Manager_PostDisplay(state);
            break;

        case CMD_CONFIRM:
            if (state == WASH_STATE_FINISH || state == WASH_STATE_ERROR)
            {
                WashingSM_ProcessEvent(WASH_EVT_RESET);
            }
            else if (s_menuIndex == 0u)
            {
                s_currentMode = WASH_MODE_WASH;
                WashingSM_ProcessEvent(WASH_EVT_START_WASH);
            }
            else
            {
                s_currentMode = WASH_MODE_SPIN;
                WashingSM_ProcessEvent(WASH_EVT_START_SPIN);
            }
            break;

        default:
            break;
    }
}

/* --------------------------------------------------------------------------
 * Task
 * -------------------------------------------------------------------------- */

void WashingManager_Task(void *pvParameters)
{
    (void)pvParameters;

    /* ------------------------------------------------------------------
     * One-time initialisation
     * ------------------------------------------------------------------ */

    static const WashingSM_Callbacks_t s_callbacks = {
        .onEnterIdle      = Manager_OnEnterIdle,
        .onEnterFillWater = Manager_OnEnterFillWater,
        .onEnterWashing   = Manager_OnEnterWashing,
        .onEnterDrain     = Manager_OnEnterDrain,
        .onEnterSpin      = Manager_OnEnterSpin,
        .onEnterFinish    = Manager_OnEnterFinish,
        .onEnterError     = Manager_OnEnterError,
    };
    WashingSM_Init(&s_callbacks);

    BSP_Motor_Init();
    BSP_Valve_Init();
    BSP_LED_Init();

    Manager_PostDisplay(WASH_STATE_IDLE);

    /* ------------------------------------------------------------------
     * State-dependent blocking loop
     *
     * Each iteration:
     *   1. Read current SM state.
     *   2. Block on the signal that state is waiting for.
     *   3. Translate the signal into a WashEvent_t.
     *   4. Call WashingSM_ProcessEvent() — fires the entry callback for
     *      the new state, which drives the BSP and posts a display update.
     * ------------------------------------------------------------------ */
    for (;;)
    {
        WashState_t state = WashingSM_GetState();

        switch (state)
        {
            /* --------------------------------------------------------------
             * IDLE / FINISH / ERROR
             * Block until UITask sends a command via xCommandQueue.
             * -------------------------------------------------------------- */
            case WASH_STATE_IDLE:
            case WASH_STATE_FINISH:
            case WASH_STATE_ERROR:
            {
                CommandMsg_t cmd;
                xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY);
                Manager_HandleCommand(&cmd);
                break;
            }

            /* --------------------------------------------------------------
             * FILL_WATER
             * Block until SensorTask sets EVT_WATER_FULL, or a fill-timeout
             * timer (Phase 7) sets EVT_TIMEOUT.
             * Either bit clears automatically on exit (pdTRUE).
             *
             * EVT_ERROR is tested FIRST in every timed state. xClearOnExit
             * clears the whole wait mask, so when two bits arrive together
             * only the first branch below runs and the other bit is gone.
             * Safety wins that race: a fault must never lose to a normal
             * event. The same order is used in WASHING / DRAIN / SPIN.
             * -------------------------------------------------------------- */
            case WASH_STATE_FILL_WATER:
            {
                EventBits_t bits = xEventGroupWaitBits(
                    xSystemEvents,
                    EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xFillTimeoutTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
                }
                else if ((bits & EVT_WATER_FULL) != 0u)
                {
                    xTimerStop(xFillTimeoutTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_WATER_FULL);
                }
                else if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_TIMEOUT);
                }
                break;
            }

            case WASH_STATE_WASHING:
            {
                EventBits_t bits = xEventGroupWaitBits(
                    xSystemEvents,
                    EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xWashTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
                }
                else if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_WASH_DONE);
                }
                break;
            }

            case WASH_STATE_DRAIN:
            {
                EventBits_t bits = xEventGroupWaitBits(
                    xSystemEvents,
                    EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xDrainTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
                }
                else if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_DRAIN_DONE);
                }
                break;
            }

            case WASH_STATE_SPIN:
            {
                EventBits_t bits = xEventGroupWaitBits(
                    xSystemEvents,
                    EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xSpinTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
                }
                else if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_SPIN_DONE);
                }
                break;
            }

            default:
                vTaskDelay(pdMS_TO_TICKS(100U));
                break;
        }
    }
}
