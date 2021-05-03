#ifndef UTIL_H_
#define UTIL_H_

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern int window_width;
extern int window_height;

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern bool running;


#define lerp(a, b, c) (((b) - (a)) * (c) + (a))

#define talloc(size) (_talloc(size))
#define tcalloc(count, size) (_tcalloc(count, size))
    
#define LOG(x) (puts(x), fflush(stdout))

#define is_control_held(k) (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL])
#define is_shift_held(k) (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT])
#define is_meta_held(k) (k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT])

static inline void *_talloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error at in file %s and line %d.", __FILE__, __LINE__);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    return ptr;
}


static inline void *_tcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error at in file %s and line %d.", __FILE__, __LINE__);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    return ptr;
}

#endif  /* UTIL_H_ */
