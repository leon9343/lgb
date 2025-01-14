#ifndef LD_R8_H
#define LD_R8_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

ResultInstr build_ld_r8_imm(u8 opcode);

ResultInstr build_ld_r8_r8(u8 opcode); 

#endif
