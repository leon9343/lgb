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

void set_addr_bus_value(Cpu* cpu, u16 value) {
  for (int i = 0; i < 16; i++) {
    u16 bit = (value >> i) & 1;

    if (bit)
      pin_set_high(&cpu->addr_bus[i]);
    else 
      pin_set_low(&cpu->addr_bus[i]);
  }  
}

void set_data_bus_value(Cpu* cpu, u8 value) {
  for (int i = 0; i < 8; i++) {
    u8 bit = (value >> i) & 1;

    if (bit)
      pin_set_high(&cpu->data_bus[i]);
    else 
      pin_set_low(&cpu->data_bus[i]);
  }  
}
