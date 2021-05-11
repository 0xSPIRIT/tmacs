#include "mark.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

struct Mark mark = {0};         /* Global mark. This should probably be a per-buffer thing. */

void mark_start(int sx, int sy) {
    mark.on = true;
    mark.sx = sx;
    mark.ex = sx;
    mark.y = sy;
}

void mark_update(int ex, int ey) {
    mark.ex = ex;
}

void mark_reset() {
    mark.on = false;
}

void mark_draw(SDL_Renderer *renderer, struct Buffer *buf, TTF_Font *font) {
    SDL_Rect mark_rect;
    int char_w, char_h;

    TTF_SizeText(font, "-", &char_w, &char_h);

    mark_rect.x = mark.sx * char_w;
    mark_rect.y = mark.y * char_h - buf->yoff;
    mark_rect.w = (mark.ex - mark.sx) * char_w;
    mark_rect.h = (mark.y - mark.y + 1) * char_h;

    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &mark_rect);
}

void mark_copy(struct Buffer *buf) {
    char *s;
    
    if (mark.sx < mark.ex) {
        s = tcalloc(mark.ex - mark.sx + 5, 1);
        strncpy(s, buf->lines[mark.y].string + mark.sx, mark.ex - mark.sx);
        SDL_SetClipboardText(s);
    }

    mark_reset();
}

void mark_kill(struct Buffer *buf) {
    char *s;

    s = tcalloc(mark.ex - mark.sx + 2, 1);
    strncpy(s, buf->lines[mark.y].string + mark.sx, mark.ex - mark.sx);
    SDL_SetClipboardText(s);

    LOG(s);
    
    for (int i = 0; i < (mark.ex - mark.sx); ++i) {
        line_cut_char(buf->lines + mark.y, mark.sx);
    }

    point_x = mark.sx;
    point_y = mark.y;
    
    mark_reset();
}
