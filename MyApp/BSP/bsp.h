#ifndef __MAP_BSP__BSP_H_
#define __MAP_BSP__BSP_H_

#include "def.h"

#if !defined(MCU_F429) && !defined(MCU_BLUEPILL)
#define MCU_F429
#endif

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

typedef enum {
    BSP_STATE_NORMAL = 0,
    BSP_STATE_INSPECTION,
    BSP_STATE_MOVING,
    BSP_STATE_EMERGENCY_STOP
} bsp_special_state_t;

typedef struct {
    bool floor_valid;
    uint8_t curr_floor;
    bsp_lift_dir_t current_dir;

    uint8_t req_mask;
    bool call_up[4];
    bool call_down[4];
    bool call_car[4];
    bool req_wifi[4];

    bool arrived_ack;

    bool top_limit;
    bool bottom_limit;
    bool door_open_limit;
    bool door_close_limit;
    bool obstacle_detected;
    bool emergency_stop;
    bool motor_over_current;

    bsp_special_state_t special_state;
} bsp_elevator_input_t;

void bspInit(void);
void bspUpdate(void);
uint32_t bspMillis(void);
void bspDelay(uint32_t delay_ms);

void bspSetCurrentThreshold(float threshold_ma);
bool bspCheckOverCurrent(float current_ma);

void bspElevatorReadInput(bsp_elevator_input_t *input);
void bspSetCurrentFloor(uint8_t floor);
void bspSetFloorValid(bool valid);
void bspSetCurrentDir(bsp_lift_dir_t dir);
void bspSetSpecialState(bsp_special_state_t state);
void bspSetArrivedAck(bool ack);
void bspSetCarCall(uint8_t floor, bool state);
void bspToggleCarCall(uint8_t floor);
void bspSetHallCallUp(uint8_t floor, bool state);
void bspSetHallCallDown(uint8_t floor, bool state);
void bspSetWifiRequest(uint8_t floor, bool state);
void bspClearServicedRequests(uint8_t floor, bsp_lift_dir_t service_dir);

void bspLiftMotorSet(bsp_lift_dir_t dir, uint16_t pwm);
void bspDoorMotorSet(bsp_door_dir_t dir, uint16_t pwm);

bool bspCanInit(void);
bool bspCanProcessRx(void);
bool bspCanSendStatus(void);
bool bspCanSendArrivedCheck(uint8_t floor);
bool bspCanSendDoorCmd(uint8_t floor, uint8_t cmd, uint8_t dir);
void bspCanSendTest(void);

void bspUiInit(void);
void bspUiUpdate(void);
char bspKeypadGetKey(void);

bool bspWifiInit(void);
void bspWifiProcess(void);

#ifdef MCU_BLUEPILL
uint8_t bspGetLocalFloor(void);
bool bspReadHallSensor(uint8_t floor);
#endif

#endif // __MAP_BSP__BSP_H_
