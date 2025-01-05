#include "apu.h"
#include "types.h"
#include <util.h>
#include <lresult.h>
#include <llog.h>
#include <string.h>

static void init_pin(Pin *pin, const char *name, EPinType type, EPinState default_state) {
  memset(pin, 0, sizeof(*pin));
  if (name) {
    strncpy(pin->name, name, sizeof(pin->name) - 1);
  }
  pin->type = type;
  pin->state = default_state;
}


Result apu_init(Apu* apu) {
  if (!apu) {
    return result_error(Error_NullPointer, "invalid apu to apu_init");
  }
  memset(apu, 0, sizeof(*apu));

  init_pin(&apu->pin_AUDIO_L, "AUDIO_L", PIN_OUTPUT, PIN_LOW);
  init_pin(&apu->pin_AUDIO_R, "AUDIO_R", PIN_OUTPUT, PIN_LOW);

  apu->nr10 = 0x80; 
  apu->nr11 = 0xBF;
  apu->nr12 = 0xF3;
  apu->nr13 = 0xFF;
  apu->nr14 = 0xBF;

  apu->nr21 = 0x3F;
  apu->nr22 = 0x00;
  apu->nr23 = 0xFF;
  apu->nr24 = 0xBF;

  apu->nr30 = 0x7F;
  apu->nr31 = 0xFF;
  apu->nr32 = 0x9F;
  apu->nr33 = 0xFF;
  apu->nr34 = 0xBF;

  apu->nr41 = 0xFF;
  apu->nr42 = 0x00;
  apu->nr43 = 0x00;
  apu->nr44 = 0xBF;

  apu->nr50 = 0x77;
  apu->nr51 = 0xF3;
  apu->nr52 = 0xF1;

  for (int i = 0; i < 16; i++) 
    apu->wave_ram[i] = 0xFF;

  LOG_TRACE("apu initialized successfully");
  return result_ok();
}

Result apu_step(Apu* apu) {
  if (!apu) {
    return result_error(Error_NullPointer, "invalid apu to apu_step");
  }

  return result_ok();
}
