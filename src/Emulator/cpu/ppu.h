#ifndef PPU_H
#define PPU_H

#include <types.h>
#include <lresult.h>

typedef struct {
  // Pins
  Pin pin_LCD_DATA;
  Pin pin_LCD_HSYNC;
  Pin pin_LCD_VSYNC;
  Pin pin_LCD_CLK;

  // Ppu Registers
  u8 lcdc;
  u8 stat;
  u8 scy;
  u8 scx;
  u8 ly;
  u8 lyc;
  u8 dma;
  u8 bgp;
  u8 obp0;
  u8 obp1;
  u8 wy;
  u8 wx;

  // TODO: counters, fetchers, etc
} Ppu;

// To be called internally by cpu
Result ppu_init(Ppu* ppu);

// TODO
Result ppu_step(Ppu* ppu);

#endif // !PPU_H
