#ifndef CB_H
#define CB_H

#include <types.h>
#include <Emulator/cpu/instruction.h>

ResultInstr build_cb();

ResultInstr build_cb_rot(u8 opcode); 
ResultInstr build_cb_bit(u8 opcode);  
ResultInstr build_cb_set(u8 opcode);   
ResultInstr build_cb_res(u8 opcode);    

#endif // !CB_H
