#include "instruction.h"
#include <lresult.h>
#include <util.h>
#include <llog.h>
#include <string.h>
#include <types.h>

ResultInstr build_nop(u8 opcode); // Forward declaration, returns a nop instruction

ResultInstr build_inc_r16(u8 opcode); // from inc_r16.h

// Empty tcycle
void nop(Cpu* cpu, Mem* mem, int t_idx) {
  return;
}

ResultInstr instruction_create(u8 opcode, const char *mnemonic, const MCycle *cycles, int mc_count) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));

  if (mc_count > MAX_MCYCLES)
    return result_err_Instr(EmuError_InstrCreation, "too many mcycles: %d", mc_count);

  instr.opcode       = opcode;
  instr.mnemonic     = mnemonic ? mnemonic : "???";
  instr.mcycle_count = mc_count;

  for (int i = 0; i < mc_count; i++)
    instr.mcycles[i] = cycles[i];

  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  return result_ok_Instr(instr);
}

bool instruction_is_complete(const Instruction *instr) {
  if (!instr) return true;
  return (instr->current_mcycle >= instr->mcycle_count);
}

ResultInstr instruction_decode(u8 opcode) {
  switch (opcode) {
    case 0x00: {
      ResultInstr ri = build_nop(opcode);
      if (result_Instr_is_ok(&ri))
        return result_ok_Instr(result_Instr_get_data(&ri));
      else
        return result_err_Instr(ri.error_code, ri.message);
    }

    case 0x03:
    case 0x13:
    case 0x23:
    case 0x33: {
      ResultInstr ri = build_inc_r16(opcode);
      if (result_Instr_is_ok(&ri))
        return result_ok_Instr(result_Instr_get_data(&ri));
      else
        return result_err_Instr(ri.error_code, ri.message);
    }

    default: 
      return result_err_Instr(EmuError_InstrUnimp, "unimplemented instruction: %02X", opcode);
  }
}

Result instruction_step(Cpu* cpu, Mem* mem, Instruction* instruction) {
  if (!cpu || !mem || !instruction) 
    return result_error(Error_NullPointer, "null pointer to instruction_step. cpu: %d | mem: %d | instruction: %d", cpu, mem, instruction);

  if (instruction_is_complete(instruction))
    return result_ok();

  MCycle* mc = &instruction->mcycles[instruction->current_mcycle];
  if (!mc) 
    return result_error(Error_NullPointer, "null mcycle pointer in instruction_step. current_mcycle: %d", instruction->current_mcycle);

  TCycle_fn fn = mc->tcycles[instruction->current_tcycle];
  if (fn) {
    if (cpu->clock_phase != CLOCK_LOW && cpu->clock_phase != CLOCK_FALLING)
      fn(cpu, mem, instruction->current_tcycle);
  }
  else
    return result_error(Error_NullPointer, "null tcycle pointer in instruction_step. current_tcycle: %d", instruction->current_tcycle);

  if (cpu->clock_phase == CLOCK_FALLING) {
    instruction->current_tcycle++;

    if (instruction->current_tcycle >= mc->tcycle_count) {
      instruction->current_mcycle++;
      instruction->current_tcycle = 0;
    }
  }

  return result_ok();
}

// MCycles //

// Base MCycle
static MCycle mcycle_new(bool uses_memory, int tcycle_count) {
  MCycle m;
  memset(&m, 0, sizeof(m));
  m.uses_memory = uses_memory;
  m.tcycle_count = tcycle_count;

  return m;
}

// General use function. Useful to differentiate between the likes of inc_BC/inc_DE/inc_HL/inc_SP
// I prefer this to a general inc_reg(Register) because it is easier to use with different kinds of instructions, as this can
// declaration can do anything from shifting/adding/subtracting/etc.
static void (*g_fn)(Cpu*, Mem*) = NULL;

// Fetch Cycles (what happens each tcycle) (pin setups are subject to change as I do more research)
static void fetch_T0(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx; 
  u16 addr = cpu->registers[PC].v;

  if (cpu->clock_phase == CLOCK_RISING) {
    for (int i = 0; i < 16; i++) {
      u16 bit = (addr >> i) & 1;
      cpu->addr_bus[i].state = bit ? PIN_HIGH : PIN_LOW;
    }
  }

  else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void fetch_T1(Cpu* cpu, Mem* mem, int t_idx) {
  (void)t_idx;
  u16 addr = cpu->registers[PC].v;

  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, addr);
    for (int i = 0; i < 8; i++) {
      u8 bit = (data >> i) & 1;
      cpu->data_bus[i].state = bit ? PIN_HIGH : PIN_LOW;
    }
  }

  if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
    cpu->IR = mem_read8(mem, cpu, cpu->registers[PC].v);
  }

}

static void fetch_T2(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;

  if (cpu->clock_phase == CLOCK_RISING) 
    pin_set_high(&cpu->pin_RD);

  else if (cpu->clock_phase == CLOCK_HIGH)
    cpu->registers[PC].v++;

}

static void fetch_T3(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;

  if (cpu->clock_phase == CLOCK_RISING) 
    pin_set_high(&cpu->pin_MCS);

  else if (cpu->clock_phase == CLOCK_HIGH)
    set_bus_hiz(cpu);

}

// Increment
static void increment_T0(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;
}

static void increment_T1(Cpu* cpu, Mem* mem, int t_idx) {
  (void)cpu; (void)mem; (void)t_idx;

  if (cpu->clock_phase == CLOCK_HIGH)
    if (g_fn) g_fn(cpu, mem);
}

// MCycle Creation
MCycle idle_cycle_create() {
  MCycle m = mcycle_new(false, 2);

  m.tcycles[0] = nop;
  m.tcycles[1] = nop;

  return m;
}

MCycle fetch_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = fetch_T0;
  m.tcycles[1] = fetch_T1;
  m.tcycles[2] = fetch_T2;
  m.tcycles[3] = fetch_T3;

  return m;
}

MCycle increment_cycle_create(void (*inc)(Cpu*, Mem*)) {
  g_fn = inc;
  MCycle m = mcycle_new(false, 4);

  m.tcycles[0] = increment_T0;
  m.tcycles[1] = increment_T1;
  m.tcycles[2] = increment_T0;
  m.tcycles[3] = increment_T0;

  return m;
}


// Misc Instructions
ResultInstr build_nop(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.mnemonic = "NOP";
  instr.opcode = opcode;
  instr.mcycle_count = 2;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  instr.mcycles[0] = idle_cycle_create();
  instr.mcycles[1] = fetch_cycle_create();

  return result_ok_Instr(instr);
}
