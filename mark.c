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
    char *s;
    int len = 0;
    int i = 0;

    if (cbuf->mark.ey < cbuf->mark.sy) {
        int temp = cbuf->mark.ey;
        int temp2 = cbuf->mark.ex;
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.ex = cbuf->mark.sx;
        cbuf->mark.sy = temp;
        cbuf->mark.sx = temp2;
    }
    if (cbuf->mark.sy == cbuf->mark.ey && cbuf->mark.ex < cbuf->mark.sx) {
        int temp = cbuf->mark.ex;
        cbuf->mark.ex = cbuf->mark.sx;
        cbuf->mark.sx = temp;
    }
    
    int x = cbuf->mark.sx, y = cbuf->mark.sy;

    while (!(x == cbuf->mark.ex && y == cbuf->mark.ey)) {
        x++;
        if (x == cbuf->lines[y].length+1) {
            ++y;
            x=0;
        }
        
        ++len;
    }

    s = tcalloc(len+1, 1); 
    x = cbuf->mark.sx, y = cbuf->mark.sy;
   
    while (!(x == cbuf->mark.ex && y == cbuf->mark.ey)) {
        x++;
        if (x == cbuf->lines[y].length+1) {
            ++y;
            x=0;
            s[i++] = '\n';
        } else {
            s[i++] = cbuf->lines[y].string[x-1];
        }
    }

    SDL_SetClipboardText(s);

    cbuf->mark.on = false;
}

void mark_kill() {
    mark_copy();
    cbuf->mark.on = true;

    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;
    while (!(point_x == cbuf->mark.ex && point_y == cbuf->mark.ey)) {
        line_cut_char(cbuf->lines+point_y, point_x);
        if (point_y == cbuf->mark.ey) cbuf->mark.ex--;
        
        if (point_x == cbuf->lines[point_y].length && point_y != cbuf->mark.ey) {
            char *str = tcalloc(strlen(cbuf->lines[point_y+1].string)+1, 1);
                        
            strcpy(str, cbuf->lines[point_y+1].string);

            buffer_cutline(point_y+1);

            line_insert_str(cbuf->lines + point_y, str);

            free(str);

            point_x = 0;
            cbuf->mark.ey--;
        }
    }

    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;
    
    cbuf->mark.on = false;
}
