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
      return build_ld_r8(opcode);

    // 16-bit INC/DEC
    case 0x03: case 0x13: case 0x23: case 0x33:
    case 0x08: case 0x18: case 0x28: case 0x38:
      return build_inc_dec_r16(opcode);

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

  // Call TCycle only during RISING or HIGH
  if (cpu->clock_phase == CLOCK_RISING || cpu->clock_phase == CLOCK_HIGH) {
    fn(cpu, mem);
  }

  // Advance to next T-cycle on FALLING edge
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

