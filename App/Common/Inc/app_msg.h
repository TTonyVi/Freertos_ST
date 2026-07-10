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
 * UITask builds this after interpreting a ButtonEvent_t against the
 * current menu cursor position:
 *
 *   cursor on "Wash" + long press  →  { CMD_START_WASH, WASH_MODE_WASH }
 *   cursor on "Spin" + long press  →  { CMD_START_SPIN, WASH_MODE_SPIN }
 *   error screen   + long press  →  { CMD_RESET,      (ignored)       }
 *
 * WashingManager reads this with xQueueReceive() and drives the state
 * machine accordingly.
 * -------------------------------------------------------------------------- */
typedef struct
{
    Command_t  command;    /**< The requested action. */
    WashMode_t mode;       /**< Program to run; only used when command = CMD_START_*. */
    uint8_t    menuIndex;  /**< Cursor position at the time the command was sent. */
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
