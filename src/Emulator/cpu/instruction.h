#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <lresult.h>
#include <types.h>
#include "cpu.h"
#include <Emulator/mem.h>

#define MAX_TCYCLES 8
#define MAX_MCYCLES 8

typedef void (*TCycle_fn)(Cpu* cpu, Mem* mem, int tcycle_idx);

typedef struct {
  TCycle_fn tcycles[MAX_TCYCLES];
  int tcycle_count;

  bool uses_memory;
} MCycle;

typedef struct {
  MCycle mcycles[MAX_MCYCLES];
  int mcycle_count;

  u8 opcode;
  const char* mnemonic;

  int current_mcycle;
  int current_tcycle;
} Instruction;

DEFINE_RESULT_TYPE(Instruction, Instr);

ResultInstr instruction_create(u8 opcode, const char* mnemonic, const MCycle* cycles, int mc_count);

ResultInstr instruction_decode(u8 opcode);
bool instruction_is_complete(const Instruction* instr);
Result instruction_step(Cpu* cpu, Mem* mem, Instruction* instruction);

#endif // !INSTRUCTION_H
