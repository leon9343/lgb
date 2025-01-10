#ifndef APP_H
#define APP_H

#include "window.h"
#include <Emulator/cpu/cpu.h>
#include <Emulator/mem.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_thread.h>
#include <stdbool.h>
#include <lresult.h>

#define MAX_TIMING_HISTORY 32

typedef struct {
  EClockPhase phase;
  bool mem_read;
  bool mem_write;
  u16 mem_addr;
} TimingPoint;

typedef enum {
  EAppWindow_None = 0,
  EAppWindow_GameBoy,
  EAppWindow_Diagram,
  EAppWindow_Cpu,
} EAppWindow;

typedef struct App {
  bool running;
  bool paused;
  bool auto_run;
  EAppWindow active_window;

  Window gameboy_window;
  Window diagram_window;
  Window cpu_window;
  bool diagram_window_open;
  bool cpu_window_open;

  SDL_Thread* emulation_thread;
  SDL_mutex* cpu_mutex;
  SDL_mutex* timing_mutex;
  bool thread_inititalized;
  volatile bool should_quit; // for thread
  volatile bool thread_running;
  volatile bool resources_valid;

  u64 cycles_per_second;

  TimingPoint timing_history[MAX_TIMING_HISTORY];
  int timing_history_pos;

  Cpu* cpu;
  Mem* mem;

  TTF_Font* font;
} App;

DEFINE_RESULT_TYPE(App, App);

ResultApp app_create();
void app_destroy(App* app);
Result app_run(App* app);

void app_update_timing_history(App* app);

#endif // !APP_H
