#include "mem.h"
#include <llog.h>
#include <util.h>
#include <lresult.h>
#include <string.h>
#include "cpu/cpu.h"

static u8 read_io_register(Cpu* cpu, u16 addr) {
  // TODO
  switch (addr) {
    case 0xFF00:
      return 0xCF;

    default:
      return 0xFF;
  }
}

static void write_io_register(Cpu* cpu, u16 addr, u8 val) {
  switch (addr) {
    case 0xFF50: {
      if (val != 0) {
        cpu->bootrom_mapped = false;
        LOG_TRACE("unmapped bootrom");
      }
      return;
    }
  }
}

Result mem_init(Mem *mem) {
  if (!mem) {
    return result_error(Error_NullPointer, "invalid mem to mem_init");
  }
  memset(mem, 0, sizeof(*mem));

  return result_ok();
}

u8 mem_read8(Mem *mem, Cpu *cpu, u16 addr) {

  if (cpu->bootrom_mapped) {
    if (addr < 0x0100)
      return cpu->dmg_bootrom[addr];
  }

  if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000)) {
    LOG_WARNING("mem_read8 called with cart addr: %04X", addr);
    return 0xFF;
  }

  // VRAM
  if (addr >= 0x8000 && addr < 0xA000) {
    u16 index = addr - 0x8000;
    if (index < VRAM_SIZE)
      return mem->vram[index];
    return 0xFF;
  }
  // WRAM
  else if (addr >= 0xC000 && addr < 0xE000) {
    u16 index = addr - 0xC000;
    if (index < WRAM_SIZE)
      return mem->wram[index];
    return 0xFF;
  }
  // Mirrored WRAM region
  else if (addr >= 0xE000 && addr < 0xFE00) {
    u16 mirror = addr - 0xE000;
    if (mirror < 0x2000) 
      return mem->wram[mirror];
    return 0xFF;
  }
  // OAM
  else if (addr >= 0xFE00 && addr < 0xFEA0) {
    u16 index = addr - 0xFE00;
    if (index < OAM_SIZE) 
      return mem->oam[index];
    return 0xFF;
  }
  // Unusable
  else if (addr >= 0xFEA0 && addr < 0xFF00) {
    return 0xFF;
  }
  else if (addr >= 0xFF00 && addr < 0xFF80) {
    return read_io_register(cpu, addr);
  }
  else if (addr >= 0xFF80 && addr < 0xFFFF) {
    u16 index = addr - 0xFF80;
    if (index < HRAM_SIZE) 
      return mem->hram[index];
    return 0xFF;
  }
  else if (addr == 0xFFFF)
    return cpu->interrupt_enable;

  // unreachable
  return 0xFF;
}

void mem_write8(Mem *mem, Cpu *cpu, u16 addr, u8 value) {
  if (addr < 0x8000 || (addr >= 0xA000 && addr < 0xC000)) {
    LOG_WARNING("mem_write8 called with cart addr: %04X", addr);
  }

  // VRAM
  if (addr >= 0x8000 && addr < 0xA000) {
    u16 index = addr - 0x8000;
    if (index < VRAM_SIZE)
      mem->vram[index] = value;
  }
  // WRAM
  else if (addr >= 0xC000 && addr < 0xE000) {
    u16 index = addr - 0xC000;
    if (index < WRAM_SIZE)
      mem->wram[index] = value;
  }
  // Mirrored WRAM region
  else if (addr >= 0xE000 && addr < 0xFE00) {
    u16 mirror = addr - 0xE000;
    if (mirror < 0x2000) 
      mem->wram[mirror] = value;
  }
  // OAM
  else if (addr >= 0xFE00 && addr < 0xFEA0) {
    u16 index = addr - 0xFE00;
    if (index < OAM_SIZE) 
      mem->oam[index] = value;
  }
  // Unusable
  else if (addr >= 0xFEA0 && addr < 0xFF00) {
    return;
  }
  else if (addr >= 0xFF00 && addr < 0xFF80) {
    write_io_register(cpu, addr, value);
  }
  else if (addr >= 0xFF80 && addr < 0xFFFF) {
    u16 index = addr - 0xFF80;
    if (index < HRAM_SIZE) 
      mem->hram[index] = value;
  }
  else if (addr == 0xFFFF)
    cpu->interrupt_enable = value;
}
