#include "ppu.h"
#include <util.h>
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

Result ppu_init(Ppu *ppu) {
  if (!ppu) {
    return result_error(Error_NullPointer, "bad ppu to ppu_init");
  }
  memset(ppu, 0, sizeof(*ppu));

  init_pin(&ppu->pin_LCD_DATA,  "LCD_DATA",  PIN_OUTPUT, PIN_LOW);
  init_pin(&ppu->pin_LCD_HSYNC, "LCD_HSYNC", PIN_OUTPUT, PIN_LOW);
  init_pin(&ppu->pin_LCD_VSYNC, "LCD_VSYNC", PIN_OUTPUT, PIN_LOW);
  init_pin(&ppu->pin_LCD_CLK,   "LCD_CLK",   PIN_OUTPUT, PIN_LOW);

  ppu->lcdc = 0x91;
  ppu->stat = 0x85;
  ppu->scy  = 0x00;
  ppu->scx  = 0x00;
  ppu->ly   = 0x00;
  ppu->lyc  = 0x00;
  ppu->dma  = 0xFF;
  ppu->bgp  = 0xFC;
  ppu->obp0 = 0xFF;
  ppu->obp1 = 0xFF;
  ppu->wy   = 0x00;
  ppu->wx   = 0x00;

  LOG_TRACE("ppu initialized successfully");
  return result_ok();
}

Result ppu_step(Ppu* ppu) {
  if (!ppu) {
    return result_error(Error_NullPointer, "bad ppu to ppu_init");
  }

  // TODO

  return result_ok();
}
