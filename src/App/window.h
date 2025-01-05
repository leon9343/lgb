#ifndef WINDOW_H
#define WINDOW_H

#include <SDL2/SDL.h>
#include <lresult.h>
#include <types.h>

typedef struct {
  const char* title;
  int x;
  int y;
  int width;
  int height;
  u32 flags;
} WindowProps;

typedef struct {
  SDL_Window* window;
  SDL_Renderer* renderer;
} Window;

DEFINE_RESULT_TYPE(Window, Window);

// Creates new window instance with the passed props
ResultWindow window_create(const WindowProps* props);

// Clears window resources
void window_destroy(Window* window);

// Draws the current state of the window (might make something more elaborate later)
void window_draw(Window* window);

#endif // !WINDOW_H
