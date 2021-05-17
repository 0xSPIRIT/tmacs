#include "mark.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "buffer.h"

void mark_start(int sx, int sy) {
    cbuf->mark.on = true;
    cbuf->mark.sx = sx;
    cbuf->mark.ex = sx;
    cbuf->mark.sy = sy;
    cbuf->mark.ey = sy;
}

void mark_update(int ex, int ey) {
    cbuf->mark.ey = ey;
    cbuf->mark.ex = ex;
}

void mark_reset() {
    cbuf->mark.on = false;
}

void mark_draw(SDL_Renderer *renderer, TTF_Font *font) {
    int char_w, char_h;

    int ssx = cbuf->mark.sx;
    int ssy = cbuf->mark.sy;
    int sex = cbuf->mark.ex;
    int sey = cbuf->mark.ey;

    TTF_SizeText(font, "-", &char_w, &char_h);

    if (cbuf->mark.ey < cbuf->mark.sy) {
        int tsx = cbuf->mark.sx;
        int tsy = cbuf->mark.sy;

        cbuf->mark.sx = cbuf->mark.ex;
        cbuf->mark.sy = cbuf->mark.ey;
        cbuf->mark.ex = tsx;
        cbuf->mark.ey = tsy;
    }
    SDL_Rect *mark_rect = tcalloc((cbuf->mark.ey - cbuf->mark.sy) + 1, sizeof(SDL_Rect));
    
    for (int y = cbuf->mark.sy; y <= cbuf->mark.ey; ++y) {
        if (y == cbuf->mark.sy && cbuf->mark.ey - cbuf->mark.sy >= 1) {
            mark_rect[y - cbuf->mark.sy].x = cbuf->mark.sx * char_w;
            mark_rect[y - cbuf->mark.sy].y = y * char_h;
            mark_rect[y - cbuf->mark.sy].w = window_width;
            mark_rect[y - cbuf->mark.sy].h = char_h;
        } else if (y == cbuf->mark.sy && (cbuf->mark.ey - cbuf->mark.sy) == 0) {
            mark_rect[y - cbuf->mark.sy].x = cbuf->mark.sx * char_w;
            mark_rect[y - cbuf->mark.sy].y = y * char_h;
            mark_rect[y - cbuf->mark.sy].w = (cbuf->mark.ex - cbuf->mark.sx) * char_w;
            mark_rect[y - cbuf->mark.sy].h = char_h;
        } else if (y == cbuf->mark.ey && cbuf->mark.ey - cbuf->mark.sy >= 1) {
            mark_rect[y - cbuf->mark.sy].x = 0;
            mark_rect[y - cbuf->mark.sy].y = y * char_h;
            mark_rect[y - cbuf->mark.sy].w = cbuf->mark.ex * char_w;
            mark_rect[y - cbuf->mark.sy].h = char_h;
        } else {
            mark_rect[y - cbuf->mark.sy].x = 0;
            mark_rect[y - cbuf->mark.sy].y = y * char_h;
            mark_rect[y - cbuf->mark.sy].w = window_width;
            mark_rect[y - cbuf->mark.sy].h = char_h;
        }

        mark_rect[y - cbuf->mark.sy].x += cbuf->x;
        mark_rect[y - cbuf->mark.sy].y += cbuf->y;

        mark_rect[y - cbuf->mark.sy].y -= cbuf->yoff;

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &mark_rect[y - cbuf->mark.sy]);
    }

    cbuf->mark.sx = ssx;
    cbuf->mark.sy = ssy;
    cbuf->mark.ex = sex;
    cbuf->mark.ey = sey;
}

void mark_copy() {
    /* char *s; */
    /* int len; */

    /* if (cbuf->mark.sx < cbuf->mark.ex) { */
    /*     s = tcalloc(cbuf->mark.ex - cbuf->mark.sx + 5, 1); */
    /*     strncpy(s, cbuf->lines[cbuf->mark.y].string + cbuf->mark.sx, cbuf->mark.ex - cbuf->mark.sx); */
    /*     SDL_SetClipboardText(s); */
    /* } */

    mark_reset();
}

void mark_kill() {
    /* char *s; */

    /* if (cbuf->mark.ex < cbuf->mark.sx) { */
    /*     int temp = cbuf->mark.ex; */
    /*     cbuf->mark.ex = cbuf->mark.sx; */
    /*     cbuf->mark.sx = temp; */
    /* } */
    
    /* s = tcalloc(cbuf->mark.ex - cbuf->mark.sx + 2, 1); */
    /* strncpy(s, cbuf->lines[cbuf->mark.y].string + cbuf->mark.sx, cbuf->mark.ex - cbuf->mark.sx); */
    /* SDL_SetClipboardText(s); */

    /* LOG(s); */
    
    /* for (int i = 0; i < (cbuf->mark.ex - cbuf->mark.sx); ++i) { */
    /*     line_cut_char(cbuf->lines + cbuf->mark.y, cbuf->mark.sx); */
    /* } */

    /* point_x = cbuf->mark.sx; */
    /* point_y = cbuf->mark.y; */
    
    mark_reset();
}
