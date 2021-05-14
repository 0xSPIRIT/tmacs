#ifndef LISP_H_
#define LISP_H_

#include <SDL2/SDL.h>

#define VARS_MAX   256
#define TOKEN_MAX  256
#define STRING_MAX 256

enum {
    TOKEN_FUNCTION = 0,
    TOKEN_VALUE,                /* Doesn't include strings. Strings are just TOKEN_IDENTIFIER */
    TOKEN_IDENTIFIER,
    TOKEN_START_FUNCTION,
    TOKEN_END_FUNCTION,
    TOKEN_START_STRING,
    TOKEN_END_STRING
};

enum {
    FUNCTION_NONE = 0,
    FUNCTION_PRINT,
  
    FUNCTION_ADD,
    FUNCTION_SUB,
    FUNCTION_DIV,
    FUNCTION_MUL,
  
    FUNCTION_SET_INT,
    FUNCTION_SET_FLOAT,
    FUNCTION_SET_STRING
};

enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_FLOAT
};

struct Token {
    int token;              /* Token identifier */
    char name[128];         /* String data of the token. */
};

struct Pair {
    char identifier[64];
    int type;
    
    union {
        int integer;
        char string[STRING_MAX];
        float floating;
    };
};

/* Loads a file defining keyboard shortcuts for functions. */
struct Lisp {
    struct Token *toks;
    int count;
};

extern int   vsync;
extern int   tab_width;
extern int   smooth_scroll;
extern int   scroll_amount;
extern int   font_size;
extern int   draw_text_blended;
extern float scroll_speed;
extern char *font_name;

extern struct Pair invars[];
extern const unsigned invars_count;

extern struct Pair uservars[VARS_MAX];
extern unsigned uservars_count;

float invars_get_float(const char *varname);
int invars_get_integer(const char *varname);
char *invars_get_string(const char *varname);

struct Lisp *lisp_interpret(const char *file);
void lisp_free(struct Lisp *lisp);
    
#endif  /* LISP_H_ */
