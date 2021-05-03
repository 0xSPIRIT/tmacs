#ifndef MODELINE_H_
#define MODELINE_H_

#include <SDL2/SDL.h>

#include "buffer.h"

void modeline_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buffer);

#endif  /* ENDIF  */
