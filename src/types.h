#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// Pin Definition
typedef enum {
  PIN_LOW = 0, // 0V
  PIN_HIGH,    // 5V
  PIN_HIGHZ    // not driven
} EPinState;

typedef enum {
  PIN_INPUT,
  PIN_OUTPUT,
  PIN_INOUT,
  PIN_POWER
} EPinType;

typedef struct {
  char name[16];
  EPinType type;
  EPinState state;
} Pin;


#endif // !TYPES_H
