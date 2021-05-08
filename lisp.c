#include "lisp.h"

#include <string.h>
#include <ctype.h>

#include "util.h"
#include "buffer.h"

int    vsync;
int    smooth_scroll;
int    tab_width;
int    scroll_amount;
int    font_size;
int    draw_text_blended;
char  *font_name;

struct Pair invars[] = {
    {"tab-width", {.integer = 2}},
    {"smooth-scroll", {.integer = 0}},
    {"vsync", {.integer = 1}},
    {"scroll-amount", {.integer = 5}},
    {"font-size", {.integer = 17}},
    {"font", {.string = "fonts/consola.ttf"}},
    {"draw-text-blended", {.integer = 0}}
};

const unsigned invars_count = ARRLEN(invars);

int invars_get_integer(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].integer;
        }
    }
    
    return -2;
}

char *invars_get_string(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].string;
        }
    }
    
    return NULL;
}

static int evaluate_tokens(struct Lisp *lisp) {
    int i, c;

    int curfunc = FUNCTION_NONE;
    int curargs = 0;
    
    for (i = 0; i < lisp->count; ++i) {
        switch (lisp->toks[i].token) {
        case TOKEN_START_FUNCTION: case TOKEN_END_FUNCTION:
            curargs = 0;
            break;
        case TOKEN_FUNCTION:
            if (0==strcmp(lisp->toks[i].name, "set-int")) {
                curfunc = FUNCTION_SET_INT;
            }
            if (0==strcmp(lisp->toks[i].name, "set-string")) {
                curfunc = FUNCTION_SET_STRING;
            }
            break;
        case TOKEN_IDENTIFIER:
            switch (curfunc) {
            case FUNCTION_SET_INT:
                c = -1;
                for (int j = 0; j < invars_count; ++j) {
                    if (0==strcmp(lisp->toks[i].name, invars[j].identifier)) {
                        c = j;
                        break;
                    }
                }

                if (c != -1) {
                    if (lisp->toks[i+1].token == TOKEN_IDENTIFIER) {
                        invars[c].integer = atoi(lisp->toks[++i].name);
                    } else {
                        fprintf(stderr, "Error parsing lisp file!");
                        fflush(stderr);
                        return 1;
                    }
                } else {
                    fprintf(stderr, "Cannot find variable %s!", lisp->toks[i].name);
                    fflush(stderr);
                }
                break;
            case FUNCTION_SET_STRING:
                c = -1;
                for (int j = 0; j < invars_count; ++j) {
                    if (0==strcmp(lisp->toks[i].name, invars[j].identifier)) {
                        c = j;
                        break;
                    }
                }

                if (c != -1) {
                    printf("%d\n", lisp->toks[++i].token); fflush(stdout);
                    if (lisp->toks[i].token == TOKEN_START_STRING) {
                        ++i;
                        memset(invars[c].string, 0, 64);
                        strcpy(invars[c].string, lisp->toks[i].name);
                        ++i;    /* +1 to get past the TOKEN_END_STRING */
                    } else {
                        fprintf(stderr, "set-string function requires string as parameter.");
                        fflush(stdout);
                        return 1;
                    }
                } else {
                    fprintf(stderr, "Cannot find variable %s!", lisp->toks[i].name);
                    fflush(stderr);
                }
                break;
            default:
                break;
            }
            
            curargs++;
            break;
        }
    }

    return 0;
}

/* Interpret a lisp file and save keybindings to memory. */
struct Lisp *lisp_interpret(const char *file) {
    struct Lisp *lisp;
    
    FILE *f;
    char *src;
    size_t len;
    long p;
    
    lisp = tcalloc(1, sizeof(struct Lisp));

    f = fopen(file, "r");
    if (!f) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "File error", "Could not open file", window);
        exit(1);
    }

    p = ftell(f);
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, p, SEEK_SET);

    src = tcalloc(len, 1);
    fread(src, 1, len, f);

    fclose(f);
    
    struct Token *toks = tcalloc(TOKEN_MAX, sizeof(struct Token));

    int i=0, j=0;
    char curtok[128] = {0};
    int k=0;
    bool isinstring = false;

    /* Separate into tokens by newline or spaces. */
    while (src[i]) {
        if (src[i] == ';') {
            /* Comment has started; skip to EOL. */
            while (src[i] != '\n' || src[i] == 0) {
                ++i;
            }
            
            ++i;
            continue;
        }
        if (src[i] == '"') {
            isinstring = !isinstring;
            if (!isinstring) {
                strcpy(toks[j++].name, curtok);
            }
            
            strcpy(toks[j++].name, "\"");
            
            ++i;
            memset(curtok, 0, 128);
            continue;
        }
        
        if (isinstring) {
            char s[2] = { src[i], 0 };
            
            strcat(curtok, s);
            ++i;
            continue;
        }
        
        if (src[i] == ')') {
            if (*curtok != 0) {
                strcpy(toks[j++].name, curtok);
            }
            
            memset(curtok, 0, 64);
            k = 0;
        }
        if (src[i] == '(' || src[i] == ')') {
            curtok[0] = src[i];
            curtok[1] = 0;
        }
        if (isspace(src[i]) || src[i] == '(' || src[i] == ')') {
            if (*curtok) {
                strcpy(toks[j++].name, curtok);
                memset(curtok, 0, 128);
                k = 0;
            }
        } else {
            curtok[k++] = src[i];
        }
        i++;
    }

    lisp->toks = toks;
    lisp->count = j;

    /* Set token types. */
    bool str = false;
        
    i=0; j=0;

    for (i = 0; i < lisp->count; ++i) {
        if (*toks[i].name == '"') {
            toks[i].token = !str ? TOKEN_START_STRING : TOKEN_END_STRING;
            str = !str;
            continue;
        }
        
        if (*toks[i].name == '(') {
            toks[i].token = TOKEN_START_FUNCTION;
            j=0;
        } else if (*toks[i].name == ')') {
            toks[i].token = TOKEN_END_FUNCTION;
        } else {
            if (j == 0) {
                toks[i].token = TOKEN_FUNCTION;
                j++;
            } else {
                toks[i].token = TOKEN_IDENTIFIER;
            }
        }
    }

    for (int x = 0; x < lisp->count; ++x) {
        printf("%d: %s\n", toks[x].token, toks[x].name);
    }
    fflush(stdout);
    
    evaluate_tokens(lisp);

    free(src);    
    return lisp;
}

void lisp_free(struct Lisp *lisp) {
    free(lisp->toks);
    free(lisp);
}


