//
// Created by hiimseoll on 26. 5. 4..
//

#ifndef F429ZI_ELEVATOR_STRUCTURE_OLED_SPI_H
#define F429ZI_ELEVATOR_STRUCTURE_OLED_SPI_H

#include "stm32f4xx_hal.h"
#include "def.h"
#include "font.h"
#include <stdbool.h>

// ====================================================================
// [소프트웨어 SPI] 핀 제어 매크로 (CubeMX User Label 연동)
// ====================================================================

// 클럭 (SCK) - PE2
#define OLED_SCK_SET()   HAL_GPIO_WritePin(OLED_SCK_GPIO_Port, OLED_SCK_Pin, GPIO_PIN_SET)
#define OLED_SCK_CLR()   HAL_GPIO_WritePin(OLED_SCK_GPIO_Port, OLED_SCK_Pin, GPIO_PIN_RESET)

// 데이터 (MOSI) - PE6
#define OLED_MOSI_SET()  HAL_GPIO_WritePin(OLED_MOSI_GPIO_Port, OLED_MOSI_Pin, GPIO_PIN_SET)
#define OLED_MOSI_CLR()  HAL_GPIO_WritePin(OLED_MOSI_GPIO_Port, OLED_MOSI_Pin, GPIO_PIN_RESET)

// 리셋 (RES) - PE5
#define OLED_RES_SET()   HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET)
#define OLED_RES_CLR()   HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_RESET)

// 데이터/명령 선택 (DC) - PE3
#define OLED_DC_SET()    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET)
#define OLED_DC_CLR()    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET)

// 칩 선택 (CS) - PE4
#define OLED_CS_SET()    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET)
#define OLED_CS_CLR()    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET)

// ====================================================================
// 함수 원형
// ====================================================================
void oled_init(void);
void oled_clear(void);
void oled_refresh(void);
void oled_write_byte(uint8_t dat, uint8_t is_data);

void oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void oled_show_scaled_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t scale, uint8_t color);
void oled_show_scaled_string(uint8_t x, uint8_t y, char *str, uint8_t scale, uint8_t color);

void f429_oled_ui_update(uint8_t floor, uint8_t lift_dir, uint8_t special_state, bool *call_car);

#endif // F429ZI_ELEVATOR_STRUCTURE_OLED_SPI_H
