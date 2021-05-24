#include "mark.h"

#include <SDL2/SDL_clipboard.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "buffer.h"

static SDL_Rect *mark_rect = NULL;

void mark_start(int sx, int sy) {
    cbuf->mark.on = true;
    cbuf->mark.sx = sx;
    cbuf->mark.ex = sx;
    cbuf->mark.sy = sy;
    cbuf->mark.ey = sy;

    if (mark_rect) {
        tfree(mark_rect);
    }
    mark_rect = tcalloc((cbuf->mark.ey - cbuf->mark.sy) + 1, sizeof(SDL_Rect));
}

void mark_update(int ex, int ey) {
    cbuf->mark.ey = ey;
    cbuf->mark.ex = ex;

    int temp = cbuf->mark.ey;
    int temp2 = cbuf->mark.sy;
    if (cbuf->mark.sy > cbuf->mark.ey) {
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.sy = temp;
    }
    
    if (mark_rect) {
        tfree(mark_rect);
    }
    mark_rect = tcalloc((cbuf->mark.ey - cbuf->mark.sy) + 1, sizeof(SDL_Rect));

    cbuf->mark.sy = temp2;
    cbuf->mark.ey = temp;
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

void mark_kill() {
    cbuf->mark.on = true;

    if (cbuf->mark.ey < cbuf->mark.sy) {
        int temp = cbuf->mark.ey;
        int temp2 = cbuf->mark.ex;
        cbuf->mark.ey = cbuf->mark.sy;
        cbuf->mark.ex = cbuf->mark.sx;
        cbuf->mark.sy = temp;
        cbuf->mark.sx = temp2;
    }
    LOG("STARTED");

    if (cbuf->mark.ey == cbuf->mark.sy && cbuf->mark.sx > cbuf->mark.ex) {
        int temp = cbuf->mark.sx;
        cbuf->mark.sx = cbuf->mark.ex;
        cbuf->mark.ex = temp;
    }
    
    int cliplen = 0;
    int q = cbuf->mark.sy;
    
    do {
        cliplen += cbuf->lines[q++].length;
    } while (q < cbuf->mark.ey);

    printf("%d\n", cliplen); fflush(stdout);

    LOG("E");
    char *clip = tcalloc(cliplen, 1);
    LOG("F");
    size_t i=0;
        
    point_x = cbuf->mark.ex;
    point_y = cbuf->mark.ey;

    
#define DO_EOL()                                \
    switch (cbuf->eol) {                        \
    case EOL_LF:                                \
        clip[i++] = '\n';                       \
        break;                                  \
    case EOL_CR:                                \
        clip[i++] = '\r';                       \
        break;                                  \
    case EOL_CRLF:                              \
        clip[i++] = '\n';                       \
        clip[i++] = '\r';                       \
        break;                                  \
    }                                           \

    
    while (true) {
        if (point_x < 0) {
            point_y--;
            point_x = cbuf->lines[point_y].length;
            DO_EOL();
        }

        if (point_x > 0) {
            --point_x;
            clip[i++] = cbuf->lines[point_y].string[point_x];
            line_cut_char(&cbuf->lines[point_y], point_x);
        } else if (point_y > 0 && cbuf->lines[point_y].length == 0) {
            DO_EOL();
            buffer_cutline(point_y--);
            point_x = cbuf->lines[point_y].length;
        } else if (point_y > 0 && point_x == 0) {
            char *s = tcalloc(strlen(cbuf->lines[point_y].string + point_x)+1, 1);
            int px;
            
            DO_EOL();
                        
            strcpy(s, cbuf->lines[point_y].string + point_x);

            buffer_cutline(point_y--);
            point_x = px = cbuf->lines[point_y].length;

            line_insert_str(cbuf->lines + point_y, s);

            point_x = px;

            tfree(s);
        }

        if (point_x == cbuf->mark.sx && point_y == cbuf->mark.sy) {
            break;
        }
    }

    printf("Length: %d\n", i); fflush(stdout);
    char *rev = talloc(i);
    
    for (int c = 0; c < i; ++c) {
        rev[c] = clip[(i-1)-c];
    }
    rev[i] = 0;

    SDL_SetClipboardText(rev);

    point_x = cbuf->mark.sx;
    point_y = cbuf->mark.sy;
    
    cbuf->mark.on = false;
    tfree(mark_rect);
    mark_rect = NULL;
}
