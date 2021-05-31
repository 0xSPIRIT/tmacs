#ifndef BUFFER_H_
#define BUFFER_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdbool.h>

#include "line.h"
#include "minibuffer.h"
#include "mark.h"

#define BUFFERS_MAX 32
#define FNAME_MAX   128
#define FRAMES_MAX 3

enum {
    FRAME_POSITION_LEFT,
    FRAME_POSITION_RIGHT,
};

extern int point_x;
extern int point_y;

struct Buffer {
    struct Line *lines;
    char *name;

    bool is_minibuf;
    
    int capacity, length;

    int px, py;                 /* Last location of point_x and point_y */
    
    int x, y;
    int w, h;
    
    float yoff, desired_yoff;
    float xoff, desired_xoff;

    struct Mark mark;
};

extern struct Buffer *cbuf;
extern struct Buffer *minibuf;
extern struct Buffer *buffers[BUFFERS_MAX];

extern int buffers_count;
extern int buffer_index;
extern int buffer_previous_index;
extern bool insert_mode;

int buffer_find_index(struct Buffer *buf);
void buffer_switch(int i);
void buffer_kill(void);

struct Buffer *buffer_new(const char *name, bool minibuf);
void buffer_newline(bool smart_indent);
void buffer_cutline(int k);
void buffer_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buf);
void buffer_free(struct Buffer *buf);
void buffer_reset(struct Buffer *buf);
void buffer_save(struct Buffer *buf);
void buffer_save_new(struct Buffer *buf, const char *name);
void buffer_load_file(struct Buffer *buf, char *file);

#endif  /* BUFFER_H_ */

