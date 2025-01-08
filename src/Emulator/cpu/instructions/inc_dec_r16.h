#ifndef INC_R16_H
#define INC_R16_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

// Creates an Instruction inc_rr with the specified register depending on the opcode
ResultInstr build_inc_dec_r16(u8 opcode);

#endif
