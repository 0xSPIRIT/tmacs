#include "buffer.h"

#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

int point_x = 0;
int point_y = 0;

struct Buffer *cbuf = NULL;
struct Buffer *minibuf;
struct Buffer *buffers[32] = {0};
bool insert_mode = false;

/* Allocates and instantiates a new text buffer. */
struct Buffer *buffer_new(const char *name, bool minibuf) {
    struct Buffer *b;
    int i;
    
    b = talloc(sizeof(struct Buffer));

    b->capacity = 8;
    b->length = 1;
    b->lines = tcalloc(b->capacity, sizeof(struct Line));
    b->is_minibuf = minibuf;
    b->name = tcalloc(128, 1);

    b->x = 0;
    b->y = 0;

    b->px = 0;
    b->py = 0;
    
    b->yoff = 0;
    b->desired_yoff = 0;
    
    strcpy(b->name, name);

    for (i = 0; i < b->capacity; ++i)
        line_new(&b->lines[i]);
    
    return b;
}

/* Adds a new line to the buffer at point. */
void buffer_newline() {
    int i;
    char *end;

    if (cbuf->is_minibuf) {
        minibuffer_execute_command();
        minibuffer_toggle();
        return;
    }
    
    end = tcalloc(cbuf->lines[point_y].length, 1);
    strcpy(end, cbuf->lines[point_y].string + point_x);
    
    line_cut_str(cbuf->lines + point_y, point_x, cbuf->lines[point_y].length);
    
    cbuf->length++;
    point_y++;

    if (cbuf->length > cbuf->capacity) {
        int add = 5;
        
        cbuf->capacity += add;
        cbuf->lines = realloc(cbuf->lines, sizeof(struct Line) * cbuf->capacity);
        for (i = cbuf->capacity-add; i < cbuf->capacity; ++i) {
            line_new(&cbuf->lines[i]);
        }
    }

    for (i = cbuf->length-1; i > point_y; --i) {
        cbuf->lines[i] = cbuf->lines[i-1];
    }

    line_new(cbuf->lines + point_y);
    point_x = 0;
    
    line_insert_str(cbuf->lines + point_y, end);
        
    point_x = 0;
    free(end);
}

/* Removes a line from the current buffer. */
void buffer_cutline(int k) {
    int i;

    free(cbuf->lines[k].string);
    for (i = k; i < cbuf->length; ++i) {
        cbuf->lines[i] = cbuf->lines[i+1];
    }
    cbuf->length--;
}

/* Draws a buffer to the screen. */
void buffer_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buf) {
    int i;
    SDL_Color color = { 255, 255, 255, 255 };

    int char_w, char_h;
    
    TTF_SizeText(font, "-", &char_w, &char_h);
    
    for (i = 0; i < buf->length; ++i) {
        SDL_Surface *surface;
        SDL_Texture *texture;
        SDL_Rect dst;
        char *str;

        if (i*char_h - buf->yoff < (-char_h+1) || i*char_h - buf->yoff > window_height - char_h) continue;

        str = buf->lines[i].string;

        if (!str[0]) continue;

        surface = TTF_RenderText_Blended(font, str, color);
        texture = SDL_CreateTextureFromSurface(renderer, surface);

        dst.x = buf->x;
        dst.y = buf->y + i * char_h - buf->yoff;
        dst.w = surface->w;
        dst.h = char_h;
        
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

/* Frees a buffer from memory. */
void buffer_free(struct Buffer *buf) {
    int i;

    for (i = 0; i < buf->length; ++i) {
        free(buf->lines[i].string);
    }

    free(buf->name);
    free(buf->lines);
    free(buf);
}

/* Free current buffer, create a new one, then load in contents of a file into the buffer. */
void buffer_load_file(struct Buffer *buf, char *file) {
    FILE *f;
    char *str;
    size_t length;
    long p;

    int ppx = point_x, ppy = point_y;
    
    f = fopen(file, "r");
    if (!f) {
        minibuffer_log("Creating new file.");

        cbuf = buf;
        
        line_cut_str(buf->lines + 0, 0, buf->lines[0].length);
        buf->lines[0].length = 0;
        for (int i = 1; i < buf->length; ++i) {
            line_cut_str(buf->lines + i, 0, buf->lines[i].length);
            buf->lines[i].length = 0;
            free(buf->lines[i].string);
        }
        buf->length = 1;
    
        strcpy(buf->name, file);
        SDL_SetWindowTitle(window, buf->name);

        point_x = 0;
        point_y = 0;
        
        cbuf->yoff = cbuf->desired_yoff = 0;
        return;
    }

    cbuf = buf;
    
    line_cut_str(buf->lines + 0, 0, buf->lines[0].length);
    buf->lines[0].length = 0;
    for (int i = 1; i < buf->length; ++i) {
        line_cut_str(buf->lines + i, 0, buf->lines[i].length);
        buf->lines[i].length = 0;
        free(buf->lines[i].string);
    }
    buf->length = 1;
    
    strcpy(cbuf->name, file);
    SDL_SetWindowTitle(window, cbuf->name);

    point_x = 0;
    point_y = 0;

    insert_mode = false;

    p = ftell(f);
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, p, SEEK_SET);

    str = talloc(length + 1);
    fread(str, 1, length, f);
    str[length] = 0;

    fclose(f);

    while (*str) {
        if (*str == '\n') {
            buffer_newline();
        } else {
            line_insert_char(buf->lines+point_y, *str);
        }
        ++str;
    }

    cbuf->yoff = cbuf->desired_yoff = 0;

    point_x = ppx;
    point_y = ppy;
}

/* Save buffer as a new file. */
void buffer_save_new(struct Buffer *buf, const char *name) {
    strcpy(buf->name, name);
    SDL_SetWindowTitle(window, name);

    buffer_save(buf);
}

/* Save buffer to file. */
void buffer_save(struct Buffer *buf) {
    char msg[80] = {0};
    FILE *f;
    int i;

    f = fopen(buf->name, "wb");
    if (!f) {
        fprintf(stderr, "Error opening file!\n"); fflush(stdout);
        return;
    }

    for (i = 0; i < buf->length; ++i) {
        fputs(buf->lines[i].string, f);
        fputs("\n", f);
    }

    strcat(msg, "Wrote to ");
    strcat(msg, buf->name);
    minibuffer_log(msg);
    
    fclose(f);
}

