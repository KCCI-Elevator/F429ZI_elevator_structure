#ifndef __SAFETY_MONITOR_H__
#define  __SAFETY_MONITOR_H__
#include <stdbool.h>

void Safety_Update(float current_ma);

bool Safety_IsSystemSafe(void)

#endif