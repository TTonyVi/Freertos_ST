/**
 * @file    freertos_objects.c
 * @brief   Definition and initialization of all shared FreeRTOS objects.
 *
 * This translation unit owns the storage for every queue and event group
 * handle used by the application. All other files access them via the
 * extern declarations in freertos_objects.h.
 *
 * Initialization sequence in main():
 *
 *   1. HAL_Init()
 *   2. SystemClock_Config()
 *   3. MX_GPIO_Init() / MX_I2C1_Init() ...
 *   4. App_FreeRTOSObjectsInit()   <-- this file
 *   5. [Task create calls — Phase 3+]
 *   6. vTaskStartScheduler()
 */

#include "freertos_objects.h"
#include "app_events.h"
#include "timers.h"

/* --------------------------------------------------------------------------
 * Handle definitions
 *
 * Declared NULL so that any access before App_FreeRTOSObjectsInit() is
 * called will be caught by configASSERT in the consumer.
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Timer handle definitions
 * -------------------------------------------------------------------------- */

TimerHandle_t xFillTimeoutTimer = NULL;
TimerHandle_t xWashTimer        = NULL;
TimerHandle_t xDrainTimer       = NULL;
TimerHandle_t xSpinTimer        = NULL;

/* --------------------------------------------------------------------------
 * Timer callback
 *
 * All four phase timers share one callback — they all do the same thing:
 * signal WashingManager that the current phase has ended.
 * WashingManager reads the current SM state to know WHICH phase ended.
 *
 * Called from Timer Service Task context (not ISR) — xEventGroupSetBits()
 * is safe here. Must not block.
 * -------------------------------------------------------------------------- */
static void Timer_PhaseCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT);
}

/* --------------------------------------------------------------------------
 * Queue and event group handle definitions
 * -------------------------------------------------------------------------- */

/** @brief See freertos_objects.h for full description. */
QueueHandle_t xButtonQueue  = NULL;

/** @brief See freertos_objects.h for full description. */
QueueHandle_t xCommandQueue = NULL;

/** @brief See freertos_objects.h for full description. */
QueueHandle_t xDisplayQueue = NULL;

/** @brief See freertos_objects.h for full description. */
EventGroupHandle_t xSystemEvents = NULL;

/* --------------------------------------------------------------------------
 * App_FreeRTOSObjectsInit
 * -------------------------------------------------------------------------- */

/**
 * @brief  Create all FreeRTOS communication objects.
 *
 * Each xQueueCreate() call allocates memory from heap_4:
 *   - Queue control block (80 bytes — sizeof(StaticQueue_t))
 *   - Storage buffer      (length × itemSize bytes)
 *
 * RAM cost, MEASURED with arm-none-eabi-gcc using this project's flags
 * (sizeof via a compile-time probe, not estimated):
 *   xButtonQueue   :  85 bytes  (80 + 5 × sizeof(ButtonEvent_t) = 5 × 1)
 *   xCommandQueue  :  85 bytes  (80 + 5 × sizeof(CommandMsg_t)  = 5 × 1)
 *   xDisplayQueue  : 215 bytes  (80 + 5 × sizeof(DisplayMsg_t)  = 5 × 27)
 *   xSystemEvents  :  28 bytes  (sizeof(StaticEventGroup_t))
 *   Total          : 413 bytes, plus heap_4's per-allocation header and
 *                    8-byte rounding (out of 128 KB SRAM — negligible)
 *
 * Why the item sizes are 1 / 1 / 27 and not 4 / 4 / 36: arm-none-eabi-gcc
 * defaults to -fshort-enums, so an enum occupies the smallest integer that
 * fits (1 byte here), not an int. Never assume sizeof(enum) == 4 on this
 * toolchain — measure it.
 *
 * configASSERT() traps at boot if any allocation fails, ensuring a
 * heap misconfiguration is caught immediately rather than causing a
 * silent NULL-pointer dereference at runtime.
 */
void App_FreeRTOSObjectsInit(void)
{
    /* -----------------------------------------------------------------------
     * Button Queue
     *
     * xQueueCreate(uxQueueLength, uxItemSize)
     *   uxQueueLength : maximum number of items the queue can hold
     *   uxItemSize    : size of each item IN BYTES — FreeRTOS copies this
     *                   many bytes on every send and receive
     *
     * sizeof(ButtonEvent_t) = 1 byte (-fshort-enums: smallest integer that
     * fits, not int). The enum value is copied by value; no pointer is stored
     * in the queue.
     * ----------------------------------------------------------------------- */
    xButtonQueue = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(ButtonEvent_t));
    configASSERT(xButtonQueue != NULL);

    /* -----------------------------------------------------------------------
     * Command Queue
     *
     * sizeof(CommandMsg_t) = sizeof(Command_t) = 1 byte (short enum, no
     * padding). The full struct is copied into the queue storage on each send.
     * ----------------------------------------------------------------------- */
    xCommandQueue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(CommandMsg_t));
    configASSERT(xCommandQueue != NULL);

    /* -----------------------------------------------------------------------
     * Display Queue
     *
     * sizeof(DisplayMsg_t) = 1 (state) + 1 (selectedMode) + 1 (menuIndex)
     *                      + 24 (statusText) = 27 bytes, no padding.
     * The statusText[24] char array is part of the struct and is copied
     * entirely — no heap string, no pointer, no fragmentation risk.
     * ----------------------------------------------------------------------- */
    xDisplayQueue = xQueueCreate(DISPLAY_QUEUE_LENGTH, sizeof(DisplayMsg_t));
    configASSERT(xDisplayQueue != NULL);

    /* -----------------------------------------------------------------------
     * System Event Group
     *
     * xEventGroupCreate() allocates an EventGroup_t from heap_4.
     * All event bits start at 0 (no condition is initially asserted).
     * Bit assignments are in app_events.h.
     * ----------------------------------------------------------------------- */
    xSystemEvents = xEventGroupCreate();
    configASSERT(xSystemEvents != NULL);

    /* -----------------------------------------------------------------------
     * Software Timers
     *
     * xTimerCreate(pcTimerName,
     *              xTimerPeriodInTicks,
     *              uxAutoReload,    -- pdFALSE = one-shot; fires once then stops
     *              pvTimerID,       -- not used; WashingManager reads SM state instead
     *              pxCallbackFunction)
     *
     * Periods use pdMS_TO_TICKS() to convert ms → ticks at configTICK_RATE_HZ=1000.
     * Timers are created in stopped state; xTimerStart() is called from the
     * SM entry-action callbacks in washing_manager.c.
     * ----------------------------------------------------------------------- */
    xFillTimeoutTimer = xTimerCreate("FillTmr",
                                     pdMS_TO_TICKS(WASH_FILL_DURATION_MS),
                                     pdFALSE,
                                     NULL,
                                     Timer_PhaseCallback);
    configASSERT(xFillTimeoutTimer != NULL);

    xWashTimer = xTimerCreate("WashTmr",
                              pdMS_TO_TICKS(WASH_CYCLE_DURATION_MS),
                              pdFALSE,
                              NULL,
                              Timer_PhaseCallback);
    configASSERT(xWashTimer != NULL);

    xDrainTimer = xTimerCreate("DrainTmr",
                               pdMS_TO_TICKS(WASH_DRAIN_DURATION_MS),
                               pdFALSE,
                               NULL,
                               Timer_PhaseCallback);
    configASSERT(xDrainTimer != NULL);

    xSpinTimer = xTimerCreate("SpinTmr",
                              pdMS_TO_TICKS(SPIN_DURATION_MS),
                              pdFALSE,
                              NULL,
                              Timer_PhaseCallback);
    configASSERT(xSpinTimer != NULL);

    /* -----------------------------------------------------------------------
     * SEGGER SystemView queue registry
     *
     * vQueueAddToRegistry() assigns a human-readable name to each handle.
     * When SystemView is connected, queues appear by name in the timeline
     * rather than as raw memory addresses — essential for debugging.
     *
     * Compiled in only when configQUEUE_REGISTRY_SIZE > 0 in
     * FreeRTOSConfig.h. Set it to at least 4 to cover these three queues
     * plus one spare.
     * ----------------------------------------------------------------------- */
#if (configQUEUE_REGISTRY_SIZE > 0)
    vQueueAddToRegistry(xButtonQueue,  "ButtonQ");
    vQueueAddToRegistry(xCommandQueue, "CommandQ");
    vQueueAddToRegistry(xDisplayQueue, "DisplayQ");
#endif
}
