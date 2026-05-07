#include "elevator_sequence.h"
#include "elevator_controller.h"
#include "cmsis_os.h"

//엘리베이터 이동시키는 로직.

void Elevator_RunSequence(void) {
    if (Elevator_IsBusy()) return; // 운행 중이면 아무것도 안 함

    static int sequence = 0;
    osDelay(2000); // 층 도착 후 대기

    switch (sequence) {
        case 0: Elevator_GoToFloor(3); sequence++; break;
        case 1: Elevator_GoToFloor(1); sequence++; break;
        case 2: Elevator_GoToFloor(3); sequence++; break;
        case 3: Elevator_GoToFloor(1); sequence = 0; break;
    }
}