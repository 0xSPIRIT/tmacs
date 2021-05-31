#include "mark.h"

#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_error.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "buffer.h"

static SDL_Rect mark_rect[MAX_MARK];

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

    int tx = cbuf->mark.ey;
    int ty = cbuf->mark.sy;
    if (cbuf->mark.sy > cbuf->mark.ey) {
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.sy = tx;
    }
    
    cbuf->mark.sy = ty;
    cbuf->mark.ey = tx;
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

void mark_copy(void) {
    if (cbuf->mark.ey < cbuf->mark.sy) {
        int temp = cbuf->mark.ey;
        int temp2 = cbuf->mark.ex;
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.ex = cbuf->mark.sx;
        cbuf->mark.sy = temp;
        cbuf->mark.sx = temp2;
    }
    if (cbuf->mark.ey == cbuf->mark.sy && cbuf->mark.sx > cbuf->mark.ex) {
        int temp = cbuf->mark.sx;
        cbuf->mark.sx = cbuf->mark.ex;
        cbuf->mark.ex = temp;
    }

    size_t cliplen = 0;
    size_t lines = 0;

    do {
        cliplen += cbuf->lines[lines++].length;
    } while (lines < cbuf->mark.ey);

    char *clip = talloc(cliplen);
    size_t i = 0;
        
    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;
    
    while (true) {
        if (point_x < cbuf->lines[point_y].length) {
            clip[i++] = cbuf->lines[point_y].string[point_x];
            point_x++;
        } else if (point_x >= cbuf->lines[point_y].length) {
            clip[i++] = '\n';
            point_y++;
            point_x = 0;
        }

        if (point_x >= cbuf->mark.ex && point_y >= cbuf->mark.ey) break;
    }
    clip[i] = 0;

    SDL_SetClipboardText(clip);
    
    cbuf->mark.on = false;
}

void mark_kill(int char_w, int char_h) {
    if (cbuf->mark.ey < cbuf->mark.sy) {
        int temp = cbuf->mark.ey;
        int temp2 = cbuf->mark.ex;
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.ex = cbuf->mark.sx;
        cbuf->mark.sy = temp;
        cbuf->mark.sx = temp2;
    }

    if (cbuf->mark.ey == cbuf->mark.sy && cbuf->mark.sx > cbuf->mark.ex) {
        int temp = cbuf->mark.sx;
        cbuf->mark.sx = cbuf->mark.ex;
        cbuf->mark.ex = temp;
    }

    size_t cliplen = 0;
    size_t lines = 0;

    do {
        cliplen += cbuf->lines[lines++].length;
    } while (lines < cbuf->mark.ey);

    char *clip = talloc(cliplen);
    size_t i = 0;
        
    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;

    while (true) {
        if (point_x < cbuf->lines[point_y].length) {
            clip[i++] = cbuf->lines[point_y].string[point_x];
            line_cut_char(cbuf->lines + point_y, point_x);
            if (cbuf->mark.sy == cbuf->mark.ey) {
                cbuf->mark.ex--;
            }
        } else if (point_x >= cbuf->lines[point_y].length) {
            if (cbuf->lines[point_y].length == 0) {
                clip[i++] = '\n';
                buffer_cutline(point_y);
                point_x = 0;
                cbuf->mark.ey--;
            } else {
                size_t len;
                char *s;

                len = cbuf->lines[point_y].length;
                s = talloc(len);
                strcpy(s, cbuf->lines[point_y].string);

                buffer_cutline(point_y);
                cbuf->mark.ey--;
                line_insert_str_at(cbuf->lines + point_y, s, 0);
                if (cbuf->mark.sy == cbuf->mark.ey) {
                    cbuf->mark.ex += len;
                }

                clip[i++] = '\n';
                
                point_x = len;
                tfree(s);
            }
        }

        if (point_x >= cbuf->mark.ex && point_y >= cbuf->mark.ey) break;
    }
    clip[i] = 0;

    SDL_SetClipboardText(clip);

    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;
    if (cbuf->desired_yoff > point_y * char_h || cbuf->desired_yoff + window_height < point_y * char_h) {
        cbuf->desired_yoff = point_y * char_h;
    }
    
    cbuf->mark.on = false;
}
