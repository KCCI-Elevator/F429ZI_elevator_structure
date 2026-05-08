//
// Created by hiimseoll on 26. 5. 8..
//

#ifndef F429ZI_ELEVATOR_STRUCTURE_SAFETY_MONITOR_H
#define F429ZI_ELEVATOR_STRUCTURE_SAFETY_MONITOR_H

#include <stdbool.h>

void Safety_Init();
void Safety_Update(float current_ma);
bool Safety_IsSystemSafe(void);

#endif // F429ZI_ELEVATOR_STRUCTURE_SAFETY_MONITOR_H
