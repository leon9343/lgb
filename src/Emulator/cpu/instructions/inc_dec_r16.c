#include "inc_dec_r16.h"
#include <types.h>
#include <util.h>
#include <string.h>

static void inc_dec_r16_t(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u16* reg_ptr = cpu_get_reg16_opcode(cpu, cpu->IR);
    if (!reg_ptr) return;

    set_addr_bus_value(cpu, *reg_ptr);

    u8 low = cpu->IR & 0x0F;
    bool is_inc = (low == 0x03);

    if (is_inc) 
      (*reg_ptr)++;
    else
      (*reg_ptr)--;
  }
}

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
      break;
    case 0x13:
      instr.mnemonic = "INC DE";
      break;
    case 0x23:
      instr.mnemonic = "INC HL";
      break;
    case 0x33:
      instr.mnemonic = "INC SP";
      break;
    case 0x08:
      instr.mnemonic = "DEC BC";
      break;
    case 0x18:
      instr.mnemonic = "DEC DE";
      break;
    case 0x28:
      instr.mnemonic = "DEC HL";
      break;
    case 0x38:
      instr.mnemonic = "DEC SP";
      break;

    default:
      return result_err_Instr(EmuError_InstrInvalid, "invalid instruction build_inc_r16: %02X", opcode);
  }

  instr.mcycles[1] = fetch_cycle_create();

  return result_ok_Instr(instr);
}

MCycle inc_dec_16_cycle_create() {
  MCycle m = mcycle_new(false, 4);

  m.tcycles[0] = idle_t;
  m.tcycles[1] = inc_dec_r16_t;
  m.tcycles[2] = idle_t;
  m.tcycles[3] = idle_reset_bus_t;

  return m;
}

