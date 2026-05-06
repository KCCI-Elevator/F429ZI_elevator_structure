#ifndef LOAD_MONITOR_H
#define LOAD_MONITOR_H

typedef enum {
    STATUS_NORMAL,
    STATUS_OVERLOAD
} LoadStatus_t;

LoadStatus_t Monitor_CheckWeight(float current_ma);

#endif