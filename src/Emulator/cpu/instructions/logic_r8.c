#include "logic_r8.h"
#include "Emulator/cpu/cpu.h"
#include <Emulator/mem.h>
#include <types.h>
#include <util.h>
#include <llog.h>
#include <string.h>

// Op + fetch
static void logic_r8_op_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u8* reg_ptr = cpu_get_reg8_opcode(cpu, cpu->IR);
    u8* a_ptr = cpu_get_reg8(cpu, A);
    u8 old_a = *a_ptr;
    u8 reg_val = reg_ptr ? *reg_ptr : 0;
    u8 result = 0;

    u8 op_type = (cpu->IR >> 3) & 0x07;
    u8 carry = cpu_get_flag(cpu, FC) ? 1 : 0;
    bool is_arithmetic = op_type < 4;
    bool is_subtract = op_type == 2 || op_type == 3;

    switch (op_type) {
      case 0: result = old_a + reg_val; break;
      case 1: result = old_a + reg_val + carry; break;
      case 2: result = old_a - reg_val; break;
      case 3: result = old_a - reg_val - carry; break;
      case 4: result = old_a & reg_val; break;
      case 5: result = old_a ^ reg_val; break;
      case 6: result = old_a | reg_val; break;
      case 7: result = old_a - reg_val; break;
    }

    cpu_set_flag(cpu, FZ, result == 0);
    cpu_set_flag(cpu, FN, is_subtract);

    if (is_arithmetic) {
      if (is_subtract) {
        u8 op = reg_val + (op_type == 3 ? carry : 0);
        cpu_set_flag(cpu, FH, (old_a & 0x0F) < (op & 0x0F));
        cpu_set_flag(cpu, FC, (u16)old_a < (u16)op);
      } else {
        u8 op = reg_val + (op_type == 1 ? carry : 0);
        cpu_set_flag(cpu, FH, (old_a & 0x0F) + (op & 0x0F) > 0x0F);
        cpu_set_flag(cpu, FC, (u16)old_a + (u16)op > 0xFF);
      }
    } else {
        cpu_set_flag(cpu, FH, op_type == 4);
        cpu_set_flag(cpu, FC, false);
    }

    if (op_type != 7)
      *a_ptr = result;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void logic_r8_op_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
  }
}

static void logic_r8_op_t2(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
  }
}

static void logic_r8_op_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle logic_r8_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = logic_r8_op_t0;
  m.tcycles[1] = logic_r8_op_t1;
  m.tcycles[2] = logic_r8_op_t2;
  m.tcycles[3] = logic_r8_op_t3;

  return m;
}

static const char* logic_r8_mnemonic(u8 opcode) {
  u8 op_type = (opcode >> 3) & 0x07;
  u8 reg_idx = opcode & 0x07;

  static const char* op_names[] = {
    "ADD A,", "ADC A,", "SUB A,", "SBC A,",
    "AND A,", "XOR A,", "OR A,", "CP A,"
  };

  static const char* reg_names[] = {
    "B", "C", "D", "E", "H", "L", "(HL)", "A"
  };

  static char mnemonic[16];

  snprintf(mnemonic, sizeof(mnemonic), "%s%s", 
           op_names[op_type], reg_names[reg_idx]);

  return mnemonic;
}

ResultInstr build_logic_r8(u8 opcode) {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));
  instr.opcode = opcode;
  instr.mcycle_count = 1;
  instr.mnemonic = logic_r8_mnemonic(opcode);

  instr.mcycles[0] = logic_r8_cycle_create();

  return result_ok_Instr(instr);
}
