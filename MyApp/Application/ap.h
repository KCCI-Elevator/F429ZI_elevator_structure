#ifndef __MY_AP__AP_H__
#define __MY_AP__AP_H__

#include "def.h"

void StartDefaultTask(void *argument);
void StartElevatorTask(void *argument);
void StartCanRXTask(void *argument);
void StartKeypadTask(void *argument);
void StartWifiTask(void *argument);
void motorTask(void *argument);

#endif // __MY_AP__AP_H__
