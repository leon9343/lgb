#include "instruction.h"
#include <lresult.h>
#include <util.h>
#include <llog.h>
#include <string.h>

ResultInstr build_inc_r16(u8 opcode);

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

Result instruction_step(Cpu *cpu, Mem *mem, Instruction *instruction) {
  if (!cpu || !mem || !instruction) 
    return result_error(Error_NullPointer, "null pointer to instruction_step. cpu: %d | mem: %d | instruction: %d", cpu, mem, instruction);

  if (instruction_is_complete(instruction))
    return result_ok();

  MCycle* mc = &instruction->mcycles[instruction->current_mcycle];
  if (!mc) {
    return result_error(Error_NullPointer, "null mcycle pointer in instruction_step. current_mcycle: %d", instruction->current_mcycle);
  }

  if (instruction->current_tcycle < mc->tcycle_count) {
    TCycle_fn fn = mc->tcycles[instruction->current_tcycle];
    if (fn) {
      fn(cpu, mem, instruction->current_tcycle);
    } else {
      return result_error(Error_NullPointer, "null tcycle pointer in instruction_step. current_tcycle: %d", instruction->current_tcycle);
    }

    instruction->current_tcycle++;

    if (instruction->current_tcycle >= mc->tcycle_count) {
      instruction->current_mcycle++;
      instruction->current_tcycle = 0;
    }
  }

  return result_ok();
}
