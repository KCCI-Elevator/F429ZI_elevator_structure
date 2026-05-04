#include "bsp.h"

// hardware
#include "hw.h"
#include "my_gpio.h"
#include "my_uart.h"
//#include "motor.h"

/* elevator unit */
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
        elevator_input.current_floor = detected_floor;
    }
    else {
        elevator_input.floor_valid = false;
    }
}

/* wifi unit */
static esp8266_t esp8266;

// function
bool bspInit(void) {
    memset(&elevator_input, 0, sizeof(elevator_input));

    hwInit();
    if (uartInit() == false) return false;

    esp8266_Init(&esp8266, UART_CH_ESP8266);

    return true;
}

void bspUpdate(void) {
    /*
     * request_mask는 매 주기 새로 읽습니다.
     * elevator.c 내부에서 ctx->request_mask |= input.request_mask 형태로 누적하면 됩니다.
     */
    elevator_input.request_mask = 0;

    if (bspReadInputPin(BSP_BTN_FLOOR_1_PORT, BSP_BTN_FLOOR_1_PIN) == true) {
        elevator_input.request_mask |= (1U << 0);
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_2_PORT, BSP_BTN_FLOOR_2_PIN) == true) {
        elevator_input.request_mask |= (1U << 1);
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_3_PORT, BSP_BTN_FLOOR_3_PIN) == true) {
        elevator_input.request_mask |= (1U << 2);
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

    elevator_input.emergency_stop =
        bspReadInputPin(BSP_EMERGENCY_STOP_PORT, BSP_EMERGENCY_STOP_PIN);

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
    /*
     * 현재 버전에서는 PWM 제어가 없습니다.
     * pwm 값은 함수 호환성을 위해 유지합니다.
     */
    if (pwm == 0 || dir == BSP_LIFT_STOP) {
        bspMotorStop(BSP_LIFT_MOTOR_IN1_PORT, BSP_LIFT_MOTOR_IN1_PIN,
                     BSP_LIFT_MOTOR_IN2_PORT, BSP_LIFT_MOTOR_IN2_PIN);
        return;
    }

    switch (dir) {
        case BSP_LIFT_UP:
            bspMotorForward(BSP_LIFT_MOTOR_IN1_PORT, BSP_LIFT_MOTOR_IN1_PIN,
                            BSP_LIFT_MOTOR_IN2_PORT, BSP_LIFT_MOTOR_IN2_PIN);
            break;

        case BSP_LIFT_DOWN:
            bspMotorReverse(BSP_LIFT_MOTOR_IN1_PORT, BSP_LIFT_MOTOR_IN1_PIN,
                            BSP_LIFT_MOTOR_IN2_PORT, BSP_LIFT_MOTOR_IN2_PIN);
            break;

        case BSP_LIFT_STOP:
        default:
            bspMotorStop(BSP_LIFT_MOTOR_IN1_PORT, BSP_LIFT_MOTOR_IN1_PIN,
                         BSP_LIFT_MOTOR_IN2_PORT, BSP_LIFT_MOTOR_IN2_PIN);
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

esp8266_t *bspGetEsp8266(void){
    return &esp8266;
}