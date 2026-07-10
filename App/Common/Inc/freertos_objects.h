/**
 * @file    freertos_objects.h
 * @brief   Declarations of all shared FreeRTOS communication objects.
 *
 * This file is the single include a task needs to access any queue or
 * event group in the system. Every handle is defined (allocated) once
 * in freertos_objects.c and declared extern here so other translation
 * units can reference them.
 *
 * Call App_FreeRTOSObjectsInit() once from main(), before
 * vTaskStartScheduler(), to create all objects on heap_4.
 *
 * Communication channel summary:
 *
 *   xButtonQueue    ButtonEvent_t    ISR            → UITask
 *   xCommandQueue   CommandMsg_t     UITask         → WashingManager
 *   xDisplayQueue   DisplayMsg_t     WashingManager → OLEDTask
 *   xSystemEvents   (bit flags)      SensorTask     → WashingManager
 *                                    Timer callback → WashingManager
 *
 * @note No task handles are declared here. Task handles belong to the
 *       files that create them and are passed as parameters where needed.
 */

#ifndef APP_COMMON_FREERTOS_OBJECTS_H
#define APP_COMMON_FREERTOS_OBJECTS_H

/* --------------------------------------------------------------------------
 * FreeRTOS includes
 * -------------------------------------------------------------------------- */
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

/* --------------------------------------------------------------------------
 * Application type includes (for sizeof() in queue item-size calculations)
 * -------------------------------------------------------------------------- */
#include "app_types.h"
#include "app_msg.h"
#include "app_config.h"

/* --------------------------------------------------------------------------
 * Queue handles
 *
 * FreeRTOS queues are FIFO buffers that copy items by value.
 * Items accumulate if the consumer is slower than the producer;
 * the producer blocks (or returns errQUEUE_FULL from ISR context)
 * only when the queue is completely full.
 *
 * Why not Task Notification for these channels?
 *   - Task Notification has a depth of 1: a second notification before
 *     the first is consumed would overwrite or be rejected. That would
 *     silently drop button presses or commands.
 *   - Task Notification carries a 32-bit scalar; CommandMsg_t and
 *     DisplayMsg_t are structs that cannot fit in 32 bits.
 *   - Task Notification is 1:1 (named recipient). Event Group is used
 *     instead where multiple tasks may eventually wait on the same bit.
 * -------------------------------------------------------------------------- */

/**
 * @brief Button event queue.
 *
 * Producer : Button ISR (via xQueueSendFromISR)
 * Consumer : UITask     (via xQueueReceive)
 * Item     : ButtonEvent_t — one value per press gesture
 * Depth    : BUTTON_QUEUE_LENGTH
 *
 * The ISR posts here immediately and returns; UITask reads when scheduled.
 * Depth absorbs rapid button presses before UITask gets CPU time.
 */
extern QueueHandle_t xButtonQueue;

/**
 * @brief Command queue.
 *
 * Producer : UITask         (via xQueueSend)
 * Consumer : WashingManager (via xQueueReceive)
 * Item     : CommandMsg_t  — command + mode
 * Depth    : COMMAND_QUEUE_LENGTH
 *
 * UITask translates button gestures and menu state into high-level
 * commands. WashingManager may be blocked on xSystemEvents when a
 * command arrives; the queue buffers it without loss.
 */
extern QueueHandle_t xCommandQueue;

/**
 * @brief Display update queue.
 *
 * Producer : WashingManager (via xQueueSend)
 * Consumer : OLEDTask       (via xQueueReceive)
 * Item     : DisplayMsg_t  — full screen state
 * Depth    : DISPLAY_QUEUE_LENGTH
 *
 * OLEDTask (lowest priority) may be mid-render when WashingManager
 * changes state. Queue decouples them so WashingManager never stalls
 * waiting for the display to finish.
 */
extern QueueHandle_t xDisplayQueue;

/* --------------------------------------------------------------------------
 * Event Group handle
 *
 * An Event Group is a set of persistent bit flags. Multiple tasks can
 * wait on any combination of bits in a single blocking call.
 * Bits stay set until explicitly cleared by the consumer.
 *
 * Why Event Group instead of Queue for system events?
 *   - WashingManager needs to wait for EVT_DOOR_CLOSED | EVT_WATER_FULL
 *     simultaneously. xQueueReceive can only wait for one item; the
 *     caller would need a polling loop to check multiple conditions.
 *   - Bits persist: if SensorTask sets EVT_DOOR_CLOSED before
 *     WashingManager reaches its wait call, the bit is still set.
 *     A queue message might be missed if the consumer is not ready.
 *   - Future tasks (e.g. a UART debug logger) can independently wait
 *     on the same EVT_ERROR bit — Event Group broadcasts; Queue does not.
 *
 * Bit assignments are defined in app_events.h.
 * -------------------------------------------------------------------------- */

/**
 * @brief System event group.
 *
 * Producers : SensorTask  (EVT_DOOR_CLOSED, EVT_WATER_FULL, EVT_ERROR)
 *             Timer cbk   (EVT_TIMEOUT)
 *             WashingMgr  (EVT_FINISHED, EVT_ERROR)
 * Consumer  : WashingManager (xEventGroupWaitBits)
 *
 * ISR-context producers must use xEventGroupSetBitsFromISR().
 */
extern EventGroupHandle_t xSystemEvents;

/* --------------------------------------------------------------------------
 * Software Timer handles
 *
 * One-shot timers — fire once per cycle phase, then stop automatically.
 * All callbacks set EVT_TIMEOUT on xSystemEvents.
 * WashingManager determines meaning from current SM state (no timer ID needed).
 *
 * Timers are created in App_FreeRTOSObjectsInit().
 * xTimerStart() is called from the SM entry-action callbacks in washing_manager.c.
 * -------------------------------------------------------------------------- */

/** Fill-water timeout — transitions FILL_WATER → ERROR if water never arrives. */
extern TimerHandle_t xFillTimeoutTimer;

/** Wash phase timer — transitions WASHING → DRAIN when agitation is complete. */
extern TimerHandle_t xWashTimer;

/** Drain phase timer — transitions DRAIN → SPIN when drain is complete. */
extern TimerHandle_t xDrainTimer;

/** Spin phase timer — transitions SPIN → FINISH when spin is complete. */
extern TimerHandle_t xSpinTimer;

/* --------------------------------------------------------------------------
 * Initialization
 * -------------------------------------------------------------------------- */

/**
 * @brief  Create all FreeRTOS communication objects.
 *
 * Allocates all queues and the event group from heap_4.
 * Must be called once from main(), before vTaskStartScheduler().
 * Traps on allocation failure via configASSERT (heap_4 exhausted or
 * configQUEUE_REGISTRY_SIZE too small).
 */
void App_FreeRTOSObjectsInit(void);

#endif /* APP_COMMON_FREERTOS_OBJECTS_H */
