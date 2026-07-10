/**
 * @file    bsp_oled.h
 * @brief   OLED BSP — SSD1306 display over I2C1 (PB8 SCL / PB9 SDA).
 *
 * Provides primitive drawing operations on a 128×64 pixel SSD1306 display.
 * Knows nothing about WashState_t, DisplayMsg_t, or menu layout —
 * those decisions belong to OLEDTask.
 *
 * Rendering model — double-buffer:
 *   BSP_OLED_Clear() and BSP_OLED_DrawText() write to an internal
 *   framebuffer in RAM. BSP_OLED_Refresh() flushes the complete
 *   framebuffer to the display over I2C in one transfer.
 *   OLEDTask always builds a full frame before calling Refresh().
 *
 * I2C handle convention (per PROJECT_SPEC):
 *   hOled is used instead of hi2c1 when referring to the OLED I2C handle.
 *
 * @note  SSD1306 driver must be added to the project before bsp_oled.c
 *        can be fully implemented. See bsp_oled.c for TODO markers.
 *
 * @note  BSP_OLED_Refresh() is blocking (I2C transfer) and must only
 *        be called from OLEDTask, never from ISR context.
 */

#ifndef APP_BSP_BSP_OLED_H
#define APP_BSP_BSP_OLED_H

#include <stdint.h>

/**
 * @brief  Initialize I2C1 and the SSD1306 driver.
 *         Must be called once before any other BSP_OLED_* function.
 */
void BSP_OLED_Init(void);

/**
 * @brief  Fill the internal framebuffer with zeros (all pixels off).
 *         Does not transmit over I2C — call BSP_OLED_Refresh() to update display.
 */
void BSP_OLED_Clear(void);

/**
 * @brief  Write a null-terminated string into the framebuffer at (x, y).
 *
 * @param  x     Pixel column, 0–127 (left to right).
 * @param  y     Character row, 0–7 (top to bottom, each row = 8 pixels tall).
 * @param  text  Null-terminated string. Text exceeding display width is clipped.
 *
 * Does not transmit over I2C — call BSP_OLED_Refresh() to update display.
 */
void BSP_OLED_DrawText(uint8_t x, uint8_t y, const char *text);

/**
 * @brief  Flush the complete framebuffer to the SSD1306 over I2C.
 *         Blocking — returns only when the I2C transfer is complete.
 */
void BSP_OLED_Refresh(void);

#endif /* APP_BSP_BSP_OLED_H */
