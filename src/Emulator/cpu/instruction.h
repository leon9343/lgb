#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <lresult.h>
#include <types.h>
#include "cpu.h"
#include <Emulator/mem.h>

#define MAX_TCYCLES 8
#define MAX_MCYCLES 8

typedef void (*TCycle_fn)(Cpu* cpu, Mem* mem);

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

ResultInstr instruction_create(u8 opcode, const char* mnemonic,
                               const MCycle* cycles, int mc_count);
ResultInstr instruction_decode(u8 opcode);
bool instruction_is_complete(const Instruction* instr);
Result instruction_step(Cpu* cpu, Mem* mem, Instruction* instruction);

// Common MCycles
MCycle mcycle_new(bool uses_memory, int tcycle_count);
MCycle idle_cycle_create();
MCycle fetch_cycle_create();

// Common TCycles
void idle_t(Cpu* cpu, Mem* mem);
void idle_reset_bus_t(Cpu* cpu, Mem* mem);
void inc_pc_t(Cpu* cpu, Mem* mem);

// Fetch TCycles
void fetch_t0(Cpu* cpu, Mem* mem);
void fetch_t1(Cpu* cpu, Mem* mem);
void fetch_t2(Cpu* cpu, Mem* mem);
void fetch_t3(Cpu* cpu, Mem* mem);

#endif

