#ifndef INC_DEC_R16_H
#define INC_DEC_R16_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

// Creates an Instruction inc_rr with the specified register depending on the opcode
ResultInstr build_inc_dec_r16(u8 opcode);

MCycle inc_dec_16_cycle_create();

#endif
