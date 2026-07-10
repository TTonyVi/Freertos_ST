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
 * Usage example (WashingManager waiting for door closed before starting):
 *
 *   xEventGroupWaitBits(hSystemEvents,
 *                       EVT_DOOR_CLOSED,  // bits to wait for
 *                       pdTRUE,           // clear on exit
 *                       pdTRUE,           // wait for ALL listed bits
 *                       portMAX_DELAY);
 */

#ifndef APP_COMMON_APP_EVENTS_H
#define APP_COMMON_APP_EVENTS_H

/* --------------------------------------------------------------------------
 * Event bit masks
 * -------------------------------------------------------------------------- */

/** Door sensor is closed; safe to run the machine. */
#define EVT_DOOR_CLOSED     (1U << 0)

/** Water level sensor has reached the target level. */
#define EVT_WATER_FULL      (1U << 1)

/**
 * A software timer has expired.
 * WashingManager uses this to advance state (e.g., wash complete → drain).
 * The timer callback sets this bit from ISR context via
 * xEventGroupSetBitsFromISR().
 */
#define EVT_TIMEOUT         (1U << 2)

/** Current cycle has completed normally; machine is in FINISH state. */
#define EVT_FINISHED        (1U << 3)

/**
 * A fault has been detected (door opened mid-cycle, fill timeout, etc.).
 * Set by SensorTask or WashingManager; clears only on CMD_RESET.
 */
#define EVT_ERROR           (1U << 4)

/** Convenience mask — all defined event bits. */
#define EVT_ALL_BITS        (EVT_DOOR_CLOSED  | \
                             EVT_WATER_FULL   | \
                             EVT_TIMEOUT      | \
                             EVT_FINISHED     | \
                             EVT_ERROR)

#endif /* APP_COMMON_APP_EVENTS_H */
