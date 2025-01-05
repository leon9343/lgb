#include "window.h"
#include <SDL_video.h>
#include <llog.h>
#include <util.h>

ResultWindow window_create(const WindowProps *props) {
  if (!props) {
    return result_err_Window(Error_NullPointer, 
                             "Invalid props to window_create");
  }

  SDL_Window* sdl_window = SDL_CreateWindow(
    props->title,
    props->x,
    props->y,
    props->width,
    props->height,
    props->flags
  );

  if (!sdl_window) {
    return result_err_Window(AppError_CreateWindow,
                             "SDL_CreateWindow failed: %s",
                             SDL_GetError());
  }

  SDL_Renderer* sdl_renderer = SDL_CreateRenderer(
    sdl_window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if (!sdl_renderer) {
    return result_err_Window(AppError_CreateRenderer,
                             "SDL_CreateRenderer failed: %s",
                             SDL_GetError());
  }

  Window window;
  window.window = sdl_window;
  window.renderer = sdl_renderer;

  LOG_TRACE("Window created successfully");
  return result_ok_Window(window);
}

void window_destroy(Window* window) {
  if (!window) return;

  if (window->renderer) {
    SDL_DestroyRenderer(window->renderer);
    window->renderer = NULL;
  }
  if (window->window) {
    SDL_DestroyWindow(window->window);
    window->window = NULL;
  }
  window = NULL;

  LOG_TRACE("Window destroyed successfully");
}

void window_draw(Window *window) {
  if (window && window->renderer) {
    SDL_RenderPresent(window->renderer);
  }
}
