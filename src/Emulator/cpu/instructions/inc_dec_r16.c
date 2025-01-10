#include "inc_dec_r16.h"
#include <types.h>
#include <util.h>
#include <string.h>

static void (*g_fn)(Cpu*, Mem*) = NULL;

void inc_dec_t(Cpu* cpu, Mem* mem) {

  if (cpu->clock_phase == CLOCK_RISING) 
    if (g_fn) g_fn(cpu, mem);
}

static void inc_BC(Cpu* cpu, Mem* _mem) { 
  (void)_mem; 
  set_addr_bus_value(cpu, cpu->registers[BC].v);
  cpu->registers[BC].v++; 
}

static void inc_DE(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[DE].v);
  cpu->registers[DE].v++;
}

static void inc_HL(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[HL].v);
  cpu->registers[HL].v++;
}

static void inc_SP(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[SP].v);
  cpu->registers[SP].v++;
}

static void dec_BC(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[BC].v);
  cpu->registers[BC].v--;
}

static void dec_DE(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[DE].v);
  cpu->registers[DE].v--;
}

static void dec_HL(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[HL].v);
  cpu->registers[HL].v--;
}

static void dec_SP(Cpu* cpu, Mem* _mem) {
  (void)_mem;
  set_addr_bus_value(cpu, cpu->registers[SP].v);
  cpu->registers[SP].v--;
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

MCycle inc_dec_16_cycle_create(void (*op)(Cpu*, Mem*)) {
  g_fn = op;
  MCycle m = mcycle_new(false, 4);

  m.tcycles[0] = idle_t;
  m.tcycles[1] = inc_dec_t;
  m.tcycles[2] = idle_t;
  m.tcycles[3] = idle_reset_bus_t;

  return m;
}

