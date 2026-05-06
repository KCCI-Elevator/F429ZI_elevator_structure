#include "load_monitor.h"

// 기준값 설정 (팬 테스트를 통해 얻은 값으로 수정)
#define OVERLOAD_THRESHOLD_MA 500.0f 

LoadStatus_t Monitor_CheckWeight(float current_ma) {
    if (current_ma > OVERLOAD_THRESHOLD_MA) {
        return STATUS_OVERLOAD;
    }
    return STATUS_NORMAL;
}