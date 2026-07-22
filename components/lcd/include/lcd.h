#pragma once

#include "esp_err.h"

#define LCD_WIDTH   240
#define LCD_HEIGHT  240

esp_err_t lcd_init(void);

esp_err_t lcd_reinit(void);

esp_err_t lcd_fill_color(uint16_t color);

esp_err_t lcd_fill_rect(uint16_t x,
                        uint16_t y,
                        uint16_t w,
                        uint16_t h,
                        uint16_t color);

esp_err_t lcd_draw_pixel(uint16_t x,
                         uint16_t y,
                         uint16_t color);

esp_err_t lcd_draw_bitmap(uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          const void *data);