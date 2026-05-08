#ifndef __MAP_AP__ELEVATOR_CONTROLLER_H__
#define __MAP_AP__ELEVATOR_CONTROLLER_H__

#include "def.h"


void Elevator_Controller_Init();
void Elevator_Controller_Update();

bool Elevator_GoToFloor(uint8_t target_floor);
bool Elevator_IsBusy(void);
void Elevator_EmergencyStop();

#endif