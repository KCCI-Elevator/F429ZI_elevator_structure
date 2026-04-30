#include "bsp.h"
#include "motor.h"//pwn 제
#include "elevator_param.h"
#include "uart.h"

// inner
static bsp_elevator_input_t elevator_input;

static bool bspReadInputPin(uint8_t port_idx, uint8_t pin_num) {
    int8_t pin_state;

    pin_state = gpioExtRead(port_idx, pin_num);

    if (pin_state < 0) {
        return false;
    }

    if (BSP_INPUT_ACTIVE_STATE == HIGH) {
        return pin_state == HIGH;
    }

    return pin_state == LOW;
}

static void bspWriteOutputPin(uint8_t port_idx, uint8_t pin_num, bool state) {
    gpioExtWrite(port_idx, pin_num, state ? HIGH : LOW);
}

static void bspMotorStop(uint8_t in1_port, uint8_t in1_pin, uint8_t in2_port, uint8_t in2_pin) {
    bspWriteOutputPin(in1_port, in1_pin, false);
    bspWriteOutputPin(in2_port, in2_pin, false);
}

static void bspMotorForward(uint8_t in1_port, uint8_t in1_pin, uint8_t in2_port, uint8_t in2_pin) {
    bspWriteOutputPin(in1_port, in1_pin, true);
    bspWriteOutputPin(in2_port, in2_pin, false);
}

static void bspMotorReverse(uint8_t in1_port, uint8_t in1_pin, uint8_t in2_port, uint8_t in2_pin) {
    bspWriteOutputPin(in1_port, in1_pin, false);
    bspWriteOutputPin(in2_port, in2_pin, true);
}

static void bspUpdateFloorSensor(void) {
    uint8_t detected_count = 0;
    uint8_t detected_floor = 0;

    if (bspReadInputPin(BSP_SENSOR_FLOOR_1_PORT, BSP_SENSOR_FLOOR_1_PIN) == true) {
        detected_count++;
        detected_floor = 1;
    }

    if (bspReadInputPin(BSP_SENSOR_FLOOR_2_PORT, BSP_SENSOR_FLOOR_2_PIN) == true) {
        detected_count++;
        detected_floor = 2;
    }

    if (bspReadInputPin(BSP_SENSOR_FLOOR_3_PORT, BSP_SENSOR_FLOOR_3_PIN) == true) {
        detected_count++;
        detected_floor = 3;
    }

    if (detected_count == 1) {
        elevator_input.floor_valid = true;
        elevator_input.curr_floor = detected_floor;
    }
    else {
        elevator_input.floor_valid = false;
    }
}

// function
void bspInit(void) {
    memset(&elevator_input, 0, sizeof(elevator_input));

    hwInit();
    (void)uartInit(); /* RX 큐, TX 뮤텍스, 9600 재설정, 수신 IT 시작 */
    motorInit();    // from motor.c ???
}

void bspUpdate(void) {
    /*
     * request_mask는 매 주기 새로 읽습니다.
     * elevator.c 내부에서 ctx->request_mask |= input.request_mask 형태로 누적하면 됩니다.
     */
    elevator_input.req_mask = 0;

    if (bspReadInputPin(BSP_BTN_FLOOR_1_PORT, BSP_BTN_FLOOR_1_PIN) == true) {
        elevator_input.req_mask |= (1U << 0);
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_2_PORT, BSP_BTN_FLOOR_2_PIN) == true) {
        elevator_input.req_mask |= (1U << 1);
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_3_PORT, BSP_BTN_FLOOR_3_PIN) == true) {
        elevator_input.req_mask |= (1U << 2);
    }

    bspUpdateFloorSensor();

    elevator_input.top_limit =
        bspReadInputPin(BSP_LIMIT_TOP_PORT, BSP_LIMIT_TOP_PIN);

    elevator_input.bottom_limit =
        bspReadInputPin(BSP_LIMIT_BOTTOM_PORT, BSP_LIMIT_BOTTOM_PIN);

    elevator_input.door_open_limit =
        bspReadInputPin(BSP_DOOR_OPEN_LIMIT_PORT, BSP_DOOR_OPEN_LIMIT_PIN);

    elevator_input.door_close_limit =
        bspReadInputPin(BSP_DOOR_CLOSE_LIMIT_PORT, BSP_DOOR_CLOSE_LIMIT_PIN);

    elevator_input.obstacle_detected =
        bspReadInputPin(BSP_DOOR_OBSTACLE_PORT, BSP_DOOR_OBSTACLE_PIN);

    /*
     * 현재 보드에서는 비상정지 입력이 미연결/부동 상태라 항상 활성로 판정될 수 있습니다.
     * 하드웨어 배선 및 active level 확정 전까지는 소프트웨어에서 비활성 처리합니다.
     */
    elevator_input.emergency_stop = false;

    /*
     * 아직 ADC 전류 감지 드라이버가 없으므로 false로 둡니다.
     * 나중에 my_adc.c/h 또는 current_sensor.c/h를 만들면 여기서 갱신하면 됩니다.
     */
    elevator_input.motor_over_current = false;
}

uint32_t bspMillis(void) {
    return hwMillis();
}
void bspDelay(uint32_t delay_ms) {
    hwDelay(delay_ms);
}

void bspElevatorReadInput(bsp_elevator_input_t *input) {
    if (input == NULL) {
        return;
    }

    *input = elevator_input;
}

void bspLiftMotorSet(bsp_lift_dir_t dir, uint16_t pwm) {
    /* motor.c: PF14/15 방향 + TIM4 CH3 PWM(myPwmSetDuty). GPIO만 켜면 드라이버 인가 없음. */
    if (pwm == 0 || dir == BSP_LIFT_STOP) {
        motorStop();
        return;
    }

    switch (dir) {
        case BSP_LIFT_UP:
            motorSetSpeed(MOTOR_DIR_CW, (uint32_t)pwm);
            break;

        case BSP_LIFT_DOWN:
            motorSetSpeed(MOTOR_DIR_CCW, (uint32_t)pwm);
            break;

        case BSP_LIFT_STOP:
        default:
            motorStop();
            break;
    }
}

void bspDoorMotorSet(bsp_door_dir_t dir, uint16_t pwm) {
    /*
     * 현재 버전에서는 PWM 제어가 없습니다.
     * pwm 값은 함수 호환성을 위해 유지합니다.
     */
    if (pwm == 0 || dir == BSP_DOOR_STOP) {
        bspMotorStop(BSP_DOOR_MOTOR_IN1_PORT, BSP_DOOR_MOTOR_IN1_PIN,
                     BSP_DOOR_MOTOR_IN2_PORT, BSP_DOOR_MOTOR_IN2_PIN);
        return;
    }

    switch (dir) {
        case BSP_DOOR_OPEN:
            bspMotorForward(BSP_DOOR_MOTOR_IN1_PORT, BSP_DOOR_MOTOR_IN1_PIN,
                            BSP_DOOR_MOTOR_IN2_PORT, BSP_DOOR_MOTOR_IN2_PIN);
            break;

        case BSP_DOOR_CLOSE:
            bspMotorReverse(BSP_DOOR_MOTOR_IN1_PORT, BSP_DOOR_MOTOR_IN1_PIN,
                            BSP_DOOR_MOTOR_IN2_PORT, BSP_DOOR_MOTOR_IN2_PIN);
            break;

        case BSP_DOOR_STOP:
        default:
            bspMotorStop(BSP_DOOR_MOTOR_IN1_PORT, BSP_DOOR_MOTOR_IN1_PIN,
                         BSP_DOOR_MOTOR_IN2_PORT, BSP_DOOR_MOTOR_IN2_PIN);
            break;
    }
}
