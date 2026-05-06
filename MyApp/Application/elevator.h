#ifndef __MAP_AP__ELEVATOR_H__
#define __MAP_AP__ELEVATOR_H__

#include "def.h"
#include "bsp.h"

#define ELEVATOR_FLOOR_MAX      3
#define FLOOR_BIT(floor)        (1U << ((floor) - 1U))

#define DOOR_OPEN_TIME_MS       3000
#define DOOR_MOVE_TIMEOUT_MS    3000

typedef enum {
  CAR_STATE_IDLE = 0,
  CAR_STATE_MOV_UP,
  CAR_STATE_MOV_DOWN,
  CAR_STATE_WAIT_SENSOR_ACK,  // 홀센서 더블체크 대기 상태
  CAR_STATE_DOOR_OPENING,
  CAR_STATE_DOOR_OPEN,
  CAR_STATE_DOOR_CLOSING,
  CAR_STATE_ERROR
} elevator_state_t;

typedef struct {
  elevator_state_t state;
  uint8_t curr_floor;
  uint8_t target_floor;
  uint8_t start_floor;  // [추가] 출발 층을 기억하여, 출발 직후 바로 다시 서는 현상 방지
  uint8_t req_mask;
  uint32_t state_time;
  bool entry_executed;  // 상태 진입 시 한 번만 실행되는 작업용 플래그
} elevator_t;

// 함수 선언
void elevatorInit(elevator_t *ctx);
void elevatorUpdate(elevator_t *ctx, uint32_t now);
uint8_t elevatorFindNextTarget(elevator_t *ctx, bsp_elevator_input_t *input, uint8_t current_dir);

#endif //__MAP_AP__ELEVATOR_H__