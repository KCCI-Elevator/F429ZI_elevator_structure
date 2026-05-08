#ifndef __MAP_DRIVER__OLED_SPI_H__
#define __MAP_DRIVER__OLED_SPI_H__

#include "def.h"

void oled_init(void);
void oled_clear(void);
void oled_refresh(void);
void oled_write_byte(uint8_t dat, uint8_t is_data);

void oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void oled_show_scaled_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t scale, uint8_t color);
void oled_show_scaled_string(uint8_t x, uint8_t y, const char *str, uint8_t scale, uint8_t color);

void f429_oled_ui_update(uint8_t floor, uint8_t lift_dir, uint8_t special_state, const bool *call_car);

#endif // __MAP_DRIVER__OLED_SPI_H__
