/**
 * @file    ssd1306_conf.h
 * @brief   SSD1306 driver configuration for STM32F411E-DISCO.
 *
 * Communication : I2C1 (PB8 SCL / PB9 SDA)
 * I2C handle    : hi2c1 (CubeMX default name)
 * Display size  : 128 x 64 pixels
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

/* Select communication protocol */
#define SSD1306_USE_I2C

/* I2C handle — must match CubeMX-generated name */
#define SSD1306_I2C_PORT    hi2c1

/* Display dimensions */
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64

/* Fonts to include */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10

#endif /* __SSD1306_CONF_H__ */
