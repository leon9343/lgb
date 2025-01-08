#include "inc_dec_r16.h"
#include "Emulator/cpu/instruction.h"
#include "Emulator/mem.h"
#include <types.h>
#include <util.h>
#include <string.h>

void inc_BC(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[BC].v++; }
void inc_DE(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[DE].v++; }
void inc_HL(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[HL].v++; }
void inc_SP(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[SP].v++; }

void dec_BC(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[BC].v--; }
void dec_DE(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[DE].v--; }
void dec_HL(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[HL].v--; }
void dec_SP(Cpu* cpu, Mem* _mem) { (void)_mem; cpu->registers[SP].v--; }

ResultInstr build_inc_dec_r16(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode = opcode;
  instr.mcycle_count = 2;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  switch (opcode) {
    case 0x03:
      instr.mnemonic = "INC BC";
      instr.mcycles[0] = inc_dec_16_cycle_create(inc_BC);
      break;
    case 0x13:
      instr.mnemonic = "INC DE";
      instr.mcycles[0] = inc_dec_16_cycle_create(inc_DE);
      break;
    case 0x23:
      instr.mnemonic = "INC HL";
      instr.mcycles[0] = inc_dec_16_cycle_create(inc_HL);
      break;
    case 0x33:
      instr.mnemonic = "INC SP";
      instr.mcycles[0] = inc_dec_16_cycle_create(inc_SP);
      break;

    case 0x08:
      instr.mnemonic = "DEC BC";
      instr.mcycles[0] = inc_dec_16_cycle_create(dec_BC);
      break;
    case 0x18:
      instr.mnemonic = "DEC DE";
      instr.mcycles[0] = inc_dec_16_cycle_create(dec_DE);
      break;
    case 0x28:
      instr.mnemonic = "DEC HL";
      instr.mcycles[0] = inc_dec_16_cycle_create(dec_HL);
      break;
    case 0x38:
      instr.mnemonic = "DEC SP";
      instr.mcycles[0] = inc_dec_16_cycle_create(dec_SP);
      break;

    default:
      return result_err_Instr(EmuError_InstrInvalid, "invalid instruction build_inc_r16: %02X", opcode);
  }

  instr.mcycles[1] = fetch_cycle_create();

  return result_ok_Instr(instr);
}
