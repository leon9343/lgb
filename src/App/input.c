#include "input.h"
#include "app.h"
#include <Emulator/cpu/cpu.h>
#include <llog.h>

#include <Emulator/cpu/cpu.h>

static EInputCode handle_keydown(struct App *app, SDL_Keycode key) {
  if (key == SDLK_ESCAPE)
    return ICODE_QUIT_ACTIVE;

  switch (app->active_window) {

    case EAppWindow_GameBoy: {
      if (key == SDLK_h) {
        LOG_INFO("[Help]\n"
                 "  'vd'    -> View Diagram\n"
                 "  'vc'    -> View CPU\n"
                 "  'h'     -> Show help\n"
                 "  'space' -> Step one tcycle\n"
                 "  'enter' -> Enable/disable auto-play\n"
                 "  'Esc'   -> Quit active window\n");
        return ICODE_SHOW_HELP;
      } else if (key == SDLK_SPACE) {
        if (app->paused) {
          return ICODE_STEP;
        } else {
          app->paused   = true;
          app->auto_run = false;
          return ICODE_AUTO;
        }
      } else if (key == SDLK_RETURN) {
        if (!app->auto_run) {
          app->paused   = false;
          app->auto_run = true;
          app->cpu->paused = false;
        } else {
          app->paused   = true;
          app->auto_run = false;
        }
        return ICODE_AUTO;
      } else {
        static bool pressed_v = false;
        if (key == SDLK_v) {
          pressed_v = true;
        } else if (key == SDLK_d && pressed_v) {
          pressed_v = false;
          return ICODE_OPEN_DIAGRAM;
        } else if (key == SDLK_c && pressed_v) {
          pressed_v = false;
          return ICODE_OPEN_CPU;
        } else {
          pressed_v = false;
        }
      }
    } break;

    default:
      break;
  }

  return ICODE_NONE;
}

void handle_input(struct App* app) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      app->running = false;
      return;
    }
    if (e.type == SDL_WINDOWEVENT || e.type == SDL_KEYDOWN) {
      u32 window_id = e.window.windowID;
      if (window_id == SDL_GetWindowID(app->gameboy_window.window))
        app->active_window = EAppWindow_GameBoy;
      else if (app->diagram_window_open &&
          window_id == SDL_GetWindowID(app->diagram_window.window))
        app->active_window = EAppWindow_Diagram;
      else if (app->cpu_window_open &&
          window_id == SDL_GetWindowID(app->cpu_window.window))
        app->active_window = EAppWindow_Cpu;
    }

    if (e.type == SDL_KEYDOWN) {
      EInputCode code = handle_keydown(app, e.key.keysym.sym);
      switch (code) {
        case ICODE_QUIT_ACTIVE: {
          if (app->active_window == EAppWindow_GameBoy) {
            app->running = false;
          } else if (app->active_window == EAppWindow_Diagram) {
            SDL_DestroyWindow(app->diagram_window.window);
            app->diagram_window_open = false;
            app->active_window = EAppWindow_GameBoy;
          } else if (app->active_window == EAppWindow_Cpu) {
            SDL_DestroyWindow(app->cpu_window.window);
            app->cpu_window_open = false;
            app->active_window = EAppWindow_GameBoy;
          }
        } break;

        case ICODE_OPEN_DIAGRAM: {
          if (!app->diagram_window_open) {
            // This can also be a helper if you prefer
            WindowProps props = {
              .title  = "Diagram",
              .x      = SDL_WINDOWPOS_CENTERED,
              .y      = SDL_WINDOWPOS_CENTERED,
              .width  = 1600,
              .height = 1100,
              .flags  = 0
            };
            ResultWindow rw = window_create(&props);
            if (result_Window_is_err(&rw)) {
              LOG_ERROR("Failed to open diagram window: %s", rw.message);
            } else {
              app->diagram_window = result_Window_get_data(&rw);
              app->diagram_window_open = true;
              app->active_window = EAppWindow_Diagram;
            }
          } else {
            app->active_window = EAppWindow_Diagram;
          }
        } break;

        case ICODE_OPEN_CPU: {
          if (!app->cpu_window_open) {
            WindowProps props = {
              .title  = "CPU Registers",
              .x      = SDL_WINDOWPOS_CENTERED,
              .y      = SDL_WINDOWPOS_CENTERED,
              .width  = 320,
              .height = 200,
              .flags  = 0
            };
            ResultWindow rw = window_create(&props);
            if (result_Window_is_err(&rw)) {
              LOG_ERROR("Failed to open CPU window: %s", rw.message);
            } else {
              app->cpu_window = result_Window_get_data(&rw);
              app->cpu_window_open = true;
              app->active_window = EAppWindow_Cpu;
            }
          } else {
            app->active_window = EAppWindow_Cpu;
          }
        } break;

        case ICODE_STEP: {
          SDL_LockMutex(app->cpu_mutex);
          cpu_clock_tick(app->cpu);
          SDL_UnlockMutex(app->cpu_mutex);
        } break;

        default:
          break;
      }
    }
  }
}
