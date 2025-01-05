#include "input.h"
#include "app.h"
#include <llog.h>

#include <Emulator/cpu/cpu.h>

EInputCode handle_input(struct App *app, SDL_Event *e) {
  if (!app) return ICODE_NONE;

  switch (e->type) {
    case SDL_QUIT:
      return ICODE_QUIT_ACTIVE;

    case SDL_KEYDOWN: {
      SDL_KeyCode key = e->key.keysym.sym;

      if (key == SDLK_ESCAPE)
        return ICODE_QUIT_ACTIVE;

      switch (app->active_window) {
        case EAppWindow_GameBoy: {
          if (key == SDLK_h) {
            LOG_INFO("[Help]\n"
                     "  'vd' -> View Diagram\n"
                     "  'vc' -> View CPU\n"
                     "  'h'  -> Show help\n"
                     "  Esc  -> Quit active window\n");
            return ICODE_SHOW_HELP;
          } 

          else if (key == SDLK_SPACE) {
            if (app->paused) 
              cpu_step(app->cpu);
            else {
              app->paused = true;
              app->auto_run = false;
            }
          }

          else if (key == SDLK_RETURN) {
            if (!app->auto_run) {
              app->paused = false;
              app->auto_run = true;
            } else {
              app->auto_run = false;
              app->paused = true;
            }
          }

          else {
            static bool pressed_v = false;
            if (key == SDLK_v) {
              pressed_v = true;
            }
            else if (key == SDLK_d && pressed_v) {
              pressed_v = false;
              return ICODE_OPEN_DIAGRAM;
            }
            else if (key == SDLK_c && pressed_v) {
              pressed_v = false;
              return ICODE_OPEN_CPU;
            }
            else {
              pressed_v = false;
            }
          }
        }

        case EAppWindow_Diagram:
          break;

        default:
          break;
      }
    }
    default: 
      break;
  }

  return ICODE_NONE;
}
