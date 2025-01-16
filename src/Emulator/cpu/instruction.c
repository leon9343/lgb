#include "instruction.h"
#include "Emulator/cpu/instructions/instructions.h"
#include <lresult.h>
#include <util.h>
#include <llog.h>
#include <string.h>
#include <types.h>

ResultInstr build_nop(u8 opcode); 

// Simple TCycles
void idle_t(Cpu* cpu, Mem* mem) {
  (void)cpu; (void)mem;
}

void idle_reset_bus_t(Cpu* cpu, Mem* mem) {
  (void)mem;
  set_bus_hiz(cpu);
}

void inc_pc_t(Cpu* cpu, Mem* mem) {
  (void)mem;
  set_addr_bus_value(cpu, cpu->registers[PC].v);
  cpu->registers[PC].v++;
}

ResultInstr instruction_create(u8 opcode, const char* mnemonic,
                               const MCycle* cycles, int mc_count) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));

  if (mc_count > MAX_MCYCLES)
    return result_err_Instr(EmuError_InstrCreation, "too many mcycles");

  instr.opcode       = opcode;
  instr.mnemonic     = mnemonic ? mnemonic : "???";
  instr.mcycle_count = mc_count;

  for (int i = 0; i < mc_count; i++)
    instr.mcycles[i] = cycles[i];

  // initial indices
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  return result_ok_Instr(instr);
}

bool instruction_is_complete(const Instruction* instr) {
  if (!instr) return true;
  return (instr->current_mcycle >= instr->mcycle_count);
}

ResultInstr instruction_decode(u8 opcode) {
  switch (opcode) {
    case 0x00:
      return build_nop(opcode);

    // LD r8, d8
    case 0x06: case 0x16: case 0x26:
    case 0x0E: case 0x1E: case 0x2E: case 0x3E:
      return build_ld_r8_imm(opcode);

    // LD r8, r8
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x47:
    case 0x48: case 0x49: case 0x4A: case 0x4B:
    case 0x4C: case 0x4D: case 0x4F:
    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x57:
    case 0x58: case 0x59: case 0x5A: case 0x5B:
    case 0x5C: case 0x5D: case 0x5F:
    case 0x60: case 0x61: case 0x62: case 0x63:
    case 0x64: case 0x65: case 0x67:
    case 0x68: case 0x69: case 0x6A: case 0x6B:
    case 0x6C: case 0x6D: case 0x6F:
    case 0x78: case 0x79: case 0x7A: case 0x7B:
    case 0x7C: case 0x7D: case 0x7F:
      return build_ld_r8_r8(opcode);

    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: // ADD A,r8
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F: // ADC A,r8
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97: // SUB A,r8
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F: // SBC A,r8
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7: // AND A,r8
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF: // XOR A,r8
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7: // OR A,r8
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF: // CP A,r8
      return build_logic_r8(opcode);

    // LD r16, d16
    case 0x01: case 0x11: case 0x21: case 0x31:
      return build_ld_r16_imm(opcode);

    // LD (r16), A
    case 0x02: case 0x12: case 0x22: case 0x32:
      return build_ld_r16mem_a(opcode);

    // 16-bit INC/DEC
    case 0x03: case 0x13: case 0x23: case 0x33:
    case 0x08: case 0x18: case 0x28: case 0x38:
      return build_inc_dec_r16(opcode);

    // 0xCB prefix
    case 0xCB:
      return build_cb();

    default:
      return result_err_Instr(EmuError_InstrUnimp,
        "unimplemented opcode 0x%02X", opcode);
  }
}

Result instruction_step(Cpu* cpu, Mem* mem, Instruction* instruction) {
  if (!cpu || !mem || !instruction)
    return result_error(Error_NullPointer, "null pointer in instruction_step");

  if (instruction_is_complete(instruction))
    return result_ok();

  MCycle* mc = &instruction->mcycles[instruction->current_mcycle];
  if (!mc)
    return result_error(Error_NullPointer, "null MCycle in instruction_step");

  TCycle_fn fn = mc->tcycles[instruction->current_tcycle];
  if (!fn)
    return result_error(Error_NullPointer, "null TCycle in instruction_step");

  if (cpu->clock_phase == CLOCK_RISING || cpu->clock_phase == CLOCK_HIGH) {
    fn(cpu, mem);
  }

  if (cpu->clock_phase == CLOCK_FALLING) {
    instruction->current_tcycle++;
    if (instruction->current_tcycle >= mc->tcycle_count) {
      instruction->current_mcycle++;
      instruction->current_tcycle = 0;
    }
  }
  return result_ok();
}

// Basic MCycle
MCycle mcycle_new(bool uses_memory, int tcycle_count) {
  MCycle m;
  memset(&m, 0, sizeof(m));
  m.uses_memory   = uses_memory;
  m.tcycle_count  = tcycle_count;
  return m;
}

// Fetch cycles
void fetch_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

void fetch_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

void fetch_t2(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
  }
}

void fetch_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

MCycle fetch_cycle_create() {
  MCycle m = mcycle_new(true, 4);
  m.tcycles[0] = fetch_t0;
  m.tcycles[1] = fetch_t1;
  m.tcycles[2] = fetch_t2;
  m.tcycles[3] = fetch_t3;
  return m;
}

// A simple NOP that just re-fetches next opcode
ResultInstr build_nop(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode       = opcode;
  instr.mnemonic     = "NOP";
  instr.mcycle_count = 1;
  instr.mcycles[0]   = fetch_cycle_create();
  return result_ok_Instr(instr);
}

