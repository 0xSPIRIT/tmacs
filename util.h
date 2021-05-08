#ifndef UTIL_H_
#define UTIL_H_

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define lerp(a, b, c) (((b) - (a)) * (c) + (a))

#define talloc(size) (_talloc(size, __func__, __FILE__, __LINE__))
#define tcalloc(count, size) (_tcalloc(count, size, __func__, __FILE__, __LINE__))

#define LOG(x) (puts(x), fflush(stdout))
#define ARRLEN(x) (sizeof(x) / sizeof(*x))

#define is_control_held(k) (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL])
#define is_shift_held(k) (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT])
#define is_meta_held(k) (k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT])


extern int window_width;
extern int window_height;

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern bool running;


static inline void *_talloc(size_t size, const char *func, const char *file, const int line) {
    void *ptr = malloc(size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error in function %s in file %s and line %d.", func, file, line);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    return ptr;
}

static inline void *_tcalloc(size_t count, size_t size, const char *func, const char *file, const int line) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error in function %s in file %s and line %d.", func, file, line);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    return ptr;
}

static inline char *read_file(FILE *f, size_t *l) {
    char *str;
    
    long p;
    size_t length;
    
    p = ftell(f);
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, p, SEEK_SET);

    str = tcalloc(length, 1);
    fread(str, 1, length, f);

    if (l) *l = length;

    return str;
}

#endif  /* UTIL_H_ */
