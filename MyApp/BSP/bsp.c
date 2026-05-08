#include "bsp.h"

#include <string.h>

#include "hw.h"
#include "my_gpio.h"
#include "my_can.h"

#include "motor.h"
#include "keypad.h"
#include "oled_spi.h"
#include "wifi.h"

#ifdef MCU_F429

static bsp_elevator_input_t s_elevator_input;
osMutexId_t i2cMutexHandle = NULL;

/* Port index: 0=A, 1=B, ... , 10=K */
#define BSP_BTN_FLOOR_1_PORT        0U
#define BSP_BTN_FLOOR_1_PIN         0U
#define BSP_BTN_FLOOR_2_PORT        0U
#define BSP_BTN_FLOOR_2_PIN         1U
#define BSP_BTN_FLOOR_3_PORT        0U
#define BSP_BTN_FLOOR_3_PIN         2U

#define BSP_SENSOR_FLOOR_1_PORT     1U
#define BSP_SENSOR_FLOOR_1_PIN      0U
#define BSP_SENSOR_FLOOR_2_PORT     1U
#define BSP_SENSOR_FLOOR_2_PIN      1U
#define BSP_SENSOR_FLOOR_3_PORT     1U
#define BSP_SENSOR_FLOOR_3_PIN      2U

#define BSP_LIMIT_TOP_PORT          2U
#define BSP_LIMIT_TOP_PIN           0U
#define BSP_LIMIT_BOTTOM_PORT       2U
#define BSP_LIMIT_BOTTOM_PIN        1U
#define BSP_DOOR_OPEN_LIMIT_PORT    2U
#define BSP_DOOR_OPEN_LIMIT_PIN     2U
#define BSP_DOOR_CLOSE_LIMIT_PORT   2U
#define BSP_DOOR_CLOSE_LIMIT_PIN    3U
#define BSP_DOOR_OBSTACLE_PORT      2U
#define BSP_DOOR_OBSTACLE_PIN       4U
#define BSP_EMERGENCY_STOP_PORT     2U
#define BSP_EMERGENCY_STOP_PIN      5U

#define BSP_CAN_LED_PORT            1U
#define BSP_CAN_LED_PIN             7U

#define BSP_INPUT_ACTIVE_STATE      HIGH
#define BSP_INPUT_PULL              GPIO_NOPULL

#define BSP_WIFI_TX_PERIOD_MS       2000U
#define BSP_WIFI_RX_PERIOD_MS       1500U

static uint32_t s_wifi_tx_prev_time = 0;
static uint32_t s_wifi_rx_prev_time = 0;

//과전류 디바운스 설정(기동전류땜에 튀는거 무시)
#define CURRENT_DEBOUNCE_THRESHOLD 15U //연속 15회 이상일 때만 확정
#define CURRENT_NOISE_FREE_COUNT   5U //연속 5회 이상 낮아야 정상으로 복구

//전류 쓰레숄드
static float bsp_current_threshold =500.0f;//일단 500으로 설정.
static uint16_t bsp_debounce_cnt = 0;
static bool bsp_is_overload_confirmed = false;

/**
 * @brief  과전류 판단 기준값(Threshold)을 설정합니다.
 * @param  threshold_ma: 설정할 전류 기준치 (mA)
 */
void bspSetCurrentThreshold(float threshold_ma) {
  bsp_current_threshold = threshold_ma;
}

/**
 * @brief  현재 읽은 전류값을 바탕으로 과전류 여부를 판단하여 결과를 반환합니다.
 * @param  current_ma: 실시간으로 읽어온 전류값 (mA)(절댓값)
 * @return true(과전류), false(정상)
 */
bool bspCheckOverCurrent(float current_ma) {
  float abs_current = fabsf(current_ma);

  if (abs_current >= bsp_current_threshold) {
    if (bsp_debounce_cnt < CURRENT_DEBOUNCE_THRESHOLD) {
      bsp_debounce_cnt++;
    }
    // 카운트가 임계치에 도달하면 과전류 확정
    if (bsp_debounce_cnt >= CURRENT_DEBOUNCE_THRESHOLD) {
      bsp_is_overload_confirmed = true;
    }
  }
  else {
    // 기준치 미달 시 카운트를 깎음 (서서히 복구)
    if (bsp_debounce_cnt > 0) {
      bsp_debounce_cnt--;
    } else {
      bsp_is_overload_confirmed = false;
    }
  }

  // 구조체 업데이트 및 리턴
  s_elevator_input.emergency_stop = bsp_is_overload_confirmed;
  s_elevator_input.motor_over_current = bsp_is_overload_confirmed;
  return s_elevator_input.motor_over_current;
}

static bool bspIsValidFloor(uint8_t floor)
{
    return floor >= CAN_FLOOR_1 && floor <= CAN_FLOOR_3;
}

static bool bspReadInputPin(uint8_t port_idx, uint8_t pin_num)
{
    int8_t pin_state = gpioExtRead(port_idx, pin_num);

    if (pin_state < 0) {
        return false;
    }

    return (BSP_INPUT_ACTIVE_STATE == HIGH) ? (pin_state == HIGH) : (pin_state == LOW);
}

static void bspWriteOutputPin(uint8_t port_idx, uint8_t pin_num, bool state)
{
    (void)gpioExtWrite(port_idx, pin_num, state ? HIGH : LOW);
}

static void bspInitInput(uint8_t port_idx, uint8_t pin_num)
{
    (void)gpioExtInitPull(port_idx, pin_num, GPIO_MODE_INPUT, BSP_INPUT_PULL);
}

static void bspInitOutput(uint8_t port_idx, uint8_t pin_num)
{
    (void)gpioExtInitPull(port_idx, pin_num, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    bspWriteOutputPin(port_idx, pin_num, false);
}

static void bspInitIo(void)
{
    bspInitInput(BSP_BTN_FLOOR_1_PORT, BSP_BTN_FLOOR_1_PIN);
    bspInitInput(BSP_BTN_FLOOR_2_PORT, BSP_BTN_FLOOR_2_PIN);
    bspInitInput(BSP_BTN_FLOOR_3_PORT, BSP_BTN_FLOOR_3_PIN);

    bspInitInput(BSP_SENSOR_FLOOR_1_PORT, BSP_SENSOR_FLOOR_1_PIN);
    bspInitInput(BSP_SENSOR_FLOOR_2_PORT, BSP_SENSOR_FLOOR_2_PIN);
    bspInitInput(BSP_SENSOR_FLOOR_3_PORT, BSP_SENSOR_FLOOR_3_PIN);

    bspInitInput(BSP_LIMIT_TOP_PORT, BSP_LIMIT_TOP_PIN);
    bspInitInput(BSP_LIMIT_BOTTOM_PORT, BSP_LIMIT_BOTTOM_PIN);
    bspInitInput(BSP_DOOR_OPEN_LIMIT_PORT, BSP_DOOR_OPEN_LIMIT_PIN);
    bspInitInput(BSP_DOOR_CLOSE_LIMIT_PORT, BSP_DOOR_CLOSE_LIMIT_PIN);
    bspInitInput(BSP_DOOR_OBSTACLE_PORT, BSP_DOOR_OBSTACLE_PIN);
    bspInitInput(BSP_EMERGENCY_STOP_PORT, BSP_EMERGENCY_STOP_PIN);

    bspInitOutput(BSP_CAN_LED_PORT, BSP_CAN_LED_PIN);
}

static void bspUpdateFloorSensor(void)
{
    uint8_t detected_count = 0;
    uint8_t detected_floor = 0;

    if (bspReadInputPin(BSP_SENSOR_FLOOR_1_PORT, BSP_SENSOR_FLOOR_1_PIN)) {
        detected_count++;
        detected_floor = 1;
    }

    if (bspReadInputPin(BSP_SENSOR_FLOOR_2_PORT, BSP_SENSOR_FLOOR_2_PIN)) {
        detected_count++;
        detected_floor = 2;
    }

    if (bspReadInputPin(BSP_SENSOR_FLOOR_3_PORT, BSP_SENSOR_FLOOR_3_PIN)) {
        detected_count++;
        detected_floor = 3;
    }

    if (detected_count == 1U) {
        s_elevator_input.floor_valid = true;
        s_elevator_input.curr_floor = detected_floor;
    }
    else {
        s_elevator_input.floor_valid = false;
    }
}

void bspInit(void)
{
    memset(&s_elevator_input, 0, sizeof(s_elevator_input));

    s_elevator_input.curr_floor = 1;
    s_elevator_input.current_dir = BSP_LIFT_STOP;
    s_elevator_input.special_state = BSP_STATE_NORMAL;
    s_elevator_input.floor_valid = true;

    hwInit();
    
    bspInitIo();
    keypadInit();
    motorInit();
}

void bspUpdate(void)
{
    s_elevator_input.req_mask = 0;

    if (bspReadInputPin(BSP_BTN_FLOOR_1_PORT, BSP_BTN_FLOOR_1_PIN)) {
        s_elevator_input.req_mask |= (1U << 0);
        s_elevator_input.call_car[1] = true;
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_2_PORT, BSP_BTN_FLOOR_2_PIN)) {
        s_elevator_input.req_mask |= (1U << 1);
        s_elevator_input.call_car[2] = true;
    }

    if (bspReadInputPin(BSP_BTN_FLOOR_3_PORT, BSP_BTN_FLOOR_3_PIN)) {
        s_elevator_input.req_mask |= (1U << 2);
        s_elevator_input.call_car[3] = true;
    }

    bspUpdateFloorSensor();

    s_elevator_input.top_limit = bspReadInputPin(BSP_LIMIT_TOP_PORT, BSP_LIMIT_TOP_PIN);
    s_elevator_input.bottom_limit = bspReadInputPin(BSP_LIMIT_BOTTOM_PORT, BSP_LIMIT_BOTTOM_PIN);
    s_elevator_input.door_open_limit = bspReadInputPin(BSP_DOOR_OPEN_LIMIT_PORT, BSP_DOOR_OPEN_LIMIT_PIN);
    s_elevator_input.door_close_limit = bspReadInputPin(BSP_DOOR_CLOSE_LIMIT_PORT, BSP_DOOR_CLOSE_LIMIT_PIN);
    s_elevator_input.obstacle_detected = bspReadInputPin(BSP_DOOR_OBSTACLE_PORT, BSP_DOOR_OBSTACLE_PIN);
    s_elevator_input.emergency_stop = bspReadInputPin(BSP_EMERGENCY_STOP_PORT, BSP_EMERGENCY_STOP_PIN);
    s_elevator_input.motor_over_current = false;
}

uint32_t bspMillis(void)
{
    return hwMillis();
}

void bspDelay(uint32_t delay_ms)
{
    hwDelay(delay_ms);
}

void bspElevatorReadInput(bsp_elevator_input_t *input)
{
    if (input == NULL) {
        return;
    }

    __disable_irq();
    memcpy(input, &s_elevator_input, sizeof(*input));
    __enable_irq();
}

void bspSetCurrentFloor(uint8_t floor)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.curr_floor = floor;
        s_elevator_input.floor_valid = true;
    }
}

void bspSetFloorValid(bool valid)
{
    s_elevator_input.floor_valid = valid;
}

void bspSetCurrentDir(bsp_lift_dir_t dir)
{
    s_elevator_input.current_dir = dir;
}

void bspSetSpecialState(bsp_special_state_t state)
{
    s_elevator_input.special_state = state;
}

void bspSetArrivedAck(bool ack)
{
    s_elevator_input.arrived_ack = ack;
}

void bspSetCarCall(uint8_t floor, bool state)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.call_car[floor] = state;
    }
}

void bspToggleCarCall(uint8_t floor)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.call_car[floor] = !s_elevator_input.call_car[floor];
    }
}

void bspSetHallCallUp(uint8_t floor, bool state)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.call_up[floor] = state;
    }
}

void bspSetHallCallDown(uint8_t floor, bool state)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.call_down[floor] = state;
    }
}

void bspSetWifiRequest(uint8_t floor, bool state)
{
    if (bspIsValidFloor(floor)) {
        s_elevator_input.req_wifi[floor - 1U] = state;
    }
}

void bspClearServicedRequests(uint8_t floor, bsp_lift_dir_t service_dir)
{
    if (bspIsValidFloor(floor) == false) {
        return;
    }

    s_elevator_input.call_car[floor] = false;
    s_elevator_input.req_wifi[floor - 1U] = false;

    if (service_dir == BSP_LIFT_UP) {
        s_elevator_input.call_up[floor] = false;
    }
    else if (service_dir == BSP_LIFT_DOWN) {
        s_elevator_input.call_down[floor] = false;
    }
}

void bspLiftMotorSet(bsp_lift_dir_t dir, uint16_t pwm)
{
    MotorDir_t motor_dir = MOTOR_DIR_STOP;

    if (dir == BSP_LIFT_UP) {
        motor_dir = MOTOR_DIR_CW;
    }
    else if (dir == BSP_LIFT_DOWN) {
        motor_dir = MOTOR_DIR_CCW;
    }

    if (pwm == 0U || motor_dir == MOTOR_DIR_STOP) {
        motorStop();
    }
    else {
        motorSetSpeed(motor_dir, pwm);
    }
}

void bspDoorMotorSet(bsp_door_dir_t dir, uint16_t pwm)
{
    (void)dir;
    (void)pwm;
    /* F429 does not drive the hall door motor directly. Door commands are sent over CAN. */
}

bool bspCanInit(void)
{
    return canInit();
}

bool bspCanSendStatus(void)
{
    const uint8_t data[3] = {
        s_elevator_input.curr_floor,
        (uint8_t)s_elevator_input.current_dir,
        (uint8_t)s_elevator_input.special_state
    };

    return canTransmit(CAN_ID_STATUS, data, 3);
}

bool bspCanSendArrivedCheck(uint8_t floor)
{
    const uint8_t data[1] = {floor};
    return canTransmit(CAN_ID_ARRIVED_CHECK, data, 1);
}

bool bspCanSendDoorCmd(uint8_t floor, uint8_t cmd, uint8_t dir)
{
    const uint8_t data[3] = {floor, cmd, dir};
    return canTransmit(CAN_ID_DOOR_CMD, data, 3);
}

void bspCanSendTest(void)
{
    canTestTx();
}

static void bspCanParseFrame(const can_frame_t *frame)
{
    uint8_t floor;

    if (frame == NULL || frame->len == 0U) {
        return;
    }

    floor = frame->data[0];

    if (bspIsValidFloor(floor) == false) {
        return;
    }

    switch (frame->id) {
    case CAN_ID_CALL_REQ:
        if (frame->len >= 2U) {
            if (frame->data[1] == CAN_CALL_UP) {
                bspSetHallCallUp(floor, true);
            }
            else if (frame->data[1] == CAN_CALL_DOWN) {
                bspSetHallCallDown(floor, true);
            }
        }
        break;

    case CAN_ID_CALL_CANCEL:
        if (frame->len >= 2U) {
            if (frame->data[1] == CAN_CALL_UP) {
                bspSetHallCallUp(floor, false);
            }
            else if (frame->data[1] == CAN_CALL_DOWN) {
                bspSetHallCallDown(floor, false);
            }
        }
        break;

    case CAN_ID_ARRIVED_ACK:
        if (floor == s_elevator_input.curr_floor) {
            bspSetArrivedAck(true);
        }
        break;

    default:
        break;
    }
}

bool bspCanProcessRx(void)
{
    can_frame_t frame;

    if (canReceive(&frame) == false) {
        return false;
    }

    bspWriteOutputPin(BSP_CAN_LED_PORT, BSP_CAN_LED_PIN, true);
    bspDelay(30);
    bspWriteOutputPin(BSP_CAN_LED_PORT, BSP_CAN_LED_PIN, false);

    bspCanParseFrame(&frame);

    return true;
}

void bspUiInit(void)
{
    oled_init();
}

void bspUiUpdate(void)
{
    f429_oled_ui_update(s_elevator_input.curr_floor,
                        (uint8_t)s_elevator_input.current_dir,
                        (uint8_t)s_elevator_input.special_state,
                        s_elevator_input.call_car);
}

char bspKeypadGetKey(void)
{
    return keypadGetKey();
}

bool bspWifiInit(void)
{
    return wifiDefaultInit();
}

void bspWifiProcess(void)
{
    wifi_t *wifi = wifiGetDefaultContext();
    uint32_t now;

    if (wifi == NULL) {
        return;
    }

    wifiProcess(wifi);

    if (wifiIsConnected(wifi) == 0U) {
        return;
    }

    now = bspMillis();

    if ((now - s_wifi_tx_prev_time) >= BSP_WIFI_TX_PERIOD_MS) {
        s_wifi_tx_prev_time = now;
        wifiSendElevatorStatus(wifi, s_elevator_input.curr_floor);
    }

    if ((now - s_wifi_rx_prev_time) >= BSP_WIFI_RX_PERIOD_MS) {
        int v1 = -1;
        int v2 = -1;
        int v3 = -1;

        s_wifi_rx_prev_time = now;

        if (wifiReceiveCommands(wifi, &v1, &v2, &v3) == 0) {
            if (v1 == 0 || v1 == 1) {
                bspSetWifiRequest(1, v1 == 1);
                if (v1 == 1) {
                    wifiClearCommand(wifi, 1);
                }
            }

            if (v2 == 0 || v2 == 1) {
                bspSetWifiRequest(2, v2 == 1);
                if (v2 == 1) {
                    wifiClearCommand(wifi, 2);
                }
            }

            if (v3 == 0 || v3 == 1) {
                bspSetWifiRequest(3, v3 == 1);
                if (v3 == 1) {
                    wifiClearCommand(wifi, 3);
                }
            }
        }
    }
}

#endif // MCU_F429

#ifdef MCU_BLUEPILL
/* BluePill-specific BSP code can stay here. This project zip is primarily F429-oriented. */
void bspInit(void) {}
void bspUpdate(void) {}
uint32_t bspMillis(void) { return hwMillis(); }
void bspDelay(uint32_t delay_ms) { hwDelay(delay_ms); }
void bspElevatorReadInput(bsp_elevator_input_t *input) { (void)input; }
void bspSetCurrentFloor(uint8_t floor) { (void)floor; }
void bspSetFloorValid(bool valid) { (void)valid; }
void bspSetCurrentDir(bsp_lift_dir_t dir) { (void)dir; }
void bspSetSpecialState(bsp_special_state_t state) { (void)state; }
void bspSetArrivedAck(bool ack) { (void)ack; }
void bspSetCarCall(uint8_t floor, bool state) { (void)floor; (void)state; }
void bspToggleCarCall(uint8_t floor) { (void)floor; }
void bspSetHallCallUp(uint8_t floor, bool state) { (void)floor; (void)state; }
void bspSetHallCallDown(uint8_t floor, bool state) { (void)floor; (void)state; }
void bspSetWifiRequest(uint8_t floor, bool state) { (void)floor; (void)state; }
void bspClearServicedRequests(uint8_t floor, bsp_lift_dir_t service_dir) { (void)floor; (void)service_dir; }
void bspLiftMotorSet(bsp_lift_dir_t dir, uint16_t pwm) { (void)dir; (void)pwm; }
void bspDoorMotorSet(bsp_door_dir_t dir, uint16_t pwm) { (void)dir; (void)pwm; }
bool bspCanInit(void) { return false; }
bool bspCanProcessRx(void) { return false; }
bool bspCanSendStatus(void) { return false; }
bool bspCanSendArrivedCheck(uint8_t floor) { (void)floor; return false; }
bool bspCanSendDoorCmd(uint8_t floor, uint8_t cmd, uint8_t dir) { (void)floor; (void)cmd; (void)dir; return false; }
void bspCanSendTest(void) {}
void bspUiInit(void) {}
void bspUiUpdate(void) {}
char bspKeypadGetKey(void) { return 0; }
bool bspWifiInit(void) { return false; }
void bspWifiProcess(void) {}
uint8_t bspGetLocalFloor(void) { return CAN_FLOOR_UNKNOWN; }
bool bspReadHallSensor(uint8_t floor) { (void)floor; return false; }
#endif // MCU_BLUEPILL
