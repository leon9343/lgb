#include "app.h"
#include "App/window.h"
#include <Emulator/cpu/cpu.h>
#include <Emulator/mem.h>
#include "input.h"
#include <SDL_ttf.h>
#include <llog.h>
#include <lresult.h>
#include <util.h>
#include <string.h>


static SDL_Color COLOR_GRAY  = {128, 128, 128, 255};
static SDL_Color COLOR_GREEN = {0,   255, 0,   255};
static SDL_Color COLOR_WHITE = {255, 255, 255, 255};

// Window Creation/Destruction
static Result create_gameboy_window(App* app);
static Result create_diagram_window(App* app);
static void   close_diagram_window(App* app);
static Result create_cpu_window(App* app);
static void   close_cpu_window(App* app);
static int emulation_thread_func(void* data);

// Helper function for diagram window
static void draw_text(SDL_Renderer* r, TTF_Font* font, int x, int y, const char* text, SDL_Color color);
static SDL_Color pin_color(EPinState state);
static void draw_pin_label(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* pin);
static void draw_bus_value(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* bus_pins, int bit_count);
static void draw_signal_line(SDL_Renderer* r, int x, int y, int width, bool high);
static void draw_clock_cycle(SDL_Renderer* r, int x, int y, int width);

static void draw_timing_diagram(SDL_Renderer* r, TTF_Font* font, App* app);
static void draw_cpu_diagram(SDL_Renderer* r, TTF_Font* font, App* app);
static void draw_cpu_registers(SDL_Renderer* r, TTF_Font* font, Cpu* cpu);

void update_timing_history(App* app) {
  TimingPoint current = {
    .phase     = app->cpu->clock_phase,
    .mem_read  = app->cpu->pin_RD.state == PIN_LOW,
    .mem_write = app->cpu->pin_WR.state == PIN_LOW,
    .mem_addr  = app->cpu->addr_value
  };

  app->timing_history[app->timing_history_pos] = current;
  app->timing_history_pos = (app->timing_history_pos + 1) % MAX_TIMING_HISTORY;
}

ResultApp app_create() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    return result_err_App(AppError_SDL_Init, "SDL init failed: %s", SDL_GetError());
  }
  if (TTF_Init() != 0) {
    return result_err_App(AppError_TTF_Init, "TTF init failed: %s", TTF_GetError());
  }

  App app;
  memset(&app, 0, sizeof(app));
  app.running = true;
  app.paused = true;
  app.auto_run = false;
  app.resources_valid = true;

  app.cpu_mutex = SDL_CreateMutex();
  app.timing_mutex = SDL_CreateMutex();

  WindowProps props = {
    .title  = "GameBoy",
    .x      = SDL_WINDOWPOS_CENTERED,
    .y      = SDL_WINDOWPOS_CENTERED,
    .width  = 320,
    .height = 288,
    .flags  = 0
  };

  app.font = TTF_OpenFont("/usr/share/fonts/open-sans/OpenSans-Regular.ttf", 14);
  if (!app.font) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(AppError_TTF_Init, "Failed to open font");
  }

  Result res_gb = create_gameboy_window(&app);
  if (result_is_error(&res_gb)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_gb.error_code,
                          "Could not create GameBoy window: %s",
                          error_string(res_gb.error_code));   
  }
  app.active_window = EAppWindow_GameBoy;

  app.mem = malloc(sizeof(Mem));
  if (!app.mem) {
    return result_err_App(Error_NullPointer, "No mem for Mem struct");
  }
  Result res_mem = mem_init(app.mem);
  if (result_is_error(&res_mem)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_mem.error_code,
                          "Could not create mem instance: %s",
                          error_string(res_mem.error_code));    
  }

  app.cpu = malloc(sizeof(Cpu));
  if (!app.cpu) {
    return result_err_App(Error_NullPointer, "No mem for Cpu struct");
  }
  Result res_cpu = cpu_init(app.cpu, app.mem);
  if (result_is_error(&res_cpu)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_cpu.error_code,
                          "Could not create cpu instance: %s",
                          error_string(res_cpu.error_code));    
  }

  Result res_load = cpu_load_bootrom(app.cpu);
  if (result_is_error(&res_load)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_load.error_code,
                          "Failed to load bootrom: %s",
                          error_string(res_load.error_code));
  }

  LOG_TRACE("app created successfully");
  return result_ok_App(app);
}

void app_destroy(App *app) {
  if (!app) return;

  SDL_LockMutex(app->cpu_mutex);
  app->should_quit = true;
  app->resources_valid = false;
  SDL_UnlockMutex(app->cpu_mutex);

  if (app->thread_inititalized) {
    while (app->thread_running)
      SDL_Delay(1);
    int thread_return;
    SDL_WaitThread(app->emulation_thread, &thread_return);
    app->thread_inititalized = false;
  }

  if (app->font) {
    TTF_CloseFont(app->font);
    app->font = NULL;
  }

  close_diagram_window(app);
  close_cpu_window(app);
  window_destroy(&app->gameboy_window);

  if (app->cpu) {
    free(app->cpu);
    app->cpu = NULL;
  }

  if (app->mem) {
    free(app->mem);
    app->mem = NULL;
  }

  SDL_DestroyMutex(app->timing_mutex);
  SDL_DestroyMutex(app->cpu_mutex);

  SDL_Quit();
  TTF_Quit();

  LOG_TRACE("app destroyed successfully");
}

Result app_run(App* app) {
  if (!app) {
    return result_error(Error_NullPointer, "Null App pointer in app_run");
  }

  if (!app->thread_inititalized) {
    app->emulation_thread = SDL_CreateThread(emulation_thread_func, 
                                             "EmulationThread", 
                                             app);
    app->thread_inititalized = (app->emulation_thread != NULL);
  }

  while (app->running) {
    handle_input(app);

    if (app->auto_run && !app->paused) {
      SDL_LockMutex(app->timing_mutex);
      update_timing_history(app);
      SDL_UnlockMutex(app->timing_mutex);
    }

    SDL_SetRenderDrawColor(app->gameboy_window.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->gameboy_window.renderer);
    window_draw(&app->gameboy_window);

    if (app->diagram_window_open) {
      SDL_SetRenderDrawColor(app->diagram_window.renderer, 40, 40, 40, 255);
      SDL_RenderClear(app->diagram_window.renderer);

      SDL_LockMutex(app->cpu_mutex);
      SDL_LockMutex(app->timing_mutex);
      draw_cpu_diagram(app->diagram_window.renderer, app->font, app);
      SDL_UnlockMutex(app->timing_mutex);
      SDL_UnlockMutex(app->cpu_mutex);

      window_draw(&app->diagram_window);
    }

    if (app->cpu_window_open) {
      SDL_SetRenderDrawColor(app->cpu_window.renderer, 40, 40, 40, 255);
      SDL_RenderClear(app->cpu_window.renderer);

      draw_cpu_registers(app->cpu_window.renderer, app->font, app->cpu);

      window_draw(&app->cpu_window);
    }

    SDL_Delay(16);
  }

  return result_ok();
}

static Result create_gameboy_window(App* app) {
  if (!app) {
    return result_error(Error_NullPointer, "Null App pointer in create_gameboy_window");
  }

  WindowProps props;
  memset(&props, 0, sizeof(props));
  props.title  = "GameBoy";
  props.x      = SDL_WINDOWPOS_CENTERED;
  props.y      = SDL_WINDOWPOS_CENTERED;
  props.width  = 320;
  props.height = 288;
  props.flags  = 0;

  ResultWindow rw = window_create(&props);
  if (result_Window_is_err(&rw)) {
    return result_error(rw.error_code, rw.message);
  }

  Window window = result_Window_get_data(&rw);
  app->gameboy_window = window;

  return result_ok();
}

static Result create_diagram_window(App* app) {
  if (!app) {
    return result_error(Error_NullPointer, "Null App pointer in create_diagram_window");
  }

  WindowProps props;
  memset(&props, 0, sizeof(props));
  props.title  = "Diagram";
  props.x      = SDL_WINDOWPOS_CENTERED;
  props.y      = SDL_WINDOWPOS_CENTERED;
  props.width  = 1600;
  props.height = 1100;
  props.flags  = 0;

  ResultWindow rw = window_create(&props);
  if (result_Window_is_err(&rw)) {
    return result_error(rw.error_code, rw.message);
  }

  Window window = result_Window_get_data(&rw);
  app->diagram_window = window;
  app->diagram_window_open = true;

  return result_ok();
}

static Result create_cpu_window(App* app) {
  if (!app) {
    return result_error(Error_NullPointer, "Null App pointer in create_cpu_window");
  }

  WindowProps props;
  memset(&props, 0, sizeof(props));
  props.title  = "CPU Registers";
  props.x      = SDL_WINDOWPOS_CENTERED;
  props.y      = SDL_WINDOWPOS_CENTERED;
  props.width  = 320;
  props.height = 200;
  props.flags  = 0;

  ResultWindow rw = window_create(&props);
  if (result_Window_is_err(&rw)) {
    return result_error(rw.error_code, rw.message);
  }

  Window window = result_Window_get_data(&rw);
  app->cpu_window = window;
  app->cpu_window_open = true;

  return result_ok();
}

static void close_diagram_window(App* app) {
  if (!app->diagram_window_open) {
    return;
  }

  window_destroy(&app->diagram_window);
  app->diagram_window_open = false;
}

static void close_cpu_window(App* app) {
  if (!app->cpu_window_open) {
    return;
  }

  window_destroy(&app->cpu_window);
  app->cpu_window_open = false;
}

static int emulation_thread_func(void* data) {
  App* app = (App*)data;
  u64 last_time = SDL_GetTicks64();
  u64 cycles = 0;
  app->thread_running = true;

  while (!app->should_quit && app->resources_valid) {
    if (app->auto_run && !app->paused) {
      SDL_LockMutex(app->cpu_mutex);

      if (!app->resources_valid) {
        SDL_UnlockMutex(app->cpu_mutex);
        break;
      }
      cpu_clock_tick(app->cpu);

      SDL_LockMutex(app->timing_mutex);
      update_timing_history(app);
      SDL_UnlockMutex(app->timing_mutex);

      SDL_UnlockMutex(app->cpu_mutex);

      cycles++;
      u64 now = SDL_GetTicks64();
      if (now - last_time >= 1000) {
        app->cycles_per_second = cycles;
        cycles = 0;
        last_time = now;
      }
    } else {
      SDL_Delay(1);  
    }
  }

  app->thread_running = false;
  return 0;
}


static void draw_text(SDL_Renderer* r, TTF_Font* font, int x, int y, const char* text, SDL_Color color) {
  if (!text || !font) {
    LOG_WARNING("Invalid font/text in draw_text. Doing nothing");
    return;
  }

  SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
  if (!surface) {
    LOG_WARNING("Invalid surface in draw_text. Doing nothing");
    return;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(r, surface);
  SDL_FreeSurface(surface);
  if (!texture) {
    LOG_WARNING("Invalid texture in draw_text. Doing nothing");
    return;
  }

  SDL_Rect dst;
  dst.x = x;
  dst.y = y;
  TTF_SizeText(font, text, &dst.w, &dst.h);
  SDL_RenderCopy(r, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
}

static SDL_Color pin_color(EPinState state) {
  return state == PIN_HIGH ? COLOR_GREEN : COLOR_GRAY;
}

static void draw_pin_label(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* pin) {
  SDL_Color c = pin_color(pin->state);
  draw_text(r, font, x, y, pin->name, c);
}

static void draw_bus_value(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* bus_pins, int bit_count) {
  unsigned val = 0;
  for (int i = 0; i < bit_count; i++) {
    if (bus_pins[i].state == PIN_HIGH)
      val |= (1u << i);
  }

  char buf[16];
  if (bit_count == 16) {
    snprintf(buf, sizeof(buf), "0x%04X", val);
  } else {
    snprintf(buf, sizeof(buf), "0x%02X", val);
  }

  draw_text(r, font, x, y, buf, COLOR_WHITE);
}

static void draw_signal_line(SDL_Renderer* r, int x, int y, int width, bool high) {
  int height = high ? y : y + 20;
  SDL_RenderDrawLine(r, x, height, x + width, height);
}

static void draw_clock_cycle(SDL_Renderer* r, int x, int y, int width) {
  int half = width / 2;
  draw_signal_line(r, x, y, half, true);
  draw_signal_line(r, x + half, y, half, false);

  SDL_RenderDrawLine(r, x, y + 20, x, y);
  SDL_RenderDrawLine(r, x + half, y, x + half, y + 20);
  SDL_RenderDrawLine(r, x + width, y + 20, x + width, y);
}

static void draw_timing_diagram(SDL_Renderer* r, TTF_Font* font, App* app) {
  const int START_X = 500;  
  const int START_Y = 50;
  const int SIGNAL_HEIGHT = 30;
  const int PHASE_WIDTH = 20;  

  draw_text(r, font, START_X - 80, START_Y, "CLK", COLOR_WHITE);
  draw_text(r, font, START_X - 80, START_Y + SIGNAL_HEIGHT, "Mem R/W", COLOR_WHITE);
  draw_text(r, font, START_X - 80, START_Y + SIGNAL_HEIGHT * 2, "Mem Addr", COLOR_WHITE);

  // Draw vertical grid lines for timing reference
  SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
  for (int x = 0; x <= MAX_TIMING_HISTORY; x++) {
    int grid_x = START_X + (x * PHASE_WIDTH);
    SDL_RenderDrawLine(r, grid_x, START_Y - 10, grid_x, START_Y + SIGNAL_HEIGHT * 3);
  }

  for (int i = 0; i < MAX_TIMING_HISTORY - 1; i++) {
    int idx = (app->timing_history_pos + i) % MAX_TIMING_HISTORY;
    int next_idx = (app->timing_history_pos + i + 1) % MAX_TIMING_HISTORY;
    TimingPoint state = app->timing_history[idx];
    TimingPoint next_state = app->timing_history[next_idx];
    int x = START_X + (i * PHASE_WIDTH);

    int y = START_Y;
    int level = 0;
    switch(state.phase) {
      case CLOCK_LOW:
      case CLOCK_FALLING:
        level = 20;  
        break;
      case CLOCK_HIGH:
      case CLOCK_RISING:
        level = 0;   
        break;
    }

    SDL_RenderDrawLine(r, x, y + level, x + PHASE_WIDTH, y + level);

    if (state.phase != next_state.phase) {
      int next_level = (next_state.phase == CLOCK_LOW || 
        next_state.phase == CLOCK_FALLING) ? 20 : 0;
      SDL_RenderDrawLine(r, x + PHASE_WIDTH, y + level, 
                         x + PHASE_WIDTH, y + next_level);
    }

    y = START_Y + SIGNAL_HEIGHT;
    if (state.mem_read)
      draw_text(r, font, x, y - 10, "R", COLOR_GREEN);
    else if (state.mem_write)
      draw_text(r, font, x, y - 10, "W", COLOR_GREEN);

    if (i % 4 == 0) {  
      y = START_Y + SIGNAL_HEIGHT * 2;
      char addr_buf[32];
      snprintf(addr_buf, sizeof(addr_buf), "%04X", state.mem_addr);
      draw_text(r, font, x, y - 10, addr_buf, COLOR_WHITE);
    }
  }
}

static void draw_cpu_diagram(SDL_Renderer* r, TTF_Font* font, App* app) {
  Cpu* cpu = app->cpu;

  if (!cpu) {
    LOG_WARNING("invalid cpu in draw_cpu_diagram. Doing nothing");
    return;
  }

  SDL_Rect cpu_rect = {50, 50, 300, 750};
  SDL_SetRenderDrawColor(r, 80, 80, 200, 255);
  SDL_RenderFillRect(r, &cpu_rect);

  // Label
  const char* label = "LR35902 CPU";
  int text_w, text_h;
  TTF_SizeText(font, label, &text_w, &text_h);
  int label_x = cpu_rect.x + (cpu_rect.w - text_w) / 2;
  int label_y = cpu_rect.y + 5;

  SDL_Color color_white = {255, 255, 255, 255};
  draw_text(r, font, label_x, label_y, label, color_white);

  // Pins
  // Left
  int pin_x = cpu_rect.x + 5; 
  int pin_y = cpu_rect.y + 40;
  int spacing = 20; 

  pin_y += spacing; // X0
  pin_y += spacing; // X1

  pin_y += spacing; 
  for (int i = 0; i < (8 + 13); i++) { // MD/MA
    pin_y += spacing; 
  }
  pin_y += spacing; 

  pin_y += spacing; // MWR
  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_MCS);
  pin_y += spacing; 
  pin_y += spacing; // MOE

  // others...

  // Right
  pin_x = cpu_rect.x + cpu_rect.w - 50; 
  pin_y = cpu_rect.y + 60;
  spacing = 20;

  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_RST);
  pin_y += spacing;
  pin_y += spacing; // SOUT
  pin_y += spacing; // SIN
  pin_y += spacing; // SCX
  pin_y += spacing; // space
  pin_y += spacing; // space
  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_CLK);
  pin_y += spacing; 
  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_WR);
  pin_y += spacing; 
  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_RD);
  pin_y += spacing; 
  pin_y += spacing; // CS

  for (int i = 0; i < 16; i++) {
    draw_pin_label(r, font, pin_x, pin_y, &cpu->addr_bus[i]);
    pin_y += spacing;
  }

  pin_y += spacing; 

  for (int i = 0; i < 8; i++) {
    draw_pin_label(r, font, pin_x, pin_y, &cpu->data_bus[i]);
    pin_y += spacing;
  }

  draw_pin_label(r, font, pin_x, pin_y, &cpu->pin_VIN);
  pin_y += spacing; 
  pin_y += spacing; // LOUT
  pin_y += spacing; // ROUT
  pin_y += spacing; // T1
  pin_y += spacing; // T2

  // Addr/Data
  label_x = cpu_rect.x + 10;
  label_y = cpu_rect.y + cpu_rect.h + 5;

  draw_text(r, font, label_x, label_y, "Addr:", COLOR_WHITE);
  draw_bus_value(r, font, label_x + 50, label_y, cpu->addr_bus, 16);

  label_y += 20;
  draw_text(r, font, label_x, label_y, "Data:", COLOR_WHITE);
  draw_bus_value(r, font, label_x + 50, label_y, cpu->data_bus, 8);

  draw_timing_diagram(r, font, app);
}

static void draw_cpu_registers(SDL_Renderer* renderer, TTF_Font* font, Cpu* cpu) {
  if (!cpu) return;

  SDL_Color color = { 255, 255, 255, 255 };
  int x = 10;
  int y = 10;

#define DRAW_REG(REGNAME, idx) do {\
  char buf[64];\
  u16 val = cpu->registers[idx].v;\
  snprintf(buf, sizeof(buf), #REGNAME " = 0x%04X", val);\
  draw_text(renderer, font, x, y, buf, color);\
  y += 20;\
} while (0)

  DRAW_REG(AF, AF);
  DRAW_REG(BC, BC);
  DRAW_REG(DE, DE);
  DRAW_REG(HL, HL);
  DRAW_REG(SP, SP);
  DRAW_REG(PC, PC);

  #undef DRAW_REG
}
