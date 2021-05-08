#include "lisp.h"

#include <string.h>
#include <ctype.h>

#include "util.h"
#include "buffer.h"

struct Pair invars[] = {
    {"tab-width", 2},
    {"smooth-scroll", 0},
    {"vsync", 1},
    {"scroll-amount", 5}
};
const int invars_count = ARRLEN(invars);

int invars_get_value(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].value;
        }
    }
    
    return -2;
}

static int evaluate_tokens(struct Lisp *lisp) {
    int i;

    int curfunc = FUNCTION_NONE;
    int curargs = 0;
    
    for (i = 0; i < lisp->count; ++i) {
        switch (lisp->toks[i].token) {
        case TOKEN_START_FUNCTION: case TOKEN_END_FUNCTION:
            curargs = 0;
            break;
        case TOKEN_FUNCTION:
            curfunc = FUNCTION_SET_VARIABLE;
            break;
        case TOKEN_IDENTIFIER:
            switch (curfunc) {
            case FUNCTION_SET_VARIABLE:;
                int c = -1;
                for (int j = 0; j < invars_count; ++j) {
                    if (0==strcmp(lisp->toks[i].name, invars[j].identifier)) {
                        c = j;
                        break;
                    }
                }

                if (c != -1) {
                    if (lisp->toks[i+1].token == TOKEN_IDENTIFIER) {
                        invars[c].value = atoi(lisp->toks[++i].name);
                    } else {
                        fprintf(stderr, "Error parsing lisp file!");
                        fflush(stderr);
                        return 1;
                    }
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
    char curtok[64] = {0};
    int k=0;

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
        if (src[i] == ')') {
            strcpy(toks[j++].name, curtok);
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
                memset(curtok, 0, 64);
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
    i=0; j=0;
        
    for (i = 0; i < lisp->count; ++i) {
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

