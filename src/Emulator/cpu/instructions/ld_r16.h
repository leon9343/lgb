#ifndef LD_R16_H
#define LD_R16_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

ResultInstr build_ld_r16_imm(u8 opcode);

ResultInstr build_ld_r16mem_a(u8 opcode);

#endif // !LD_R16_H
