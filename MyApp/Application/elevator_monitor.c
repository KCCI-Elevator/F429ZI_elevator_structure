#include "elevator_monitor.h"
#include "my_uart.h"
#include "cmsis_os.h"

//전류 가져와서 출력

extern float g_avg_current;//motorTask가 업데이트하는 전역변수

void Elevator_ReportStatus(void) {
    static uint32_t last_tick = 0;

    //출력 속도 조절
    if(osKernelGetTickCount() - last_tick < 500) return;
    last_tick = osKernelGetTickCount();

    int32_t val_x100 = (int32_t)(g_avg_current * 100.0f);
    int32_t int_part = val_x100 / 100; //정수부분 추출
    int32_t frac_part = (val_x100 % 100 < 0) ? -(val_x100 % 100) : (val_x100 % 100);//음수로 찍혀도 양수로 보정

    uartPrintf(0, "Current: %ld.%02ld mA\r\n", int_part, frac_part);


}