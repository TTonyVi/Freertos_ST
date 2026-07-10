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
static uint8_t    s_menuIndex    = 0u;

/* --------------------------------------------------------------------------
 * Private function declarations
 * -------------------------------------------------------------------------- */

static void Manager_PostDisplay(WashState_t state, const char *statusText);
static void Manager_HandleCommand(const CommandMsg_t *cmd);

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

static void Manager_OnEnterIdle(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_INLET);
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    /* Clear status bits left over from a completed or reset cycle. */
    xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);

    Manager_PostDisplay(WASH_STATE_IDLE, "IDLE");
}

static void Manager_OnEnterFillWater(void)
{
    BSP_Valve_Open(BSP_VALVE_INLET);
    BSP_LED_On();
    xTimerStart(xFillTimeoutTimer, 0);
    Manager_PostDisplay(WASH_STATE_FILL_WATER, "FILL WATER");
}

static void Manager_OnEnterWashing(void)
{
    BSP_Valve_Close(BSP_VALVE_INLET);   /* stop filling before motor starts */
    BSP_Motor_Start();
    xTimerStart(xWashTimer, 0);
    Manager_PostDisplay(WASH_STATE_WASHING, "WASHING");
}

static void Manager_OnEnterDrain(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Open(BSP_VALVE_DRAIN);
    xTimerStart(xDrainTimer, 0);
    Manager_PostDisplay(WASH_STATE_DRAIN, "DRAIN");
}

static void Manager_OnEnterSpin(void)
{
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_Motor_Start();
    xTimerStart(xSpinTimer, 0);
    Manager_PostDisplay(WASH_STATE_SPIN, "SPINNING");
}

static void Manager_OnEnterFinish(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    xEventGroupSetBits(xSystemEvents, EVT_FINISHED);

    Manager_PostDisplay(WASH_STATE_FINISH, "FINISHED");
}

static void Manager_OnEnterError(void)
{
    BSP_Motor_Stop();
    BSP_Valve_Close(BSP_VALVE_INLET);
    BSP_Valve_Close(BSP_VALVE_DRAIN);
    BSP_LED_Off();

    xEventGroupSetBits(xSystemEvents, EVT_ERROR);

    Manager_PostDisplay(WASH_STATE_ERROR, "ERROR");
}

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

/*
 * Build a DisplayMsg_t from the current state and send it to OLEDTask.
 * Uses timeout 0 — if the display queue is full, the update is silently
 * dropped rather than blocking WashingManager.  OLEDTask will render the
 * next queued message instead.
 */
static void Manager_PostDisplay(WashState_t state, const char *statusText)
{
    DisplayMsg_t msg;
    msg.state        = state;
    msg.selectedMode = s_currentMode;
    msg.menuIndex    = s_menuIndex;

    (void)strncpy(msg.statusText, statusText, sizeof(msg.statusText) - 1u);
    msg.statusText[sizeof(msg.statusText) - 1u] = '\0';

    xQueueSend(xDisplayQueue, &msg, 0);
}

/*
 * Translate a CommandMsg_t received from xCommandQueue into a WashEvent_t
 * and forward it to the state machine.
 *
 * Also records the selected mode so that display messages carry the correct
 * mode while the cycle is running.
 */
static void Manager_HandleCommand(const CommandMsg_t *cmd)
{
    s_menuIndex = cmd->menuIndex;

    switch (cmd->command)
    {
        case CMD_START_WASH:
            s_currentMode = WASH_MODE_WASH;
            WashingSM_ProcessEvent(WASH_EVT_START_WASH);
            break;

        case CMD_START_SPIN:
            s_currentMode = WASH_MODE_SPIN;
            WashingSM_ProcessEvent(WASH_EVT_START_SPIN);
            break;

        case CMD_RESET:
            WashingSM_ProcessEvent(WASH_EVT_RESET);
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

    Manager_PostDisplay(WASH_STATE_IDLE, "IDLE");

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
             * -------------------------------------------------------------- */
            case WASH_STATE_FILL_WATER:
            {
                EventBits_t bits = xEventGroupWaitBits(
                    xSystemEvents,
                    EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR,
                    pdTRUE,
                    pdFALSE,
                    portMAX_DELAY);

                if ((bits & EVT_WATER_FULL) != 0u)
                {
                    xTimerStop(xFillTimeoutTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_WATER_FULL);
                }
                else if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_TIMEOUT);
                }
                else if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xFillTimeoutTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
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

                if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_WASH_DONE);
                }
                else if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xWashTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
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

                if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_DRAIN_DONE);
                }
                else if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xDrainTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
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

                if ((bits & EVT_TIMEOUT) != 0u)
                {
                    WashingSM_ProcessEvent(WASH_EVT_SPIN_DONE);
                }
                else if ((bits & EVT_ERROR) != 0u)
                {
                    xTimerStop(xSpinTimer, 0);
                    WashingSM_ProcessEvent(WASH_EVT_ERROR);
                }
                break;
            }

            default:
                vTaskDelay(pdMS_TO_TICKS(100U));
                break;
        }
    }
}
