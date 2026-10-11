/**
 * @file    app_events.h
 * @brief   FreeRTOS Event Group bit mask definitions.
 *
 * These constants are used with a single shared EventGroupHandle_t.
 * Each bit represents one system condition that tasks can set or wait on.
 *
 * FreeRTOS EventBits_t provides at least 24 usable bits (bits 24-31 are
 * reserved by the kernel).  We occupy bits 0-4.
 *
 * Producers and consumers per bit:
 *
 *   Bit  Mask               Producer          Consumer
 *   ---  -----------------  ----------------  ----------------
 *    0   EVT_DOOR_CLOSED    SensorTask        WashingManager
 *    1   EVT_WATER_FULL     SensorTask        WashingManager
 *    2   EVT_TIMEOUT        Timer callback    WashingManager
 *    3   EVT_FINISHED       WashingManager    (future: UART, OLED)
 *    4   EVT_ERROR          WashingManager    WashingManager
 *                           SensorTask
 *
 * Two distinct ways to read this group — do not confuse them:
 *
 * 1. WAIT for an event that will arrive later (blocking).  The task leaves
 *    the CPU until one of the listed bits is set.  This is what
 *    WashingManager does inside each running state:
 *
 *      xEventGroupWaitBits(xSystemEvents,
 *                          EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR,
 *                          pdTRUE,   // clear the matched bits on exit
 *                          pdFALSE,  // pdFALSE = OR (any one bit wakes us)
 *                          portMAX_DELAY);
 *
 * 2. CHECK a precondition right now (non-blocking guard).  Used to decide
 *    whether a command is even allowed to start a cycle.  Never use the
 *    blocking form for this: a blocked WashingManager cannot report *why*
 *    it refused, so the machine would just appear frozen to the user.
 *
 *      EventBits_t bits = xEventGroupGetBits(xSystemEvents);
 *      if ((bits & EVT_DOOR_CLOSED) == 0u) { reject and tell the user; }
 *
 * Only level-style bits (set/cleared to mirror a physical state, the way
 * SensorTask maintains EVT_DOOR_CLOSED) are valid as guards.  Edge-style
 * bits that are set once per occurrence carry no meaningful "right now"
 * value and must not be polled with GetBits.
 */

#ifndef APP_COMMON_APP_EVENTS_H
#define APP_COMMON_APP_EVENTS_H

/* --------------------------------------------------------------------------
 * Event bit masks
 * -------------------------------------------------------------------------- */

/** Door sensor is closed; safe to run the machine. */
#define EVT_DOOR_CLOSED     (1U << 0)

/**
 * Water level sensor reports full.  Level-style: SensorTask sets it while the
 * sensor is full and clears it as soon as the sensor reports empty, so a stale
 * bit from the previous cycle can never end the next FILL_WATER immediately.
 * The inlet valve stays open until this bit arrives; the fill timer is only a
 * safety timeout (FILL_WATER -> ERROR).
 */
#define EVT_WATER_FULL      (1U << 1)

/**
 * A software timer has expired.
 * WashingManager uses this to advance state (e.g., wash complete → drain).
 *
 * Set by Timer_PhaseCallback() using the plain xEventGroupSetBits() — NOT
 * the FromISR variant.  A software timer callback does not run in ISR
 * context: it is executed by the Timer Service Task (a real FreeRTOS task,
 * priority configTIMER_TASK_PRIORITY), so the ordinary API is correct here.
 * Using xEventGroupSetBitsFromISR() from a task would be wrong, and it is
 * also a deferred operation that can fail if the timer command queue is
 * full — one more reason not to reach for it unless genuinely in an ISR.
 *
 * Note this single bit is shared by all four phase timers (TD-10), so its
 * meaning depends on the state reading it: in FILL_WATER it means "fill
 * timed out" (a fault), while in WASHING it means "wash finished normally".
 */
#define EVT_TIMEOUT         (1U << 2)

/** Current cycle has completed normally; machine is in FINISH state. */
#define EVT_FINISHED        (1U << 3)

/**
 * A fault has been detected (door opened mid-cycle, fill timeout, etc.).
 *
 * Set by SensorTask (door open) or WashingManager.  Cleared in two places:
 *   - automatically by xEventGroupWaitBits(..., xClearOnExit = pdTRUE) in
 *     WashingManager whenever this is the bit that ended the wait;
 *   - explicitly in Manager_OnEnterIdle(), together with EVT_FINISHED, so a
 *     new cycle never starts on a stale fault flag.
 *
 * Beware: SensorTask re-sets this bit on every 100 ms poll while the door
 * is open, so clearing it has no lasting effect until the door is actually
 * closed again.  That is intended for a genuine fault, but it means the bit
 * must not be treated as a one-shot "an error just happened" edge event.
 */
#define EVT_ERROR           (1U << 4)

/** Convenience mask — all defined event bits. */
#define EVT_ALL_BITS        (EVT_DOOR_CLOSED  | \
                             EVT_WATER_FULL   | \
                             EVT_TIMEOUT      | \
                             EVT_FINISHED     | \
                             EVT_ERROR)

#endif /* APP_COMMON_APP_EVENTS_H */
