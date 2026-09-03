/**
  ******************************************************************************
  * @file    bsp_lcd.h
  * @brief   SPI TFT LCD driver (soft SPI) - ST7789 / ILI9341 / ST7735 selectable.
  *
  *  Board wiring (Genbotter G474):
  *    LCD_CS  PD11   LCD_SCL PB3   LCD_SDA PB5   LCD_DC PD12   LCD_BL PD13
  *
  *  Controller selected by LCD_CONTROLLER in bsp_board_cfg.h:
  *    0 = ST7789 (default, 240x240)
  *    1 = ILI9341 (240x320)
  *    2 = ST7735  (128x160)
  *
  *  Text UI helpers draw 5x7 ASCII font, one char cell = 6x8 px.
  ******************************************************************************
  */
#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#include <stdint.h>

/* Controller ids */
#define LCD_CTRL_ST7789   0
#define LCD_CTRL_ILI9341  1
#define LCD_CTRL_ST7735   2

/** Init LCD (backlight on, clear to black). Call once after clock setup. */
void LCD_Init(void);

/** Fill whole screen with 16-bit RGB565 color. */
void LCD_Fill(uint16_t color);

/** Clear screen to black. */
void LCD_Clear(void);

/**
  * @brief  Draw an ASCII text line at char-cell coordinates.
  * @param  row  char row (0..LCD_CHAR_ROWS-1)
  * @param  col  char col (0..LCD_CHAR_COLS-1)
  * @param  text  null-terminated ASCII string
  * @param  fg    RGB565 foreground
  * @param  bg    RGB565 background
  */
void LCD_Print(uint8_t row, uint8_t col, const char *text,
               uint16_t fg, uint16_t bg);

/** Screen geometry (depends on controller). */
extern const uint16_t LCD_WIDTH;
extern const uint16_t LCD_HEIGHT;
extern const uint8_t  LCD_CHAR_COLS;
extern const uint8_t  LCD_CHAR_ROWS;

/* Some RGB565 colors */
#define LCD_COLOR_BLACK    0x0000U
#define LCD_COLOR_WHITE    0xFFFFU
#define LCD_COLOR_RED      0xF800U
#define LCD_COLOR_GREEN    0x07E0U
#define LCD_COLOR_BLUE     0x001FU
#define LCD_COLOR_YELLOW   0xFFE0U
#define LCD_COLOR_CYAN     0x07FFU
#define LCD_COLOR_GRAY     0x8410U

#endif /* __BSP_LCD_H */
