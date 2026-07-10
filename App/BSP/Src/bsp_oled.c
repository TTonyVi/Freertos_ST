/**
 * @file    bsp_oled.c
 * @brief   OLED BSP implementation — SSD1306 over I2C1 (PB8/PB9).
 *
 * Wraps the afiskon/stm32-ssd1306 driver behind three simple primitives.
 * OLEDTask is the only caller — never call these from ISR context.
 */

#include "bsp_oled.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

void BSP_OLED_Init(void)
{
    ssd1306_Init();
    /* Clear display RAM — SSD1306 power-on state is undefined. */
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void BSP_OLED_Clear(void)
{
    ssd1306_Fill(Black);
}

void BSP_OLED_DrawText(uint8_t x, uint8_t y, const char *text)
{
    ssd1306_SetCursor(x, (uint8_t)(y * 8u));
    ssd1306_WriteString((char *)text, Font_6x8, White);
}

void BSP_OLED_Refresh(void)
{
    ssd1306_UpdateScreen();
}
