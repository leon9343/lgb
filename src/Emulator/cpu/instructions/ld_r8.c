#include "ld_r8.h"
#include <Emulator/mem.h>
#include <types.h>
#include <util.h>
#include <llog.h>
#include <string.h>

// LD r8 imm
static void ld_r8_readimm_t0(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r8_readimm_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

static void ld_r8_readimm_t2(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++; 
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_high(&cpu->pin_RD);
  }
}
static void ld_r8_readimm_t3(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r8_read_imm_cycle_create() {
  MCycle m = mcycle_new(true, 4);
  m.tcycles[0] = ld_r8_readimm_t0;
  m.tcycles[1] = ld_r8_readimm_t1;
  m.tcycles[2] = ld_r8_readimm_t2;
  m.tcycles[3] = ld_r8_readimm_t3;
  return m;
}

static void ld_r8_store_t0(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 reg_idx = (cpu->IR >> 3) & 0x07;
    u8* reg_ptr = cpu_get_reg8_opcode(cpu, reg_idx);

    if (reg_ptr) *reg_ptr = cpu->data_value;
  }
}

static void ld_r8_fetch_t1(Cpu* cpu, Mem* mem) { fetch_t0(cpu, mem); }
static void ld_r8_fetch_t2(Cpu* cpu, Mem* mem) { fetch_t1(cpu, mem); }

static void ld_r8_fetch_t3(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_MCS);
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r8_fetch_next_cycle_create() {
  MCycle m = mcycle_new(true, 4);
  m.tcycles[0] = ld_r8_store_t0; 
  m.tcycles[1] = ld_r8_fetch_t1; 
  m.tcycles[2] = ld_r8_fetch_t2; 
  m.tcycles[3] = ld_r8_fetch_t3; 
  return m;
}

static const char* ld_r8_imm_mnemonic(u8 opcode) {
  static const char* reg_names[8] = {
    "B","C","D","E","H","L","[HL]","A"
  };
  static char buffer[16];

  u8 dst_idx = (opcode >> 3) & 0x07;

  snprintf(buffer, sizeof(buffer), "LD %s, d8",
           reg_names[dst_idx]);

  return buffer;
}

ResultInstr build_ld_r8_imm(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode       = opcode;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;
  instr.mcycle_count   = 2;

  instr.mnemonic = ld_r8_imm_mnemonic(opcode);

  instr.mcycles[0] = ld_r8_read_imm_cycle_create();
  instr.mcycles[1] = ld_r8_fetch_next_cycle_create();

  return result_ok_Instr(instr);
}


// LD r8, r8
static void ld_r8_r8_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u8 dst_idx = (cpu->IR >> 3) & 0x07;
    u8 src_idx = (cpu->IR) & 0x07;

    u8* dst_reg = cpu_get_reg8_opcode(cpu, dst_idx);
    u8* src_reg = cpu_get_reg8_opcode(cpu, src_idx);

    if (dst_reg && src_reg) *dst_reg = *src_reg;
  }
}

static void ld_r8_r8_fetch_t1(Cpu* cpu, Mem* mem) { ld_r8_fetch_t1(cpu, mem); }
static void ld_r8_r8_fetch_t2(Cpu* cpu, Mem* mem) { ld_r8_fetch_t2(cpu, mem); }
static void ld_r8_r8_fetch_t3(Cpu* cpu, Mem* mem) { ld_r8_fetch_t3(cpu, mem); }

static MCycle ld_r8_r8_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = ld_r8_r8_t0;
  m.tcycles[1] = ld_r8_r8_fetch_t1;
  m.tcycles[2] = ld_r8_r8_fetch_t2;
  m.tcycles[3] = ld_r8_r8_fetch_t3;

  return m;
}

// TODO Would have been a giant switch case, might do something similar for all instrs
static const char* ld_r8_r8_mnemonic(u8 opcode) {
  static const char* reg_names[8] = {
    "B","C","D","E","H","L","[HL]","A"
  };
  static char buffer[16];

  u8 dst_idx = (opcode >> 3) & 0x07;
  u8 src_idx = opcode & 0x07;

  snprintf(buffer, sizeof(buffer), "LD %s, %s",
           reg_names[dst_idx],
           reg_names[src_idx]);

  return buffer;
}

ResultInstr build_ld_r8_r8(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));

  instr.opcode = opcode;
  instr.mcycle_count = 1;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  instr.mnemonic = ld_r8_r8_mnemonic(opcode);

  instr.mcycles[0] = ld_r8_r8_cycle_create();

  return result_ok_Instr(instr);
}
