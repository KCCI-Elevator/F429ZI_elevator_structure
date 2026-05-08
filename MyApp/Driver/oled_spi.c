//
// Created by hiimseoll on 26. 5. 4..
//

#include "oled_spi.h"

#include <string.h>
#include <stdlib.h>

#include "font.h"
#include "hw.h"
#include "my_gpio.h"

#define OLED_SCK_SET()   gpioPinWrite(OLED_SCK_GPIO_Port, OLED_SCK_Pin, true)
#define OLED_SCK_CLR()   gpioPinWrite(OLED_SCK_GPIO_Port, OLED_SCK_Pin, false)
#define OLED_MOSI_SET()  gpioPinWrite(OLED_MOSI_GPIO_Port, OLED_MOSI_Pin, true)
#define OLED_MOSI_CLR()  gpioPinWrite(OLED_MOSI_GPIO_Port, OLED_MOSI_Pin, false)
#define OLED_RES_SET()   gpioPinWrite(OLED_RES_GPIO_Port, OLED_RES_Pin, true)
#define OLED_RES_CLR()   gpioPinWrite(OLED_RES_GPIO_Port, OLED_RES_Pin, false)
#define OLED_DC_SET()    gpioPinWrite(OLED_DC_GPIO_Port, OLED_DC_Pin, true)
#define OLED_DC_CLR()    gpioPinWrite(OLED_DC_GPIO_Port, OLED_DC_Pin, false)
#define OLED_CS_SET()    gpioPinWrite(OLED_CS_GPIO_Port, OLED_CS_Pin, true)
#define OLED_CS_CLR()    gpioPinWrite(OLED_CS_GPIO_Port, OLED_CS_Pin, false)


static uint8_t oled_buffer[1024];

// =========================================================
// [핵심] 소프트웨어 SPI 바이트 전송 (비트뱅잉)
// =========================================================
void oled_write_byte(uint8_t dat, uint8_t is_data) {
    if (is_data) OLED_DC_SET();
    else OLED_DC_CLR();

    OLED_CS_CLR();

    for (uint8_t i = 0; i < 8; i++) {
        OLED_SCK_CLR(); // 클럭 LOW

        if (dat & 0x80) OLED_MOSI_SET(); // MSB가 1이면 HIGH
        else OLED_MOSI_CLR();            // 0이면 LOW

        OLED_SCK_SET(); // 클럭 HIGH (데이터 밀어넣기)
        dat <<= 1;
    }

    OLED_CS_SET();
}

void oled_init(void) {
    // FreeRTOS 환경이므로 osDelay 사용
    hwDelay(200);

    OLED_RES_CLR();
    hwDelay(100);
    OLED_RES_SET();
    hwDelay(100);

    uint8_t cmds[] = {
        0xAE, 0x20, 0x02, 0x81, 0xCF, 0xA1, 0xC8, 0xA8, 0x3F,
        0xD3, 0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB, 0x40,
        0x8D, 0x14, 0xA4, 0xA6, 0xAF, 0xA0, 0xC0
    };

    for (uint32_t i = 0; i < sizeof(cmds); i++) {
        oled_write_byte(cmds[i], 0);
    }

    oled_clear();
    oled_refresh();
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

// 화면 전체 갱신 (소프트웨어 방식)
void oled_refresh(void) {
    for (uint8_t i = 0; i < 8; i++) {
        oled_write_byte(0xB0 + i, 0);
        oled_write_byte(0x00, 0);
        oled_write_byte(0x10, 0);

        OLED_CS_CLR();
        OLED_DC_SET();

        for (uint8_t j = 0; j < 128; j++) {
            uint8_t dat = oled_buffer[128 * i + j];
            for (uint8_t k = 0; k < 8; k++) {
                OLED_SCK_CLR();
                if (dat & 0x80) OLED_MOSI_SET();
                else OLED_MOSI_CLR();
                OLED_SCK_SET();
                dat <<= 1;
            }
        }
        OLED_CS_SET();
    }
}

// =========================================================
// 세로 모드(64x128) 지원 픽셀 매핑
// =========================================================
void oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x > 63 || y > 127) return;
    uint8_t phys_x = 127 - y;
    uint8_t phys_y = x;
    if (color) oled_buffer[phys_x + (phys_y / 8) * 128] |= (1 << (phys_y % 8));
    else       oled_buffer[phys_x + (phys_y / 8) * 128] &= ~(1 << (phys_y % 8));
}

void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        oled_draw_pixel(x1, y1, 1);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    (void)color;
    oled_draw_line(x, y, x + w - 1, y);
    oled_draw_line(x, y + h - 1, x + w - 1, y + h - 1);
    oled_draw_line(x, y, x, y + h - 1);
    oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1);
}

void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t i = x; i < x + w; i++) {
        for (uint8_t j = y; j < y + h; j++) oled_draw_pixel(i, j, color);
    }
}

void oled_show_scaled_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t scale, uint8_t color) {
    uint16_t font_ptr = chr * 5;
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t col = font[font_ptr + i];
        for (uint8_t j = 0; j < 8; j++) {
            if (col & (1 << j)) {
                for (uint8_t sx = 0; sx < scale; sx++) {
                    for (uint8_t sy = 0; sy < scale; sy++) oled_draw_pixel(x + (i * scale) + sx, y + (j * scale) + sy, color);
                }
            }
        }
    }
}

void oled_show_scaled_string(uint8_t x, uint8_t y, const char *str, uint8_t scale, uint8_t color) {
    while (*str) {
        oled_show_scaled_char(x, y, *str, scale, color);
        x += (6 * scale);
        if (x > 63) { x = 0; y += (8 * scale); }
        str++;
    }
}

// =========================================================
// UI 업데이트 함수 (애니메이션 포함)
// =========================================================
static uint32_t last_anim_tick = 0;
static uint8_t anim_frame = 0;
#define ANIM_SPEED_MS  100

static void draw_up_chevron(uint8_t x, uint8_t y) {
    oled_draw_line(x, y, x - 6, y + 6);
    oled_draw_line(x, y, x + 6, y + 6);
    oled_draw_line(x, y + 1, x - 6, y + 7);
    oled_draw_line(x, y + 1, x + 6, y + 7);
}

static void draw_down_chevron(uint8_t x, uint8_t y) {
    oled_draw_line(x, y + 6, x - 6, y);
    oled_draw_line(x, y + 6, x + 6, y);
    oled_draw_line(x, y + 5, x - 6, y - 1);
    oled_draw_line(x, y + 5, x + 6, y - 1);
}

void f429_oled_ui_update(uint8_t floor, uint8_t lift_dir, uint8_t special_state, const bool *call_car) {
    oled_clear();
    oled_draw_line(40, 0, 40, 52);
    oled_draw_line(0, 52, 63, 52);
    oled_draw_line(0, 72, 63, 72);
    oled_show_scaled_char(6, 3, '0' + floor, 6, 1);

    uint8_t cx = 52;
    bool is_moving = (special_state == 2U);

    if (is_moving && lift_dir != 0U) {
        uint32_t current_tick = hwMillis();
        if (current_tick - last_anim_tick >= ANIM_SPEED_MS) {
            last_anim_tick = current_tick;
            anim_frame = (anim_frame + 1) % 4;
        }
    } else {
        anim_frame = 3;
    }

    if (lift_dir == 1U) {
        for (int i = 0; i < 3; i++) {
            if (i < ((anim_frame == 3) ? 3 : (anim_frame + 1))) draw_up_chevron(cx, 38 - (i * 14));
        }
    }
    else if (lift_dir == 2U) {
        for (int i = 0; i < 3; i++) {
            if (i < ((anim_frame == 3) ? 3 : (anim_frame + 1))) draw_down_chevron(cx, 10 + (i * 14));
        }
    }

    if (special_state == 1U) oled_show_scaled_string(8, 55, "INSP", 2, 1);
    else if (special_state == 2U) oled_show_scaled_string(8, 55, "MOVE", 2, 1);
    else if (special_state == 3U) oled_show_scaled_string(8, 55, "NORM", 2, 1);
    else oled_show_scaled_string(8, 55, "STOP", 2, 1);

    for (int i = 1; i <= 3; i++) {
        uint8_t bx = 4 + (i - 1) * 20;
        uint8_t by = 80;
        if (call_car[i]) {
            oled_fill_rect(bx, by, 16, 24, 1);
            oled_show_scaled_char(bx + 3, by + 4, '0' + i, 2, 0);
        } else {
            oled_draw_rect(bx, by, 16, 24, 1);
            oled_show_scaled_char(bx + 3, by + 4, '0' + i, 2, 1);
        }
    }
    oled_refresh();
}