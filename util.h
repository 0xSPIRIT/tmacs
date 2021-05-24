#ifndef UTIL_H_
#define UTIL_H_

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern int memory_debug;

#define lerp(a, b, c) (((b) - (a)) * (c) + (a))

#define talloc(size) (_talloc(size, __func__, __FILE__, __LINE__))
#define tcalloc(count, size) (_tcalloc(count, size, __func__, __FILE__, __LINE__))
#define trealloc(ptr, new) (_trealloc(ptr, new, __func__, __FILE__, __LINE__))
#define tassert(ptr) (_assert(ptr, __func__, __FILE__, __LINE__))
#define tfree(ptr) (_tfree(ptr, __func__, __FILE__, __LINE__))

#define LOG(x) (puts(x), fflush(stdout))
#define ARRLEN(x) (sizeof(x) / sizeof(*x))

#define is_control_held(k) (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL])
#define is_shift_held(k) (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT])
#define is_meta_held(k) (k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT])

extern int memory_counter;

extern Uint32 point_time, last, delta;

extern int window_width;
extern int window_height;

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern bool running;

static inline void _tfree(void *ptr, const char *func, const char *file, const int line) {
    if (memory_debug) {
        printf("Memory freed              :       %15s %20s %5d\n", file, func, line); fflush(stdout);
    }
    
    free(ptr);

    memory_counter--;
}

static inline void _assert(void *ptr, const char *func, const char *file, const int line) {
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "ASSERTION ERROR in function %s, in file %s, and line %d.", func, file, line);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Assertion Error", msg, window);
        exit(1);
    }
}

static inline void *_talloc(size_t size, const char *func, const char *file, const int line) {
    if (memory_debug) {
        printf("Requested allocation      : %5u %15s %20s %5d\n", size, file, func, line); fflush(stdout);
    }

    void *ptr = malloc(size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error in function %s in file %s and line %d.", func, file, line);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    memory_counter++;

    return ptr;
}

static inline void *_tcalloc(size_t count, size_t size, const char *func, const char *file, const int line) {
    if (memory_debug) {
        printf("Requested clear allocation: %5u %15s %20s %5d\n", count * size, file, func, line); fflush(stdout);
    }

    void *ptr = calloc(count, size);
    if (!ptr) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error in function %s in file %s and line %d.", func, file, line);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);

        exit(1);
    }
    
    memory_counter++;

    return ptr;
}

static inline void *_trealloc(void *ptr, size_t new, const char *func, const char *file, const int line) {
    if (memory_debug) {
        printf("Requested reallocation    : %5u %15s %20s %5d\n", new, file, func, line); fflush(stdout);
    }

    void *p = realloc(ptr, new);
    if (!p) {
        char msg[256] = {0};
        sprintf(msg, "Allocation error in function %s in file %s and line %d.", func, file, line);
        
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Allocation Error", msg, window);
        exit(1);
    }

    return p;
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

static inline void print_memory_counter() {
    printf("Outsanding memory allocations not freed: %d\n", memory_counter); fflush(stdout);
}

#endif  /* UTIL_H_ */
