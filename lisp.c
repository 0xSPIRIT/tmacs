#include "lisp.h"

#include <string.h>
#include <ctype.h>

#include "util.h"
#include "buffer.h"

struct Lisp *lisp_interpret(const char *file) {
    struct Lisp *lisp;
    
    FILE *f;
    char *source;
    size_t length;
    long p;

    lisp = tcalloc(1, sizeof(struct Lisp));

    f = fopen(file, "r");
    if (!f) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "File error", "Could not open file", window);
        exit(1);
    }

    p = ftell(f);
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, p, SEEK_SET);

    source = tcalloc(length, 1);
    fread(source, 1, length, f);

    fclose(f);
    
    {
        struct Token words[64] = {0};

        int i=0, j=0;
        char token[64] = {0};
        int k=0;

        /* Separate into tokens (only words) */
        while (source[i]) {
            if (source[i] == '(' || source[i] == ')') {
                if (source[i] == ')') {
                    strcpy(words[j++].name, token);
                    memset(token, 0, 64);
                    k = 0;
                }
                token[0] = source[i];
                token[1] = 0;
            }
            if (isspace(source[i]) || source[i] == '(' || source[i] == ')') {
                if (*token) {
                    strcpy(words[j++].name, token);
                    memset(token, 0, 64);
                    k = 0;
                }
            } else {
                token[k++] = source[i];
            }
            i++;
        }
        

        /* Set token types. */
        i=0; j=0;

        for (int x = 0; x < j; ++x) {
            LOG(words[x].name);
        }
    }


    free(source);    
    return lisp;
}

void lisp_free(struct Lisp *lisp) {
    free(lisp);
}
