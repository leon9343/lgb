#include "cpu.h"
#include "Emulator/cpu/instruction.h"
#include <Emulator/cpu/instructions/instructions.h>
#include <util.h>
#include <lresult.h>
#include <llog.h>
#include <stdio.h>
#include <string.h>

static Instruction g_currentInstr;
static bool g_hasInstr = false;

static void init_pin(Pin *pin, const char *name, EPinType type, EPinState default_state) {
  memset(pin, 0, sizeof(*pin));
  if (name) {
    strncpy(pin->name, name, sizeof(pin->name) - 1);
  }
  pin->type = type;
  pin->state = default_state;
}

Result cpu_init(Cpu* cpu, Mem* mem) {
  memset(cpu, 0, sizeof(*cpu));

  for (int i = 0; i < 16; i++) {
    char buf[8];
    snprintf(buf, sizeof(buf), "A%d", i);
    init_pin(&cpu->addr_bus[i], buf, PIN_INOUT, PIN_HIGHZ);
  }
  for (int i = 0; i < 8; i++) {
    char buf[8];
    snprintf(buf, sizeof(buf), "D%d", i);
    init_pin(&cpu->data_bus[i], buf, PIN_INOUT, PIN_HIGHZ);
  }

  init_pin(&cpu->pin_X0,  "X0",  PIN_INPUT,  PIN_LOW); 
  init_pin(&cpu->pin_X1,  "X1",  PIN_INPUT,  PIN_LOW);

  /* Memory signals: MWR, MCS, MOE => typical external SRAM lines 
     /MWR => write enable, /MCS => chip select, /MOE => output enable 
     The real GB might label them differently, but we follow your struct. */
  init_pin(&cpu->pin_MWR, "/MWR", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_MCS, "/MCS", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_MOE, "/MOE", PIN_OUTPUT, PIN_HIGH);

  /* Possibly LCD or other lines: LD1, LD0, CPG, CP, ST, CPL, FR, S 
     We label them as output or input depending on usage. For now, default = HIGH. */
  init_pin(&cpu->pin_LD1, "LD1", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_LD0, "LD0", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_CPG, "CPG", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_CP,  "CP",  PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_ST,  "ST",  PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_CPL, "CPL", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_FR,  "FR",  PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_S,   "S",   PIN_OUTPUT, PIN_HIGH);

  /* RST, SOUT, SIN, SCX might be reset, serial out, serial in, etc. */
  init_pin(&cpu->pin_RST,  "/RST", PIN_INPUT,  PIN_HIGH);
  init_pin(&cpu->pin_SOUT, "SOUT", PIN_OUTPUT, PIN_LOW);
  init_pin(&cpu->pin_SIN,  "SIN",  PIN_INPUT,  PIN_LOW);
  init_pin(&cpu->pin_SCX,  "SCX",  PIN_OUTPUT, PIN_LOW); // e.g. serial clock?

  /* CPU main clock and read/write lines */
  init_pin(&cpu->pin_CLK, "CLK", PIN_INPUT, PIN_LOW);
  init_pin(&cpu->pin_WR,  "/WR", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_RD,  "/RD", PIN_OUTPUT, PIN_HIGH);
  init_pin(&cpu->pin_CS,  "/CS", PIN_OUTPUT, PIN_HIGH); // Possibly chip select for something

  /* Power lines: VIN, LOUT, ROUT, T1, T2, VDD, VSS0/1 
     Some might be analog lines or timers. We default them as power or input. */
  init_pin(&cpu->pin_VIN,  "VIN",  PIN_POWER,  PIN_HIGH);
  init_pin(&cpu->pin_LOUT, "LOUT", PIN_OUTPUT, PIN_LOW);
  init_pin(&cpu->pin_ROUT, "ROUT", PIN_OUTPUT, PIN_LOW);
  init_pin(&cpu->pin_T1,   "T1",   PIN_INPUT,  PIN_LOW);
  init_pin(&cpu->pin_T2,   "T2",   PIN_INPUT,  PIN_LOW);

  init_pin(&cpu->pin_VDD,  "VDD",   PIN_POWER, PIN_HIGH);
  init_pin(&cpu->pin_VSS0, "VSS0",  PIN_POWER, PIN_LOW);
  init_pin(&cpu->pin_VSS1, "VSS1",  PIN_POWER, PIN_LOW);

  for (int i = 0; i < 6; i++) {
    cpu->registers[i].v = 0;
  }
  cpu->registers[AF].v = 0x01B0;
  cpu->registers[BC].v = 0x0013;
  cpu->registers[DE].v = 0x00D8;
  cpu->registers[HL].v = 0x014D;
  cpu->registers[SP].v = 0xFFFE;
  cpu->registers[PC].v = 0x0100;
  cpu->IR = 0x00;

  cpu->interrupt_enable = 0x00;
  cpu->interrupt_flag   = 0xE0;

  cpu->bootrom_mapped = false;

  Result rppu = ppu_init(&cpu->ppu);
  if (result_is_error(&rppu)) {
    return result_error(rppu.error_code,
                        "failed to init ppu: %s", rppu.message);
  }

  Result rapu = apu_init(&cpu->apu);
  if (result_is_error(&rapu)) {
    return result_error(rapu.error_code,
                        "failed to init apu: %s", rapu.message);
  }

  cpu->mem = mem;

  LOG_TRACE("cpu initialized successfully");
  return result_ok();
}

Result cpu_step(Cpu* cpu) {
  if (!cpu) {
    return result_error(Error_NullPointer, "invalid cpu to cpu_step");
  }

  if (!g_hasInstr) {
    ResultInstr rdec = instruction_decode(cpu->IR);
    if (result_Instr_is_err(&rdec)) {
      LOG_WARNING("UNIMPLEMENTED OPCODE 0x%02X at PC=0x%04X", cpu->IR, cpu->registers[PC].v);
      g_hasInstr = false;
      return result_error(rdec.error_code, "Decode Error: %s", rdec.message);
    }

    g_currentInstr = result_Instr_get_data(&rdec);
    g_hasInstr     = true;

    LOG_INFO("INSTRUCTION: (%s) at PC=0x%04X", g_currentInstr.mnemonic, cpu->registers[PC].v);
  }

  Result r = instruction_step(cpu, cpu->mem, &g_currentInstr);
  if (result_is_error(&r)) {
    LOG_ERROR("Error stepping instruction: %s", r.message);
    return r;
  }

  if (instruction_is_complete(&g_currentInstr))
    g_hasInstr = false;

  return result_ok();
}
