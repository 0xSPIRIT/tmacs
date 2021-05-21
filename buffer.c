#include "buffer.h"

#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "util.h"
#include "lisp.h"

int point_x = 0;
int point_y = 0;

bool insert_mode = false;

struct Buffer *cbuf = NULL;
struct Buffer *minibuf;
struct Buffer *buffers[BUFFERS_MAX] = {0};

int buffers_count = 1;
int buffer_index = 0;
int buffer_previous_index = 0;

/* Switches buffer_index to i. Does not set cbuf. */
void buffer_switch(int i) {
    buffer_previous_index = buffer_index;
    buffer_index = i;
    point_x = buffers[buffer_index]->px;
    point_y = buffers[buffer_index]->py;
}

/* Kills current buffer. */
void buffer_kill() {
    if (buffers_count == 1) return;
    
    buffer_free(buffers[buffer_index]);

    for (int i = buffer_index; i < buffers_count; ++i) {
        buffers[i] = buffers[i+1];
    }
    buffers_count--;

    if (buffer_index >= buffers_count) buffer_index = buffers_count-1;

    cbuf = buffers[buffer_index];
    point_x = cbuf->px;
    point_y = cbuf->py;
         
    if (buffer_previous_index >= buffers_count) buffer_previous_index = buffers_count-1;
}

/* Allocates and instantiates a new text buffer. */
struct Buffer *buffer_new(const char *name, bool minibuf) {
    struct Buffer *b;
    int i;
    
    b = talloc(sizeof(struct Buffer));

    b->capacity = 8;
    b->length = 1;
    b->lines = tcalloc(b->capacity, sizeof(struct Line));
    b->is_minibuf = minibuf;
    b->name = tcalloc(FNAME_MAX, 1);
    
    b->eol = EOL_UNKNOWN;

    b->x = 0;
    b->y = 0;

    b->px = 0;
    b->py = 0;
    
    b->yoff = 0;
    b->desired_yoff = 0;

    memset(&b->mark, 0, sizeof(struct Mark));

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

        if (!*str) continue;

        if (!draw_text_blended) {
            surface = TTF_RenderText_Solid(font, str, color);
        } else {
            surface = TTF_RenderText_Blended(font, str, color);
        }
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

/* Clears the lines of a buffer */
void buffer_reset(struct Buffer *buf) {
    line_cut_str(buf->lines + 0, 0, buf->lines[0].length);
    buf->lines[0].length = 0;
    for (int i = 1; i < buf->length; ++i) {
        line_cut_str(buf->lines + i, 0, buf->lines[i].length);
        buf->lines[i].length = 0;
        free(buf->lines[i].string);
    }
    buf->length = 1;
}

/* Loads a file's content into buf. Assumes buf is empty. */
void buffer_load_file(struct Buffer *buf, char *file) {
    FILE *f;
    char *str;

    f = fopen(file, "rb");
    if (!f) {
        minibuffer_log("Creating new file.");

        //        buffer_reset(buf);
        
        cbuf = buf;
        
        strcpy(buf->name, file);
        SDL_SetWindowTitle(window, buf->name);

        point_x = 0;
        point_y = 0;
        
        cbuf->yoff = cbuf->desired_yoff = 0;
        return;
    } else {
        fclose(f);
        buf->eol = buffer_find_eol_sequence(buf);
        f = fopen(file, "rb");
    }

    cbuf = buf;
    
    //    buffer_reset(buf);
    
    strcpy(cbuf->name, file);
    SDL_SetWindowTitle(window, cbuf->name);

    point_x = 0;
    point_y = 0;

    insert_mode = false;

    str = read_file(f, NULL);
        
    fclose(f);

    while (*str) {
        if (*str == '\r' && *(str+1) == '\n') { ++str; continue; }
        
        if (*str == '\n' || *str == '\r') {
            buffer_newline();
        } else {
            line_insert_char(buf->lines+point_y, *str);
        }
        ++str;
    }

    cbuf->yoff = cbuf->desired_yoff = 0;

    point_x = 0;
    point_y = 0;
}

/* Save buffer as a new file. */
void buffer_save_new(struct Buffer *buf, const char *name) {
    strcpy(buf->name, name);
    SDL_SetWindowTitle(window, name);

    buffer_save(buf);
}

int buffer_find_eol_sequence(struct Buffer *buf) {
    FILE *f;
    char *str;
    
    int ending = EOL_LF;
    
    f = fopen(buf->name, "rb");
    /* Figure out EOL character sequence. */
    if (f) {
        size_t length;
        
        str = read_file(f, &length);
        
        for (size_t i = 0; i < length; ++i) {
            if (str[i] == '\r' && str[i+1] != '\n') {
                ending = EOL_CR;
                break;
            }
            if (str[i] == '\r' && str[i+1] == '\n') {
                ending = EOL_CRLF;
                break;
            }
            if (str[i] == '\n') {
                ending = EOL_LF;
                break;
            }
        }

        free(str);
    }
    fclose(f);

    {
        char s[16] = {0};
        switch (ending) {
        case EOL_UNKNOWN: strcpy(s, "Unknown"); break;
        case EOL_CR: strcpy(s, "CR"); break;
        case EOL_LF: strcpy(s, "LF"); break;
        case EOL_CRLF: strcpy(s, "CRLF"); break;
        }
        
        printf("End of line sequence: %s\n", s); fflush(stdout);
    }

    return ending;
}

/* Save buffer to file. */
void buffer_save(struct Buffer *buf) {
    char msg[80] = {0};
    int i;

    if (buf->name[0] == '*') {
        if (cbuf != minibuf) minibuffer_toggle();
        
        minibuffer_log("write-to ");
        point_x = minibuf->lines[0].length;
        return;
    }

    if (buf->eol == EOL_UNKNOWN) {
        if (access(buf->name, F_OK) != -1) {
            buf->eol = buffer_find_eol_sequence(buf);
        } else {
            buf->eol = EOL_CRLF;
        }
    }

    FILE *f = fopen(buf->name, "wb");

    for (i = 0; i < buf->length; ++i) {
        fputs(buf->lines[i].string, f);

        if (i == buf->length-1) continue; /* Do not add an extra \n to the end. */
        
        switch (buf->eol) {
        case EOL_CRLF:
            fputs("\r\n", f);
            break;
        case EOL_CR:
            fputs("\r", f);
            break;
        case EOL_LF:
            fputs("\n", f);
            break;
        }
    }

    strcat(msg, "Wrote to ");
    strcat(msg, buf->name);
    minibuffer_log(msg);
    
    fclose(f);
}



