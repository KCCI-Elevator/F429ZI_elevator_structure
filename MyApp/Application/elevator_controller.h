#ifndef __MAP_AP__ELEVATOR_CONTROLLER_H__
#define __MAP_AP__ELEVATOR_CONTROLLER_H__

#include "def.h"

void Elevator_Controller_Init();
bool Elevator_Controller_Update(uint8_t direction);
void Elevator_Controller_SetTarget(float distance_mm);



#endif