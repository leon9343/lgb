#include "ld_r16.h"
#include "Emulator/cpu/cpu.h"
#include "Emulator/cpu/instruction.h"
#include <Emulator/mem.h>
#include <types.h>
#include <util.h>
#include <llog.h>
#include <string.h>

//
// ld_r16_imm
//

// Read low
static void ld_r16_readlow_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r16_readlow_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

static void ld_r16_readlow_t2(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_high(&cpu->pin_RD);
  }
}

static void ld_r16_readlow_t3(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
    cpu->temp_l = cpu->data_value;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r16_read_low_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = ld_r16_readlow_t0;
  m.tcycles[1] = ld_r16_readlow_t1;
  m.tcycles[2] = ld_r16_readlow_t2;
  m.tcycles[3] = ld_r16_readlow_t3;

  return m;
}

// Read high
static void ld_r16_readhigh_t0(Cpu* cpu, Mem* mem) {
  (void)mem;
  if (cpu->clock_phase == CLOCK_RISING) {
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r16_readhigh_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

static void ld_r16_readhigh_t2(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_high(&cpu->pin_RD);
  }
}

static void ld_r16_readhigh_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
    cpu->temp_h = cpu->data_value;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r16_read_high_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = ld_r16_readhigh_t0;
  m.tcycles[1] = ld_r16_readhigh_t1;
  m.tcycles[2] = ld_r16_readhigh_t2;
  m.tcycles[3] = ld_r16_readhigh_t3;

  return m;
}

// Load/store 
static void ld_r16_store_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u16* reg = cpu_get_reg16_opcode(cpu, cpu->IR);

    if (reg) {
      *reg = (u16)((cpu->temp_h << 8) | (cpu->temp_l));
      set_addr_bus_value(cpu, *reg);
    }
  } 
}

static void ld_r16_store_t1(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r16_store_t2(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

static void ld_r16_store_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_MCS);
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r16_store_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = ld_r16_store_t0;
  m.tcycles[1] = ld_r16_store_t1;
  m.tcycles[2] = ld_r16_store_t2;
  m.tcycles[3] = ld_r16_store_t3;

  return m;
}

ResultInstr build_ld_r16_imm(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode         = opcode;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;
  instr.mcycle_count   = 3;

  switch (opcode) {
    case 0x01: instr.mnemonic = "LD BC, d16"; break;
    case 0x11: instr.mnemonic = "LD DE, d16"; break;
    case 0x21: instr.mnemonic = "LD HL, d16"; break;
    case 0x31: instr.mnemonic = "LD SP, d16"; break;
    default:
      return result_err_Instr(
        EmuError_InstrInvalid,
        "Invalid opcode for LD r16, d16: 0x%02X", opcode
      );
  }

  instr.mcycles[0] = ld_r16_read_low_cycle_create();
  instr.mcycles[1] = ld_r16_read_high_cycle_create();
  instr.mcycles[2] = ld_r16_store_cycle_create();

  return result_ok_Instr(instr);
}

//
// ld_r16mem_a
//

static void ld_r16mem_a_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    set_data_bus_value(cpu, cpu->registers[AF].bytes.h);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r16mem_a_t1(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u16* reg_ptr = cpu_get_reg16mem_opcode(cpu, cpu->IR);
    set_addr_bus_value(cpu, *reg_ptr);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_WR);
  }
}

static void ld_r16mem_a_t2(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    mem_write8(mem, cpu, cpu->addr_value, cpu->data_value);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_high(&cpu->pin_WR);
  }
}

static void ld_r16mem_a_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u8 op_type = (cpu->IR >> 4) & 0x02;
    bool inc_dec = (op_type & 0x02) != 0;

    if (inc_dec) {
      if ((op_type & 0x01) == 0)
        cpu->registers[HL].v++;
      else 
        cpu->registers[HL].v--;
    }
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle ld_r16mem_a_cycle_create() {
  MCycle m = mcycle_new(4, true);

  m.tcycles[0] = ld_r16mem_a_t0;
  m.tcycles[1] = ld_r16mem_a_t1;
  m.tcycles[2] = ld_r16mem_a_t2;
  m.tcycles[3] = ld_r16mem_a_t3;

  return m;
}

ResultInstr build_ld_r16mem_a(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode         = opcode;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;
  instr.mcycle_count   = 2;

  switch (opcode) {
    case 0x02: instr.mnemonic = "LD (BC), A"; break;
    case 0x12: instr.mnemonic = "LD (DE), A"; break;
    case 0x22: instr.mnemonic = "LD (HL+), A"; break;
    case 0x32: instr.mnemonic = "LD (HL-), A"; break;
    default:
      return result_err_Instr(
        EmuError_InstrInvalid,
        "Invalid opcode for LD r16, d16: 0x%02X", opcode
      );
  }

  instr.mcycles[0] = ld_r16mem_a_cycle_create();
  instr.mcycles[1] = fetch_cycle_create();

  return result_ok_Instr(instr);
}
