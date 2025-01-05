#ifndef CPU_H
#define CPU_H

#include <types.h>
#include <lresult.h>
#include "ppu.h"
#include "apu.h"
#include <Emulator/mem.h>

typedef enum {
  AF = 0,
  BC,
  DE,
  HL,
  SP,
  PC
} ERegisterFull;

typedef enum {
  A = 0, F,
  B, C,
  D, E,
  H, L,
  SPL, SPH,
  PCL, PCH
} ERegisterHalf;

typedef struct {
  union {
    u16 v;
    struct {
      u8 l;
      u8 h;
    } bytes;
  };
} Register;

typedef struct Cpu {
  Pin addr_bus[16];
  Pin data_bus[8];

  Pin pin_X0;
  Pin pin_X1;

  // MD/MA tbd

  Pin pin_MWR;
  Pin pin_MCS;
  Pin pin_MOE;

  Pin pin_LD1;
  Pin pin_LD0;
  Pin pin_CPG;
  Pin pin_CP;
  Pin pin_ST;
  Pin pin_CPL;
  Pin pin_FR;
  Pin pin_S;

  // P10-15 tbd

  Pin pin_RST;
  Pin pin_SOUT;
  Pin pin_SIN;
  Pin pin_SCX;

  Pin pin_CLK;
  Pin pin_WR;
  Pin pin_RD;
  Pin pin_CS;

  Pin pin_VIN;
  Pin pin_LOUT;
  Pin pin_ROUT;
  Pin pin_T1;
  Pin pin_T2;

  Pin pin_VDD;
  Pin pin_VSS0;
  Pin pin_VSS1;

  Register registers[6];
  u8 IR;

  u8 interrupt_enable;
  u8 interrupt_flag;

  bool bootrom_mapped;

  Ppu ppu;
  Apu apu;

  Mem* mem;
} Cpu;

Result cpu_init(Cpu* cpu, Mem* mem);

Result cpu_step(Cpu* cpu);

#endif // !CPU_H
