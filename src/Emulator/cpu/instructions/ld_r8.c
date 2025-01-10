#include "ld_r8.h"
#include <Emulator/mem.h>
#include <types.h>
#include <util.h>
#include <llog.h>
#include <string.h>

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
    switch (cpu->IR) {
      case 0x06: cpu->registers[BC].bytes.h = cpu->data_value; break; // B
      case 0x16: cpu->registers[DE].bytes.h = cpu->data_value; break; // D
      case 0x26: cpu->registers[HL].bytes.h = cpu->data_value; break; // H
      case 0x0E: cpu->registers[BC].bytes.l = cpu->data_value; break; // C
      case 0x1E: cpu->registers[DE].bytes.l = cpu->data_value; break; // E
      case 0x2E: cpu->registers[HL].bytes.l = cpu->data_value; break; // L
      case 0x3E: cpu->registers[AF].bytes.h = cpu->data_value; break; // A
    }
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

ResultInstr build_ld_r8(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode       = opcode;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;
  instr.mcycle_count   = 2;

  switch (opcode) {
    case 0x06: instr.mnemonic = "LD B, d8"; break;
    case 0x16: instr.mnemonic = "LD D, d8"; break;
    case 0x26: instr.mnemonic = "LD H, d8"; break;
    case 0x0E: instr.mnemonic = "LD C, d8"; break;
    case 0x1E: instr.mnemonic = "LD E, d8"; break;
    case 0x2E: instr.mnemonic = "LD L, d8"; break;
    case 0x3E: instr.mnemonic = "LD A, d8"; break;
    default:
      return result_err_Instr(EmuError_InstrInvalid,
        "Invalid LD r8 opcode=0x%02X", opcode);
  }

  instr.mcycles[0] = ld_r8_read_imm_cycle_create();
  instr.mcycles[1] = ld_r8_fetch_next_cycle_create();

  return result_ok_Instr(instr);
}

