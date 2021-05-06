#ifndef MARK_H_
#define MARK_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdbool.h>

#include "buffer.h"

struct Mark {
    bool on;
    
    int sx, ex;
    int y;
};

extern struct Mark mark;

void mark_start(int sx, int sy);
void mark_reset();
void mark_update(int ex, int ey);
void mark_draw(SDL_Renderer *renderer, struct Buffer *buf, TTF_Font *font);
void mark_copy(struct Buffer *buf);
void mark_kill(struct Buffer *buf);

#endif  /* MARK_H_ */
