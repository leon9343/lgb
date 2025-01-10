#ifndef INC_DEC_R16_H
#define INC_DEC_R16_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

// Creates an Instruction inc_rr with the specified register depending on the opcode
ResultInstr build_inc_dec_r16(u8 opcode);

MCycle inc_dec_16_cycle_create(void (*op)(Cpu*, Mem*));

void inc_dec_t(Cpu* cpu, Mem* mem);

#endif
