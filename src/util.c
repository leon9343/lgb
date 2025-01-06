#include "util.h"

void pin_set_low(Pin* pin)  { pin->state = PIN_LOW;  }
void pin_set_high(Pin* pin) { pin->state = PIN_HIGH; }
void pin_set_hiz (Pin* pin) { pin->state = PIN_HIGHZ; }

void set_bus_hiz(Cpu* cpu) {
  for (int i = 0; i < 16; i++) {
    pin_set_hiz(&cpu->addr_bus[i]);
  }
  for (int i = 0; i < 8; i++) {
    pin_set_hiz(&cpu->data_bus[i]);
  }
}
