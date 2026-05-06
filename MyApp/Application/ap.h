#ifndef __MY_AP__AP_H__
#define __MY_AP__AP_H__

#include <stdint.h>
#include "elevator.h"
#include "my_can.h"
#include "cmsis_os2.h"
#include "wifi.h"

// [중요] 이 함수들은 freertos.c의 __weak 함수를 override하기 위해 선언
// ap.c에서 실제 구현을 제공함
void StartDefaultTask(void *argument);
void motorTask(void *argument);
void StartCanRXTask(void *argument);
void StartElevatorTask(void *argument);

#endif //__MY_AP__AP_H__