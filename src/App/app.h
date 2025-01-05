#ifndef APP_H
#define APP_H

#include "window.h"
#include <Emulator/cpu/cpu.h>
#include <Emulator/mem.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <lresult.h>

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

  Cpu* cpu;
  Mem* mem;

  TTF_Font* font;
} App;

DEFINE_RESULT_TYPE(App, App);

// Creates new instance of app
ResultApp app_create();

// Clears app resources
void app_destroy(App* app);

// Main loop of the application
Result app_run(App* app);

#endif // !APP_H
