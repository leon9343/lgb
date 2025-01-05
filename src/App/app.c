#include "app.h"
#include "App/window.h"
#include "Emulator/mem.h"
#include "input.h"
#include <SDL_ttf.h>
#include <llog.h>
#include <lresult.h>
#include <util.h>
#include <string.h>

#include <Emulator/cpu/cpu.h>

static SDL_Color COLOR_GRAY  = {128, 128, 128, 255};
static SDL_Color COLOR_GREEN = {0,   255, 0,   255};
static SDL_Color COLOR_WHITE = {255, 255, 255, 255};

static Result create_gameboy_window(App* app);

static Result create_diagram_window(App* app);
static Result create_cpu_window(App* app);
static void  close_diagram_window(App* app);
static void  close_cpu_window(App* app);

static void _test_program(Mem* mem, Cpu* cpu) {
  // TEST 
  mem_write8(mem, cpu, 0xC0A0, 0x03);
  mem_write8(mem, cpu, 0xC0A1, 0x13);
  mem_write8(mem, cpu, 0xC0A2, 0x23);
  cpu->registers[PC].v = 0xC0A0;
}

ResultApp app_create() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    return result_err_App(AppError_SDL_Init, "failed to init SDL: %s", SDL_GetError());
  }

  if (TTF_Init() != 0) {
    return result_err_App(AppError_TTF_Init, "failed to init TTF: %s", TTF_GetError());
  }

  App app;
  memset(&app, 0, sizeof(app));
  app.running = true;
  app.paused = true;
  app.auto_run = false;
  app.active_window = EAppWindow_None;
  app.diagram_window_open = false;
  app.cpu_window_open = false;

  app.font = TTF_OpenFont("/home/leonardo/.local/share/fonts/JetBrainsMonoNerdFont-Bold.ttf", 14);
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
  Result res_mem = mem_init(app.mem);
  if (result_is_error(&res_mem)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_mem.error_code,
                          "Could not create mem instance: %s",
                          error_string(res_mem.error_code));
  }

  app.cpu = malloc(sizeof(Cpu));
  Result res_cpu = cpu_init(app.cpu, app.mem);
  if (result_is_error(&res_cpu)) {
    SDL_Quit();
    TTF_Quit();
    return result_err_App(res_cpu.error_code,
                          "Could not create cpu instance: %s",
                          error_string(res_cpu.error_code));
  }

  _test_program(app.mem, app.cpu);

  LOG_TRACE("app created successfully");
  return result_ok_App(app);
}

void app_destroy(App *app) {
  if (!app) return;

  if (app->font) {
    TTF_CloseFont(app->font);
    app->font = NULL;
  }

  close_diagram_window(app);
  window_destroy(&app->diagram_window);
  window_destroy(&app->gameboy_window);

  if (app->cpu) {
    free(app->cpu);
    app->cpu = NULL;
  }

  SDL_Quit();
  TTF_Quit();

  LOG_TRACE("app destroyed successfully");
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
  props.width  = 640;
  props.height = 1000;
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

static void handle_events(App* app) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    u32 window_id = (e.type == SDL_WINDOWEVENT || e.type == SDL_KEYDOWN || e.type == SDL_QUIT)
      ? e.window.windowID
      : 0;

    if (window_id == SDL_GetWindowID(app->gameboy_window.window)) {
      app-> active_window = EAppWindow_GameBoy;
    }
    else if (app->diagram_window_open &&
      window_id == SDL_GetWindowID(app->diagram_window.window)) {
      app->active_window = EAppWindow_Diagram;
    }
    else if (app->cpu_window_open &&
      window_id == SDL_GetWindowID(app->cpu_window.window)) {
      app->active_window = EAppWindow_Cpu;
    }


    EInputCode code = handle_input(app, &e);
    switch (code) {
      case ICODE_QUIT_ACTIVE:
        if (app->active_window == EAppWindow_GameBoy) {
          app->running = false;
        } else if (app->active_window == EAppWindow_Diagram) {
          close_diagram_window(app);
          app->active_window = EAppWindow_GameBoy;
        } else if (app->active_window == EAppWindow_Cpu) {
          close_cpu_window(app);
          app->active_window = EAppWindow_GameBoy;
        }
        break;

      case ICODE_OPEN_DIAGRAM: {
        if (!app->diagram_window_open) {
          Result r = create_diagram_window(app);
          if (result_is_error(&r)) {
            LOG_ERROR("failed to open diagram window: %s (%s)",
                      r.message, error_string(r.error_code));
          } else {
            app->active_window = EAppWindow_Diagram;
          }
        } else {
          app->active_window = EAppWindow_Diagram;
        }
      } 
        break;

      case ICODE_OPEN_CPU: {
        if (!app->cpu_window_open) {
          Result r = create_cpu_window(app);
          if (result_is_error(&r)) {
            LOG_ERROR("failed to open cpu window: %s (%s)",
                      r.message, error_string(r.error_code));
          } else {
            app->active_window = EAppWindow_Cpu;
          }
        } else {
          app->active_window = EAppWindow_Cpu;
        }
      } 
        break;

      case ICODE_NONE:
      case ICODE_SHOW_HELP:
      case ICODE_UNKNOWN:
      default:
        break;
    }
  }
}

// Helper function for diagram window
static void draw_text(SDL_Renderer* r, TTF_Font* font, int x, int y, const char* text, SDL_Color color);
static SDL_Color pin_color(EPinState state);
static void draw_pin_label(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* pin);
static void draw_bus_value(SDL_Renderer* r, TTF_Font* font, int x, int y, Pin* bus_pins, int bit_count);

static void draw_cpu_diagram(SDL_Renderer* r, TTF_Font* font, Cpu* cpu);
static void draw_cpu_registers(SDL_Renderer* r, TTF_Font* font, Cpu* cpu);

Result app_run(App* app) {
  if (!app) {
    return result_error(Error_NullPointer, "Null App pointer in app_run");
  }

  while (app->running) {
    handle_events(app);

    if (app->auto_run && !app->paused) 
      cpu_step(app->cpu);

    // Rendering
    SDL_SetRenderDrawColor(app->gameboy_window.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->gameboy_window.renderer);
    window_draw(&app->gameboy_window);

    if (app->diagram_window_open) {
      SDL_SetRenderDrawColor(app->diagram_window.renderer, 40, 40, 40, 255);
      SDL_RenderClear(app->diagram_window.renderer);

      draw_cpu_diagram(app->diagram_window.renderer, app->font, app->cpu);

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

static void draw_cpu_diagram(SDL_Renderer* r, TTF_Font* font, Cpu* cpu) {
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
