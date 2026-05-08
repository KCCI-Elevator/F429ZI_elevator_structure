// safety_monitor.c
#include "safety_monitor.h"
#include "bsp.h"
#include "elevator_controller.h"

void Safety_Update(float current_ma) {
   

    // 2. 만약 BSP가 과전류라고 확정(디바운스 완료)했다면
    if (bspCheckOverCurrent(current)) {
        // [결정] 시스템을 즉시 멈추고 안전 상태를 false로 만듦
        Elevator_EmergencyStop(); 
    }
}

bool Safety_IsSystemSafe(void) {
    bsp_elevator_input_t input;
    bspElevatorReadInput(&input); // BSP의 최종 상태를 읽어옴(데이터 무결성)
    
    // 과전류 상태가 아니어야 안전함
    return !input.motor_over_current;
}