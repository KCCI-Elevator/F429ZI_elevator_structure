#ifndef __MAP_HW__MY_CAN_H__
#define __MAP_HW__MY_CAN_H__

#include "def.h"  // 공통 CAN ID 및 매크로 사용
#include "main.h"
#include "bsp.h"

extern volatile uint8_t can_rx_flag;
extern uint8_t can_rx_buf[8];
extern uint32_t can_rx_id;
extern uint8_t can_rx_len;

// 1. 통신 초기화 및 기본 송수신
void can_init(void);
void can_transmit(uint32_t id, uint8_t* data, uint8_t length);
void process_can_rx(uint32_t rx_id, uint8_t *rx_data);

// 2. F429 -> BluePill 명령 송신 함수들
void send_car_status(void);
void send_can_arrived_check(uint8_t floor);
void send_can_door_cmd(uint8_t floor, uint8_t cmd, uint8_t dir);
void test_can_tx(void);

#endif //__MAP_HW__MY_CAN_H__