#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <lresult.h>

struct App;

typedef enum {
  ICODE_NONE = 0,
  ICODE_SHOW_HELP,
  ICODE_OPEN_DIAGRAM,
  ICODE_OPEN_CPU,
  ICODE_QUIT_ACTIVE,
  ICODE_UNKNOWN
} EInputCode;

EInputCode handle_input(struct App* app, SDL_Event* e);

#endif // !INPUT_H
