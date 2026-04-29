#include "motion_unit_conv.h"

// 엔코더 11000 pulses == 약 520 mm
#define REF_PULSE_FOR_MM  11000.0f
#define REF_DISTANCE_MM      520.0f

float motionPulseToMM(float pulse) {
    return pulse * (REF_DISTANCE_MM / REF_PULSE_FOR_MM);
}

float motionMMToPulse(float mm) {
    return mm * (REF_PULSE_FOR_MM / REF_DISTANCE_MM);
}
