#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"
#include "hw_def.h"
#include "uart.h"

// Row 핀 정의
#define ROW1_PORT GPIOA
#define ROW1_PIN  GPIO_PIN_3 //Row 1
#define ROW2_PORT GPIOC
#define ROW2_PIN  GPIO_PIN_0 //Row 2
#define ROW3_PORT GPIOC
#define ROW3_PIN  GPIO_PIN_3 //Row 3
#define ROW4_PORT GPIOF
#define ROW4_PIN  GPIO_PIN_3 //Row 4

// Column 핀 정의
#define COL1_PORT GPIOF
#define COL1_PIN  GPIO_PIN_5 // Col 1
#define COL2_PORT GPIOF
#define COL2_PIN  GPIO_PIN_10 // Col 2
#define COL3_PORT GPIOD
#define COL3_PIN  GPIO_PIN_4 // Col 3
#define COL4_PORT GPIOD
#define COL4_PIN  GPIO_PIN_3 // Col 4

typedef struct _keypad_input{
	bool selected_floor[3]; // false true false
	bool door_open_button;   // a 꾹 누르면 true/ 떼면 false
	bool door_close_button;  // b 꾹 누르면 true/ 떼면 false

    int8_t temp_floor;
    int8_t confirmed_floor;

	char last_key;
} keypad_input_t;

void init_keypad_input(keypad_input_t*);
keypad_input_t get_keypad_input(); 

char get_key(void);

#endif