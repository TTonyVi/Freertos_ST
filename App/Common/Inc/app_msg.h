/**
 * @file    app_msg.h
 * @brief   Inter-task message structures for queue communication.
 *
 * Structs defined here are copied by value into FreeRTOS queues.
 * No pointers to heap memory — safe, deterministic, ISR-friendly.
 *
 * Queues and their message types:
 *
 *   Button Queue   : ButtonEvent_t   (defined in app_types.h — no struct needed)
 *   Command Queue  : CommandMsg_t    (UITask → WashingManager)
 *   Display Queue  : DisplayMsg_t    (WashingManager → OLEDTask)
 */

#ifndef APP_COMMON_APP_MSG_H
#define APP_COMMON_APP_MSG_H

#include <stdint.h>
#include "app_types.h"

/* --------------------------------------------------------------------------
 * CommandMsg_t
 *
 * Sent by UITask to WashingManager via the Command Queue.
 *
 * UITask builds this from a ButtonEvent_t — one gesture, one command, no
 * other information. It deliberately carries NO menu position or program:
 * the cursor lives in WashingManager (its single owner), so there is no
 * second copy that could drift out of step with what the OLED shows.
 * See Command_t in app_types.h for how each command is interpreted.
 *
 * Kept as a struct rather than a bare Command_t so a future command that
 * needs a payload can add a field without changing the queue's item type.
 *
 * WashingManager reads this with xQueueReceive() and drives the state
 * machine accordingly.
 * -------------------------------------------------------------------------- */
typedef struct
{
    Command_t command;    /**< The requested action. */
} CommandMsg_t;

/* --------------------------------------------------------------------------
 * DisplayMsg_t
 *
 * Sent by WashingManager to OLEDTask via the Display Queue.
 *
 * OLEDTask is a pure renderer — it holds no application state.
 * Every field required to draw one complete screen is contained here.
 *
 * Screen layout (SSD1306, 128×64, font 6×8):
 *
 *   Line 0 : "Washing Machine"   (title, always static)
 *   Line 1 : "> Wash"  or  "  Wash"   (menu item 0, cursor driven by menuIndex)
 *   Line 2 : "> Spin"  or  "  Spin"   (menu item 1)
 *   Line 3 : statusText               (state / error message, max 23 chars)
 *
 * WashingManager is the ONLY producer: it alone knows the state, the cursor
 * and the status text, so every message it builds is complete and consistent.
 *
 * statusText is 24 bytes (23 visible chars + null terminator) because
 * the SSD1306 at 6×8 font fits 21 chars per line; 24 bytes gives safe
 * headroom with no dynamic allocation.
 * -------------------------------------------------------------------------- */
typedef struct
{
    WashState_t state;          /**< Current machine state for status rendering. */
    WashMode_t  selectedMode;   /**< Which program is active or selected. */
    uint8_t     menuIndex;      /**< Cursor position: 0 = Wash, 1 = Spin. */
    char        statusText[24]; /**< Null-terminated status string for line 3. */
} DisplayMsg_t;

#endif /* APP_COMMON_APP_MSG_H */
