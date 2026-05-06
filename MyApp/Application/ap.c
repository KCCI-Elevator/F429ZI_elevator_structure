#include "ap.h"

static elevator_t elevator;
extern bsp_elevator_input_t elevator_input;

// CAN 관련 외부 변수
extern volatile uint8_t can_rx_flag;
extern uint8_t can_rx_buf[8];
extern uint32_t can_rx_id;
extern uint8_t can_rx_len;

// my_can.c의 함수들
extern void send_car_status(void);
extern void send_can_arrived_check(uint8_t floor);
extern void send_can_door_cmd(uint8_t floor, uint8_t cmd, uint8_t dir);
extern void process_can_rx(uint32_t rx_id, uint8_t *rx_data);

// 상태 변화 감지용
static uint8_t last_sent_floor = 0xFF;
static uint8_t last_sent_dir = 0xFF;
static uint8_t last_sent_state = 0xFF;
static uint32_t last_heartbeat_tick = 0;


/**
 * @brief 엘리베이터 태스크 (F429 메인 로직)
 */
void StartElevatorTask(void *argument) {

    // ==========================================
    // [수정 1] BSP 초기화 후 OLED 초기화 호출
    // ==========================================
    bspInit();
    oled_init();

    osDelay(100);
    test_can_tx();
    osDelay(100);

    uint32_t prev_time = HAL_GetTick();
    uint32_t move_start_tick = HAL_GetTick();

    // [수정 2] OLED 갱신 주기를 제어할 타이머 변수 추가
    uint32_t oled_prev_time = HAL_GetTick();

    memset(&elevator, 0, sizeof(elevator));
    elevator.state = CAR_STATE_IDLE;
    elevator.curr_floor = 1;
    elevator.target_floor = 0;
    elevator.req_mask = 0;
    elevator.state_time = HAL_GetTick();
    elevator.entry_executed = false;

    send_car_status();

    while (1) {
        uint32_t now = HAL_GetTick();

        // --- 10ms 주기 처리 (통신 및 상태 제어) ---
        if (now - prev_time >= 10) {
            prev_time = now;

            elevator_input.floor_valid = true;
            elevatorUpdate(&elevator, now);

            bool is_changed = (elevator_input.curr_floor != last_sent_floor) ||
                              (elevator_input.current_dir != last_sent_dir) ||
                              (elevator_input.special_state != last_sent_state);

            bool is_heartbeat = (now - last_heartbeat_tick >= 5000);

            if (is_changed || is_heartbeat) {
                if (elevator_input.curr_floor != 0 || elevator_input.special_state == BSP_STATE_MOVING) {
                    send_car_status();
                    last_sent_floor = elevator_input.curr_floor;
                    last_sent_dir = elevator_input.current_dir;
                    last_sent_state = elevator_input.special_state;
                    last_heartbeat_tick = now;
                }
            }
        }

        // ==========================================
        // [수정 3] 50ms 주기 OLED 화면 갱신 (초당 20프레임)
        // 무한 루프 폭주를 막고 부드럽게 렌더링
        // ==========================================
        if (now - oled_prev_time >= 50) {
            oled_prev_time = now;
            f429_oled_ui_update(
                elevator_input.curr_floor,
                elevator_input.current_dir,
                elevator_input.special_state,
                elevator_input.call_car
            );
        }

        // 4. 이동 시뮬레이션
        if (elevator.state == CAR_STATE_MOV_UP || elevator.state == CAR_STATE_MOV_DOWN) {
            if (now - move_start_tick >= 3000) {
                move_start_tick = now;
                if (elevator_input.current_dir == BSP_LIFT_UP && elevator_input.curr_floor < 3) {
                    elevator_input.curr_floor++;
                }
                else if (elevator_input.current_dir == BSP_LIFT_DOWN && elevator_input.curr_floor > 1) {
                    elevator_input.curr_floor--;
                }
            }
        } else {
            move_start_tick = now;
        }

        osDelay(1);
    }
}

/**
 * @brief 더미 함수들 (freertos.c의 weak 함수 override)
 */
void StartDefaultTask(void *argument) {
    for(;;) {
        osDelay(1);
    }
}

void motorTask(void *argument) {
    for(;;) {
        osDelay(1);
    }
}

/**
 * @brief CAN 수신 전용 Task
 */
void StartCanRXTask(void *argument) {
  // 통신 시작
  can_init();

  while (1) {
    // CAN 수신 처리
    if (can_rx_flag == 1) {
      can_rx_flag = 0;

      // 수신 데이터 복사
      uint32_t rx_id = can_rx_id;
      uint8_t rx_buf[8];
      uint8_t rx_len = can_rx_len;

      for (uint8_t i = 0; i < rx_len && i < 8; i++) {
        rx_buf[i] = can_rx_buf[i];
      }

      // LED 수신 표시 (Active-High)
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   // ON
      osDelay(50);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // OFF

      // 처리
      process_can_rx(rx_id, rx_buf);
    }

    osDelay(10);
  }
}

// 2. 키패드 처리 태스크
void StartKeypadTask(void *argument) {
  char last_processed_key = 0;

  for(;;) {
    char key = get_key();

    if (key != 0) {
      // 버튼이 길게 눌려도 한 번만 처리되도록 에지(Edge) 감지
      if (key != last_processed_key) {

        // 엘리베이터 층수 버튼 (1~3)
        if (key >= '1' && key <= '3') {
          // 문자 '1'을 숫자 1로 변환
          uint8_t floor_idx = key - '0';

          // [핵심] 누를 때마다 엘리베이터 전역 상태(call_car) 토글 (ON/OFF)
          elevator_input.call_car[floor_idx] = !elevator_input.call_car[floor_idx];

          // 디버그 출력
          //uartPrintf(0, "[KEYPAD] Car Call Floor %d : %s\r\n",
          //          floor_idx, elevator_input.call_car[floor_idx] ? "ON" : "OFF");
        }
        // 도어 열림 버튼 ('A'를 열림 버튼으로 가정)
        else if (key == 'A') {
          // 추후 도어 열림 로직 연동
        }
        // 도어 닫힘 버튼 ('B'를 닫힘 버튼으로 가정)
        else if (key == 'B') {
          // 추후 도어 닫힘 로직 연동
        }
      }
    }

    last_processed_key = key; // 현재 키 상태 저장

    osDelay(20); // 태스크 휴식
  }
}

extern wifi_t wifi_ctx;
extern esp8266_t esp8266_ctx;

/*=================================================================*/
// Wi-Fi 상태 및 송수신을 총괄 관리하는 FreeRTOS 태스크
/*=================================================================*/
void StartWifiTask(void *argument) {
  uint32_t last_send_time = 0;
  uint32_t last_recv_time = 0;

  esp8266_Init(&esp8266_ctx, UART_CH_ESP8266);
  uartInit();
  wifiInit(&wifi_ctx, &esp8266_ctx);

  for (;;) {
    wifiProcess(&wifi_ctx);

    if (wifiIsConnected(&wifi_ctx) == 1) {
      uint32_t now = HAL_GetTick();

      // 1. 상태 송신 로직 (2초마다 V4로 현재 층 전송)
      if ((now - last_send_time) >= 2000) {
        wifiSendElevatorStatus(&wifi_ctx, &elevator_input);
        last_send_time = HAL_GetTick();
      }

      // 2. 명령 수신 로직 (1.5초마다 V1, V2, V3 확인)
      else if ((now - last_recv_time) >= 1500) {
        int v1, v2, v3;
        if (wifiReceiveCommands(&wifi_ctx, &v1, &v2, &v3) == 0) {
          // 0 또는 1이 들어왔을 때만 상태 업데이트
          if (v1 == 0 || v1 == 1) elevator_input.req_wifi[0] = (v1 == 1);
          if (v2 == 0 || v2 == 1) elevator_input.req_wifi[1] = (v2 == 1);
          if (v3 == 0 || v3 == 1) elevator_input.req_wifi[2] = (v3 == 1);
        }
        last_recv_time = HAL_GetTick();
      }
    }

    osDelay(10);
  }
}