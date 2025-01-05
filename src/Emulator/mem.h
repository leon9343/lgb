#ifndef MEM_H
#define MEM_H

#include <lresult.h>
#include <types.h>

struct Cpu;

#define WRAM_SIZE (8 * 1024)
#define VRAM_SIZE (8 * 1024)
#define OAM_SIZE  (0xA0)
#define HRAM_SIZE (0x7F)

typedef struct {
  u8 wram[WRAM_SIZE];
  u8 vram[VRAM_SIZE];
  u8 oam[OAM_SIZE];
  u8 hram[HRAM_SIZE];
} Mem;

// Inititalizes the memory with default values 
Result mem_init(Mem* mem);

// Returns the byte at the specified address
u8 mem_read8(Mem* mem, struct Cpu* cpu, u16 addr);

// Write the byte at the specified address
void mem_write8(Mem* mem, struct Cpu* cpu, u16 addr, u8 val);

#endif // !MEM_H
