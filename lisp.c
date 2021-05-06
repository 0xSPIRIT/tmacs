#include "lisp.h"

#include <string.h>
#include <ctype.h>

#include "util.h"
#include "buffer.h"

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
    
    struct Token toks[64] = {0};

    int i=0, j=0;
    char curtok[64] = {0};
    int k=0;

    /* Separate into tokens by newline or spaces. */
    while (src[i]) {
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

    free(src);    
    return lisp;
}

void lisp_free(struct Lisp *lisp) {
    free(lisp);
}
