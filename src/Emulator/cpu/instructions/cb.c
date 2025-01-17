#include "cb.h"
#include "Emulator/cpu/cpu.h"
#include "Emulator/cpu/instruction.h"
#include <Emulator/mem.h>
#include <util.h>
#include <string.h>
#include <llog.h>

//
// cb fetch
//
static void cb_fetch_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    // TODO check some stuff ig
    set_addr_bus_value(cpu, cpu->registers[PC].v);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_MCS);
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_WR);
  }
}

static void cb_fetch_t1(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    u8 data = mem_read8(mem, cpu, cpu->registers[PC].v + 1);
    set_data_bus_value(cpu, data);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    pin_set_low(&cpu->pin_RD);
    cpu->registers[PC].v++;
  }
}

static void cb_fetch_t2(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
  }
}

static void cb_fetch_t3(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    pin_set_high(&cpu->pin_MCS);
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    set_bus_hiz(cpu);
  }
}

static MCycle cb_fetch_cycle_create() {
  MCycle m = mcycle_new(true, 4);

  m.tcycles[0] = cb_fetch_t0;
  m.tcycles[1] = cb_fetch_t1;
  m.tcycles[2] = cb_fetch_t2;
  m.tcycles[3] = cb_fetch_t3;

  return m;
}

//
// cb op + fetch
//
static void cb_op_t0(Cpu* cpu, Mem* mem) {
  (void)mem;

  if (cpu->clock_phase == CLOCK_RISING) {
    u8 op_type = (cpu->IR >> 6) & 0x03;
    u8* reg_ptr = cpu_get_reg8_opcode(cpu, cpu->IR);
    u8 bit_pos = (cpu->IR >> 3) & 0x07;
    if (!reg_ptr) return;

    switch (op_type) {
      case 0x00: {
        u8 rop_type = (cpu->IR >> 3) & 0x07;
        bool old_carry = cpu_get_flag(cpu, FC);

        switch (rop_type) {
          case 0x00: 
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x80) != 0);
            *reg_ptr = (*reg_ptr << 1) | (cpu_get_flag(cpu, FC) ? 1 : 0);
            break;

          case 0x01: 
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x01) != 0);
            *reg_ptr = (*reg_ptr >> 1) | (cpu_get_flag(cpu, FC) ? 0x80 : 0);
            break;

          case 0x02:
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x80) != 0);
            *reg_ptr = (*reg_ptr << 1) | (old_carry ? 1 : 0);
            break;

          case 0x03: 
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x01) != 0);
            *reg_ptr = (*reg_ptr >> 1) | (old_carry ? 0x80 : 0);
            break;

          case 0x04:
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x80) != 0);
            *reg_ptr <<= 1;
            break;

          case 0x05:
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x01) != 0);
            *reg_ptr = (*reg_ptr >> 1) | (*reg_ptr & 0x80);
            break;

          case 0x06:
            *reg_ptr = ((*reg_ptr & 0x0F) << 4) | ((*reg_ptr & 0xF0) >> 4);
            cpu_set_flag(cpu, FC, false);
            break;

          case 0x07:
            cpu_set_flag(cpu, FC, (*reg_ptr & 0x01) != 0);
            *reg_ptr >>= 1;
            break;
        }

        cpu_set_flag(cpu, FZ, *reg_ptr == 0);
        cpu_set_flag(cpu, FN, false);
        cpu_set_flag(cpu, FH, false);
      } break;

      case 0x01: {
        cpu_set_flag(cpu, FZ, (*reg_ptr & (1 << bit_pos)) == 0);
        cpu_set_flag(cpu, FN, false);
        cpu_set_flag(cpu, FH, true);
      } break;

      case 0x02: {
        *reg_ptr &= ~(1 << bit_pos);
      } break;

      case 0x03: {
        *reg_ptr |= (1 << bit_pos);
      } break;
    }
  }

}

static void cb_op_t1(Cpu* cpu, Mem* mem) {
  fetch_t0(cpu, mem);
}

static void cb_op_t2(Cpu* cpu, Mem* mem) {
  fetch_t1(cpu, mem);
}

static void cb_op_t3(Cpu* cpu, Mem* mem) {
  if (cpu->clock_phase == CLOCK_RISING) {
    cpu->registers[PC].v++;
  } else if (cpu->clock_phase == CLOCK_HIGH) {
    cpu->IR = cpu->data_value;
    pin_set_high(&cpu->pin_RD);
    pin_set_high(&cpu->pin_MCS);
    set_bus_hiz(cpu);
  }
}

static MCycle cb_op_cycle_create() {
  MCycle m = mcycle_new(4, false);

  m.tcycles[0] = cb_op_t0;
  m.tcycles[1] = cb_op_t1;
  m.tcycles[2] = cb_op_t2;
  m.tcycles[3] = cb_op_t3;

  return m;
}

ResultInstr build_cb() {
  Instruction instr;
  memset(&instr, 0, sizeof(instr));

  instr.mcycle_count = 2;

  instr.mcycles[0] = cb_fetch_cycle_create();
  instr.mcycles[1] = cb_op_cycle_create();

  return result_ok_Instr(instr);
}
