#ifndef __MAP_HW__MY_CAN_H__
#define __MAP_HW__MY_CAN_H__

#include "def.h"
#include "hw_def.h"

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
} can_frame_t;

extern volatile uint8_t can_init_status;
extern volatile uint32_t can_rx_callback_count;

bool canInit(void);
bool canTransmit(uint32_t id, const uint8_t *data, uint8_t len);
bool canReceive(can_frame_t *frame);
void canResetRx(void);
void canTestTx(void);

/* Compatibility aliases for older code. Prefer the camelCase APIs above. */
static inline void can_init(void) { (void)canInit(); }
static inline void can_transmit(uint32_t id, uint8_t *data, uint8_t length) { (void)canTransmit(id, data, length); }
static inline void test_can_tx(void) { canTestTx(); }

#endif // __MAP_HW__MY_CAN_H__
