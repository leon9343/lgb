#ifndef UTIL_H
#define UTIL_H

#include "types.h"
#include <Emulator/cpu/cpu.h>

typedef enum {
  Error_None = 0,
  Error_Unknown,
  Error_NullPointer,
  AppError_SDL_Init,
  AppError_TTF_Init,
  AppError_CreateWindow,
  AppError_CreateRenderer,
  EmuError_InvalidRead,
  EmuError_InstrCreation,
  EmuError_InstrInvalid,
  EmuError_InstrUnimp,
} EAppError;

static inline const char* error_string(int code) {
  switch ((EAppError)code) {
    case Error_None:
      return "No error";
    case Error_NullPointer:
      return "Null pointer";
    case AppError_SDL_Init:
      return "SDL Init failed";
    case AppError_TTF_Init:
      return "TTF Init failed";
    case AppError_CreateWindow:
      return "SDL_CreateWindow failed";
    case AppError_CreateRenderer:
      return "SDL_CreateRenderer failed";
    case EmuError_InvalidRead:
      return "Invalid read";
    case EmuError_InstrCreation:
      return "Invalid arguments";
    case EmuError_InstrUnimp:
      return "Unimplemented instruction";
    case EmuError_InstrInvalid:
      return "Invalid instruction";
    case Error_Unknown:
    default:
      return "Unknown error";
  }
}

// Pin Operations
void pin_set_low(Pin* pin);
void pin_set_high(Pin* pin);
void pin_set_hiz (Pin* pin);
void set_bus_hiz(Cpu* cpu);
void set_addr_bus_value(Cpu* cpu, u16 value);
void set_data_bus_value(Cpu* cpu, u8 value);

#endif // !UTIL_H
