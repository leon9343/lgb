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

// Creates new instance of Instruction
ResultInstr instruction_create(u8 opcode, const char* mnemonic, const MCycle* cycles, int mc_count);

// Returns an instruction based on the passed opcode
ResultInstr instruction_decode(u8 opcode);

// Returns true if the instruction is done executing, false otherwise
bool instruction_is_complete(const Instruction* instr);

// Steps the instruction by one tcycle
Result instruction_step(Cpu* cpu, Mem* mem, Instruction* instruction);

// MCycles
MCycle idle_cycle_create();                             // idle mcyle (think of nop)
MCycle fetch_cycle_create();                            // fetches new opcode into IR
MCycle inc_dec_16_cycle_create(void (*op)(Cpu*, Mem*));

// tcycles
void fetch_t0(Cpu* cpu, Mem* mem, int t_idx);
void fetch_t1(Cpu* cpu, Mem* mem, int t_idx);
void fetch_t2(Cpu* cpu, Mem* mem, int t_idx);
void fetch_t3(Cpu* cpu, Mem* mem, int t_idx);

void op_high_t(Cpu* cpu, Mem* mem, int t_idx);


#endif // !INSTRUCTION_H
