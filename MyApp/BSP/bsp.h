#ifndef __MAP_BSP__BSP_H_
#define __MAP_BSP__BSP_H_

// header
#include <string.h>
#include <stddef.h>

#include "def.h"
#include "hw.h"
#include "motor.h"
#include "my_gpio.h"

// define

/*
 * GPIO port index
 *
 * 0  = GPIOA
 * 1  = GPIOB
 * 2  = GPIOC
 * 3  = GPIOD
 * 4  = GPIOE
 * 5  = GPIOF
 * 6  = GPIOG
 * 7  = GPIOH
 * 8  = GPIOI
 * 9  = GPIOJ
 * 10 = GPIOK
 */
 
/*
 * =========================
 * BSP GPIO MAP
 * =========================
 *
 * 아래 포트/핀 번호는 예시입니다.
 * 실제 NUCLEO-F429ZI 배선에 맞게 반드시 수정해야 합니다.
 */

/* Floor request buttons */
#define BSP_BTN_FLOOR_1_PORT        0
#define BSP_BTN_FLOOR_1_PIN         0

#define BSP_BTN_FLOOR_2_PORT        0
#define BSP_BTN_FLOOR_2_PIN         1

#define BSP_BTN_FLOOR_3_PORT        0
#define BSP_BTN_FLOOR_3_PIN         2

/* Floor position sensors */
#define BSP_SENSOR_FLOOR_1_PORT     1
#define BSP_SENSOR_FLOOR_1_PIN      0

#define BSP_SENSOR_FLOOR_2_PORT     1
#define BSP_SENSOR_FLOOR_2_PIN      1

#define BSP_SENSOR_FLOOR_3_PORT     1
#define BSP_SENSOR_FLOOR_3_PIN      2

/* Lift limit switches */
#define BSP_LIMIT_TOP_PORT          2
#define BSP_LIMIT_TOP_PIN           0

#define BSP_LIMIT_BOTTOM_PORT       2
#define BSP_LIMIT_BOTTOM_PIN        1

/* Door limit switches */
#define BSP_DOOR_OPEN_LIMIT_PORT    2
#define BSP_DOOR_OPEN_LIMIT_PIN     2

#define BSP_DOOR_CLOSE_LIMIT_PORT   2
#define BSP_DOOR_CLOSE_LIMIT_PIN    3

/* Door obstacle sensor */
#define BSP_DOOR_OBSTACLE_PORT      2
#define BSP_DOOR_OBSTACLE_PIN       4

/* Emergency stop */
#define BSP_EMERGENCY_STOP_PORT     2
#define BSP_EMERGENCY_STOP_PIN      5

/*
 * Lift motor control pins — motor.c(MOT_PORT/MOT_CW_PIN/MOT_CCW_PIN)와 동일 배선
 *   GPIOF PF14 = IN1 (CW), PF15 = IN2 (CCW)
 *   IN1=1, IN2=0 : UP   (motor.c CW)
 *   IN1=0, IN2=1 : DOWN (motor.c CCW)
 *   IN1=0, IN2=0 : STOP
 */
#define BSP_LIFT_MOTOR_IN1_PORT     5
#define BSP_LIFT_MOTOR_IN1_PIN      14

#define BSP_LIFT_MOTOR_IN2_PORT     5
#define BSP_LIFT_MOTOR_IN2_PIN      15

/*
 * Door motor control pins
 *
 * IN1 = 1, IN2 = 0 : OPEN
 * IN1 = 0, IN2 = 1 : CLOSE
 * IN1 = 0, IN2 = 0 : STOP
 */
#define BSP_DOOR_MOTOR_IN1_PORT     3
#define BSP_DOOR_MOTOR_IN1_PIN      2

#define BSP_DOOR_MOTOR_IN2_PORT     3
#define BSP_DOOR_MOTOR_IN2_PIN      3

/*
 * 입력 신호 기준
 *
 * 버튼, 리미트 스위치, 센서를 누르거나 감지했을 때 HIGH가 들어오는 구조라면 1로 둡니다.
 * 풀업 입력이라서 눌렀을 때 LOW가 되는 구조라면 0으로 바꾸면 됩니다.
 */
#define BSP_INPUT_ACTIVE_STATE      HIGH

// typedef
typedef enum {
    BSP_LIFT_STOP = 0,
    BSP_LIFT_UP,
    BSP_LIFT_DOWN
} bsp_lift_dir_t;

typedef enum {
    BSP_DOOR_STOP = 0,
    BSP_DOOR_OPEN,
    BSP_DOOR_CLOSE
} bsp_door_dir_t;

typedef struct {
    bool floor_valid;
    uint8_t curr_floor;

    uint8_t req_mask;

    bool top_limit;
    bool bottom_limit;

    bool door_open_limit;
    bool door_close_limit;
    bool obstacle_detected;

    bool emergency_stop;
    bool motor_over_current;
} bsp_elevator_input_t;

// function
void bspInit(void);
void bspUpdate(void);

uint32_t bspMillis(void);
void bspDelay(uint32_t delay_ms);

void bspElevatorReadInput(bsp_elevator_input_t *input);

void bspLiftMotorSet(bsp_lift_dir_t dir, uint16_t pwm);
void bspDoorMotorSet(bsp_door_dir_t dir, uint16_t pwm);

#endif //__MAP_BSP__BSP_H_