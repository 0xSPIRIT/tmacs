#ifndef MARK_H_
#define MARK_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdbool.h>

#define MAX_MARK 8192

struct Mark {
    bool on;
    
    int sx, ex;
    int sy, ey;
};

extern struct Mark mark;

void mark_start(int sx, int sy);
void mark_update(int ex, int ey);
void mark_draw(SDL_Renderer *renderer, TTF_Font *font);
void mark_copy(void);
void mark_kill(int char_w, int char_h);

#endif  /* MARK_H_ */
