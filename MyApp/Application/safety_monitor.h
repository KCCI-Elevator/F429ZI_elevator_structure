#include "safety_monitor.h"
#include "bsp.h"
#include <math.h>

static bool s_is_overload = false;
static uint16_t s_debounce_cnt = 0;

void Safety_Update(float current_ma) {
    // 1. 디바운스 로직 수행 (기존에 만든 코드)
    if (fabsf(current_ma) > 500.0f) { // 예시 문턱치
        if (s_debounce_cnt < 15) s_debounce_cnt++;
        if (s_debounce_cnt >= 15) s_is_overload = true;
    } else {
        if (s_debounce_cnt > 0) s_debounce_cnt--;
        if (s_debounce_cnt == 0) s_is_overload = false;
    }
}

bool Safety_IsSystemSafe(void) {
    return !s_is_overload; // 과부하가 아니어야 안전함
}