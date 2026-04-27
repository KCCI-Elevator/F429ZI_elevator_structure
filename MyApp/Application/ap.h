#ifndef __MY_AP__AP_H__
#define __MY_AP__AP_H__

#include <stdint.h>

#include "bsp.h"
#include "elevator.h"

#include "cmsis_os2.h"

// task
// StartDefaultTask
// motorTask

// function
void apInit(void);
void apMain(void);

#endif //__MY_AP__AP_H__