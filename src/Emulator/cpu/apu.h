#ifndef APU_H
#define APU_H

#include <types.h>
#include <lresult.h>

typedef struct {
  Pin pin_AUDIO_L;
  Pin pin_AUDIO_R;

  // Channel 1
  u8 nr10;
  u8 nr11;
  u8 nr12;
  u8 nr13;
  u8 nr14;
  
  // Channel 2
  u8 nr21;
  u8 nr22;
  u8 nr23;
  u8 nr24;

  // Channel 3
  u8 nr30;
  u8 nr31;
  u8 nr32;
  u8 nr33;
  u8 nr34;

  // Channel 4
  u8 nr41;
  u8 nr42;
  u8 nr43;
  u8 nr44;

  // Control
  u8 nr50;
  u8 nr51;
  u8 nr52;

  u8 wave_ram[16];
} Apu;

// To be called internally by cpu. Inititalizes the ppu's internals to default values
Result apu_init(Apu* apu);

// TODO (empty)
Result apu_step(Apu* apu);

#endif // !APU_H
