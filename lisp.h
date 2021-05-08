#ifndef LISP_H_
#define LISP_H_

#include <SDL2/SDL.h>

#define VARS_MAX 256
#define TOKEN_MAX 256

enum {
    TOKEN_FUNCTION = 0,
    TOKEN_IDENTIFIER,
    TOKEN_START_FUNCTION,
    TOKEN_END_FUNCTION,
    TOKEN_START_STRING,
    TOKEN_END_STRING
};

enum {
    FUNCTION_NONE = 0,
    FUNCTION_SET_INT,
    FUNCTION_SET_STRING
};

struct Token {
    int token;                  /* Token identifier */
    char name[128];             /* String data of the token. */
};

struct Key_Chord {
    SDL_Keycode prefix;         /* Control, meta, shift */
    SDL_Keycode key;            /* Actual key */
};

struct Pair {
    char identifier[64];
    
    union {
        int integer;
        char string[64];
        float floating;
    };
};

/* Loads a file defining keyboard shortcuts for functions. */
struct Lisp {
    struct Token *toks;
    int count;
};

extern int vsync;
extern int smooth_scroll;
extern int tab_width;
extern int scroll_amount;
extern int font_size;
extern int draw_text_blended;
extern char *font_name;

extern struct Pair invars[];
extern const unsigned invars_count;

int invars_get_integer(const char *varname);
char *invars_get_string(const char *varname);

struct Lisp *lisp_interpret(const char *file);
void lisp_free(struct Lisp *lisp);

#endif  /* LISP_H_ */
