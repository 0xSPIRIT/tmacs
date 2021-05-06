#ifndef LISP_H_
#define LISP_H_

#include <SDL2/SDL.h>

#define VARS_MAX 256

enum {
    TOKEN_FUNCTION,
    TOKEN_IDENTIFIER,
    TOKEN_START_FUNCTION,
    TOKEN_END_FUNCTION
};

struct Token {
    int token;                  /* Token identifier */
    char name[64];              /* String data of the token. */
};

struct Key_Chord {
    SDL_Keycode prefix;         /* Control, meta, shift */
    SDL_Keycode key;            /* Actual key */
};

struct Variable {
    char c;
};

/* Loads a file defining keyboard shortcuts for functions. */
struct Lisp {
    struct Key_Chord *chords;
    int count;
};

extern struct Variable uservars[VARS_MAX];

struct Lisp *lisp_interpret(const char *file);
void lisp_free(struct Lisp *lisp);

#endif  /* LISP_H_ */
