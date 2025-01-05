#include "inc_r16.h"
#include "Emulator/cpu/instruction.h"
#include "Emulator/mem.h"
#include <types.h>
#include <util.h>
#include <string.h>

// Function to increment the 16 bit register. It is set based on the opcode
static void (*g_inc_fn)(Cpu*) = NULL;

static void pin_set_low(Pin* pin)  { pin->state = PIN_LOW;  }
static void pin_set_high(Pin* pin) { pin->state = PIN_HIGH; }
static void pin_set_hiz (Pin* pin) { pin->state = PIN_HIGHZ; }

static void set_bus_hiz(Cpu* cpu) {
  for (int i = 0; i < 16; i++) {
    pin_set_hiz(&cpu->addr_bus[i]);
  }
  for (int i = 0; i < 8; i++) {
    pin_set_hiz(&cpu->data_bus[i]);
  }
}

// M1: Fetch
static void M1_fetch_T0(Cpu* cpu, Mem* mem, int t_idx) {
  (void)t_idx; (void)mem;

  u16 address = cpu->registers[PC].v;
  for (int i = 0; i < 16; i++) {
    u16 bit = (address >> i) & 1;
    cpu->addr_bus[i].state = bit ? PIN_HIGH : PIN_LOW;
  }

  pin_set_low(&cpu->pin_MCS);
  pin_set_high(&cpu->pin_RD);
  pin_set_high(&cpu->pin_WR);

  for (int i = 0; i < 8; i++) 
    pin_set_hiz(&cpu->data_bus[i]);
}

static void M1_fetch_T1(Cpu* cpu, Mem* mem, int t_idx) {
  (void)t_idx;
  pin_set_low(&cpu->pin_RD); // read from memory

  u16 address = cpu->registers[PC].v;
  u8 data = mem_read8(mem, cpu, address);

  // Put opcode bits on data bus
  for (int i = 0; i < 8; i++) {
    u8 bit = (data >> i) & 1;
    cpu->data_bus[i].state = bit ? PIN_HIGH : PIN_LOW;
  }
}

static void M1_fetch_T2(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;
  pin_set_high(&cpu->pin_RD); // read is done

  cpu->registers[PC].v++;
}

static void M1_fetch_T3(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;
  pin_set_high(&cpu->pin_MCS); // end memory cycle

  // address bus back to normal
  for (int i = 0; i < 16; i++) 
    cpu->addr_bus[i].state = PIN_HIGHZ;

  for (int i = 0; i < 8; i++) 
    cpu->data_bus[i].state = PIN_HIGHZ;

  pin_set_high(&cpu->pin_RD);
  pin_set_high(&cpu->pin_WR);
}

// M2: increment
static void M2_exec_inc_T0(Cpu* cpu, Mem* mem, int t_idx) {
  (void)mem; (void)t_idx;
  if (g_inc_fn) 
    g_inc_fn(cpu);
}

static void M2_exec_inc_T1(Cpu* cpu, Mem* mem, int t_idx) {
  (void)cpu; (void)mem; (void)t_idx;
  // IDLE
}

static void inc_BC(Cpu* cpu) { cpu->registers[BC].v++; }
static void inc_DE(Cpu* cpu) { cpu->registers[DE].v++; }
static void inc_HL(Cpu* cpu) { cpu->registers[HL].v++; }
static void inc_SP(Cpu* cpu) { cpu->registers[SP].v++; }

ResultInstr build_inc_r16(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode = opcode;
  instr.mcycle_count = 2;
  instr.current_mcycle = 0;
  instr.current_tcycle = 0;

  switch (opcode) {
    case 0x03:
      g_inc_fn = inc_BC;
      instr.mnemonic = "INC BC";
      break;
    case 0x13:
      g_inc_fn = inc_DE;
      instr.mnemonic = "INC DE";
      break;
    case 0x23:
      g_inc_fn = inc_HL;
      instr.mnemonic = "INC HL";
      break;
    case 0x33:
      g_inc_fn = inc_SP;
      instr.mnemonic = "INC SP";
      break;
    default:
      return result_err_Instr(EmuError_InstrInvalid, "invalid instruction: %02X", opcode);
  }

  // Fetch
  MCycle m1;
  memset(&m1, 0, sizeof(m1));
  m1.uses_memory  = true;
  m1.tcycle_count = 4;
  m1.tcycles[0]   = M1_fetch_T0;
  m1.tcycles[1]   = M1_fetch_T1;
  m1.tcycles[2]   = M1_fetch_T2;
  m1.tcycles[3]   = M1_fetch_T3;

  // Increment
  MCycle m2;
  memset(&m2, 0, sizeof(m2));
  m2.uses_memory  = false;
  m2.tcycle_count = 2;
  m2.tcycles[0]   = M2_exec_inc_T0;
  m2.tcycles[1]   = M2_exec_inc_T1;

  instr.mcycles[0] = m1;
  instr.mcycles[1] = m2;

  return result_ok_Instr(instr);
}
