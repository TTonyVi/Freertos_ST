/**
 * @file    oled_task.h
 * @brief   OLEDTask — sole owner of the SSD1306 OLED display.
 *
 * Responsibilities:
 *   - Initialises the BSP OLED layer on startup.
 *   - Blocks on xDisplayQueue waiting for DisplayMsg_t from WashingManager.
 *   - On each message: clears framebuffer, draws full screen, flushes to display.
 *   - Never holds application state — every message is self-contained.
 *
 * Screen layout (SSD1306 128x64, font 6x8):
 *   Row 0 : "Washing Machine"         (static title)
 *   Row 1 : "> Wash" or "  Wash"      (cursor driven by msg.menuIndex)
 *   Row 2 : "> Spin" or "  Spin"
 *   Row 3 : msg.statusText            (state / error string)
 *
 * Priority: TASK_PRIORITY_OLED (1) — lowest application priority.
 * No other task may call any BSP_OLED_* function.
 */

#ifndef APP_OLEDTASK_OLED_TASK_H
#define APP_OLEDTASK_OLED_TASK_H

/**
 * @brief  OLEDTask FreeRTOS task function.
 * @param  pvParameters  Unused. Pass NULL to xTaskCreate().
 */
void OLEDTask(void *pvParameters);

#endif /* APP_OLEDTASK_OLED_TASK_H */
