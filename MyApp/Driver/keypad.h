#ifndef __MAP_DRIVER__KEYPAD_H__
#define __MAP_DRIVER__KEYPAD_H__

#include "def.h"

void keypadInit(void);
char keypadGetKey(void);

/* Compatibility alias for older code. */
static inline char get_key(void) { return keypadGetKey(); }

#endif // __MAP_DRIVER__KEYPAD_H__
