#ifndef LISP_H_
#define LISP_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

enum {
    TOKEN_FUNCTION,
    TOKEN_IDENTIFIER,
    TOKEN_START_FUNCTION,
    TOKEN_END_FUNCTION
};

struct Token {
    int token;
    char name[64];
};

struct Key_Chord {
    SDL_Keycode prefix;         /* Control, meta, shift */
    SDL_Keycode key;            /* Actual key */
};

/* Loads a file defining keyboard shortcuts for functions. */
struct Lisp {
    struct Key_Chord *chords;
    enum Functions *funcs;
    
    int count;
};

struct Lisp *lisp_interpret(const char *file);
void lisp_free(struct Lisp *lisp);

#endif  /* LISP_H_ */
