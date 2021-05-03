#include "mark.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

struct Mark mark = {0};         /* Global mark. This should probably be a per-buffer thing? */

void mark_start(int sx, int sy) {
    mark.on = true;
    mark.start_x = sx;
    mark.start_y = sy;
    mark.end_x = sx;
    mark.end_y = sy;
}

void mark_update(int ex, int ey) {
    mark.end_x = ex;
    mark.end_y = ey;
}

void mark_reset() {
    mark.on = false;
}

void mark_draw(SDL_Renderer *renderer, struct Buffer *buf, TTF_Font *font) {
    SDL_Rect mark_rect;
    int char_w, char_h;

    TTF_SizeText(font, "-", &char_w, &char_h);

    mark_rect.x = mark.start_x * char_w;
    mark_rect.y = mark.start_y * char_h - buf->yoff;
    mark_rect.w = (mark.end_x - mark.start_x) * char_w;
    mark_rect.h = (mark.end_y - mark.start_y + 1) * char_h;

    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &mark_rect);
}

void mark_copy(struct Buffer *buf) {
    char *s;
    
    if (mark.start_x < mark.end_x) {
        s = tcalloc(mark.end_x - mark.start_x + 5, 1);
        strncpy(s, buf->lines[mark.start_y].string + mark.start_x, mark.end_x - mark.start_x);
        SDL_SetClipboardText(s);
    }

    mark_reset();
}

void mark_kill(struct Buffer *buf) {
    char *s;

    if (mark.start_x < mark.end_x) {
        s = tcalloc(mark.end_x - mark.start_x + 1, 1);
        strncpy(s, buf->lines[mark.start_y].string + mark.start_x, mark.end_x - mark.start_x - 1);
        line_cut_str(buf->lines+mark.start_y, mark.start_x, mark.end_x-1);
        SDL_SetClipboardText(s);
        LOG(s);
        free(s);
    } else if (mark.start_x > mark.end_x) {
        s = tcalloc(mark.start_x - mark.end_x + 1, 1);
        strncpy(s, buf->lines[mark.start_y].string + mark.end_x, mark.start_x - mark.end_x-1);
        line_cut_str(buf->lines+mark.start_y, mark.end_x, mark.start_x-1);
        SDL_SetClipboardText(s);
        LOG(s);
        free(s);
    }

    point_x = mark.start_x;
    point_y = mark.start_y;
    
    mark_reset();
}
