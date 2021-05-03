#ifndef BUFFER_H_
#define BUFFER_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdbool.h>

#include "line.h"
#include "minibuffer.h"

extern int point_x;
extern int point_y;

enum {
    EOL_UNKNOWN = 0,
    EOL_LF,
    EOL_CR,
    EOL_CRLF
};

struct Buffer {
    struct Line *lines;
    char *name;

    bool is_minibuf;

    int  eol_sequence;
    
    int  capacity, length;

    int  px, py;                 /* Last location of point_x and point_y */
    int  x, y;                   /* Position of buffer on-screen. */
    int  yoff, desired_yoff;
};

extern struct Buffer *cbuf;
extern struct Buffer *minibuf;
extern struct Buffer *buffers[32];

extern bool insert_mode;

struct Buffer *buffer_new(const char *name, bool minibuf);
void buffer_newline();
void buffer_cutline(int k);
void buffer_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buf);
void buffer_free(struct Buffer *buf);
void buffer_reset(struct Buffer *buf);
void buffer_save(struct Buffer *buf);
void buffer_save_new(struct Buffer *buf, const char *name);
void buffer_load_file(struct Buffer *buf, char *file);

int buffer_find_eol_sequence(struct Buffer *buf);

#endif  /* BUFFER_H_ */
