#include "keypad.h"
#include "main.h"
#include "stdio.h"
#include "cmsis_os.h" // RTOS osDelay 등을 위해 필요

// 1. 초기화 함수 구현 (이게 빠져서 에러 발생)
void init_keypad_input(keypad_input_t* input) {
    for (uint8_t i = 0; i < 3; i++) input->selected_floor[i] = false;
    input->door_open_button = false;
    input->door_close_button = false;
    input->temp_floor = 0;
    input->confirmed_floor = 0;
    input->last_key = 0;
}

// 2. 키패드 스캔 함수 구현 (여기에 실제 Row/Col 스캔 로직을 넣으셔야 합니다)
char get_key(void) {
    // static을 사용하여 메모리 재할당 방지
    static const char key_map[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    // 포트와 핀 매핑 (직접 호출하는 방식이 가장 확실함)
    GPIO_TypeDef* row_ports[] = {ROW1_PORT, ROW2_PORT, ROW3_PORT, ROW4_PORT};
    uint16_t row_pins[] = {ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN};
    
    GPIO_TypeDef* col_ports[] = {COL1_PORT, COL2_PORT, COL3_PORT, COL4_PORT};
    uint16_t col_pins[] = {COL1_PIN, COL2_PIN, COL3_PIN, COL4_PIN};

    for (int i = 0; i < 4; i++) {
        // 모든 행 리셋 (SET = High)
        HAL_GPIO_WritePin(ROW1_PORT, ROW1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ROW2_PORT, ROW2_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ROW3_PORT, ROW3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ROW4_PORT, ROW4_PIN, GPIO_PIN_SET);

        // 현재 행만 활성화 (RESET = Low)
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_RESET);

        for (int j = 0; j < 4; j++) {
            if (HAL_GPIO_ReadPin(col_ports[j], col_pins[j]) == GPIO_PIN_RESET) {
                HAL_Delay(20); // 디바운싱
                if (HAL_GPIO_ReadPin(col_ports[j], col_pins[j]) == GPIO_PIN_RESET) {
                    return key_map[i][j];
                }
            }
        }
    }
    return 0;
}

// 3. 상태 조회 함수
keypad_input_t get_keypad_input() {
    keypad_input_t current;
    
    // 도어 버튼 체크
    HAL_GPIO_WritePin(ROW1_PORT, ROW1_PIN, GPIO_PIN_RESET);
    current.door_open_button  = (HAL_GPIO_ReadPin(COL4_PORT, COL4_PIN) == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ROW1_PORT, ROW1_PIN, GPIO_PIN_SET);

    HAL_GPIO_WritePin(ROW2_PORT, ROW2_PIN, GPIO_PIN_RESET);
    current.door_close_button = (HAL_GPIO_ReadPin(COL4_PORT, COL4_PIN) == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ROW2_PORT, ROW2_PIN, GPIO_PIN_SET);

    // 숫자키 스캔
    char key = get_key();
    
    for(int i=0; i<3; i++) current.selected_floor[i] = false;
    current.last_key = key;

    if (key >= '1' && key <= '3') {
        current.selected_floor[key - '1'] = true;
    }
    
    return current;
}

// 4. 태스크
void StartKeypadTask(void *argument) {
    uartInit();
    keypad_input_t keypad;
    init_keypad_input(&keypad);

    //bool last_close = false;
    char last_processed_key = 0;

    for(;;) {
        // // [1] 도어 버튼 감지
        // bool cur_open = (HAL_GPIO_ReadPin(ROW1_PORT, ROW1_PIN) == GPIO_PIN_RESET && 
        //                  HAL_GPIO_ReadPin(COL4_PORT, COL4_PIN) == GPIO_PIN_RESET);
        // bool cur_close = (HAL_GPIO_ReadPin(ROW2_PORT, ROW2_PIN) == GPIO_PIN_RESET && 
        //                   HAL_GPIO_ReadPin(COL4_PORT, COL4_PIN) == GPIO_PIN_RESET);

        // // A: 누르는 동안 계속 출력
        // if (cur_open) uartPrintf(0, "Door Open\r\n");
        
        // // B: 한번만 출력
        // if (cur_close) {
        //     if (!last_close) uartPrintf(0, "Door Close\r\n");
        //     last_close = true;
        // } else {
        //     last_close = false;
        // }

        // [2] 숫자키 스캔
        if (true) { // !cur_open && !cur_close
            char key = get_key();
            
            // 키가 새로 눌렸을 때만 처리
            if (key != 0 && key != last_processed_key) {
                if (key >= '1' && key <= '3') {
                    int floor_idx = key - '1';

                    // 토글 로직: 이미 선택되어 있으면 취소, 아니면 선택
                    keypad.selected_floor[floor_idx] = !keypad.selected_floor[floor_idx];

                    // 현재 상태 출력
                    if (keypad.selected_floor[floor_idx]) {
                        uartPrintf(0, "Go : %c\r\n", key); // 선택 시
                    } else {
                        uartPrintf(0, "Go : \r\n"); // 취소 시
                    }

                    // 전체 선택된 층 리스트 출력
                    for(int i = 0; i < 3; i++) {
                        if (keypad.selected_floor[i]) {
                            uartPrintf(0, "selected: %d\r\n", i + 1);
                        }
                    }
                }
            }
            last_processed_key = key; 
        } else {
            last_processed_key = 0; // 버튼 떼면 입력 감지 초기화
        }
        osDelay(20);
    }
}