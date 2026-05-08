#include "keypad.h"

#include "hw.h"
#include "my_gpio.h"

#define KEYPAD_ROW_COUNT 4U
#define KEYPAD_COL_COUNT 4U

/* Port index: 0=A, 1=B, ... , 10=K */
typedef struct {
    uint8_t port;
    uint8_t pin;
} keypad_pin_t;

static const keypad_pin_t s_rows[KEYPAD_ROW_COUNT] = {
    {0, 3},   /* PA3 */
    {2, 0},   /* PC0 */
    {2, 3},   /* PC3 */
    {5, 3}    /* PF3 */
};

static const keypad_pin_t s_cols[KEYPAD_COL_COUNT] = {
    {5, 5},   /* PF5 */
    {5, 10},  /* PF10 */
    {3, 4},   /* PD4 */
    {3, 3}    /* PD3 */
};

static const char s_key_map[KEYPAD_ROW_COUNT][KEYPAD_COL_COUNT] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static void keypadSetAllRows(bool state)
{
    for (uint8_t i = 0; i < KEYPAD_ROW_COUNT; i++) {
        (void)gpioExtWrite(s_rows[i].port, s_rows[i].pin, state ? HIGH : LOW);
    }
}

void keypadInit(void)
{
    for (uint8_t i = 0; i < KEYPAD_ROW_COUNT; i++) {
        (void)gpioExtInitPull(s_rows[i].port, s_rows[i].pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
        (void)gpioExtWrite(s_rows[i].port, s_rows[i].pin, HIGH);
    }

    for (uint8_t i = 0; i < KEYPAD_COL_COUNT; i++) {
        (void)gpioExtInitPull(s_cols[i].port, s_cols[i].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
    }
}

char keypadGetKey(void)
{
    for (uint8_t row = 0; row < KEYPAD_ROW_COUNT; row++) {
        keypadSetAllRows(true);
        (void)gpioExtWrite(s_rows[row].port, s_rows[row].pin, LOW);

        for (uint8_t col = 0; col < KEYPAD_COL_COUNT; col++) {
            if (gpioExtRead(s_cols[col].port, s_cols[col].pin) == LOW) {
                hwDelay(20);

                if (gpioExtRead(s_cols[col].port, s_cols[col].pin) == LOW) {
                    keypadSetAllRows(true);
                    return s_key_map[row][col];
                }
            }
        }
    }

    keypadSetAllRows(true);
    return 0;
}
