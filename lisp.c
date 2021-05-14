#include "lisp.h"

#include <string.h>
#include <ctype.h>

#include "util.h"
#include "buffer.h"

/* This is dumb, just have macros defined from 0 to whatever and use that is an index into the invars array. */
int   vsync;
int   tab_width;
int   smooth_scroll;
int   scroll_amount;
int   font_size;
int   draw_text_blended;
float scroll_speed;
char *font_name;

struct Pair invars[] = {
    {"tab-width",         TYPE_INT,    {.integer  = 2}},
    {"smooth-scroll",     TYPE_INT,    {.integer  = 0}},
    {"vsync",             TYPE_INT,    {.integer  = 1}},
    {"scroll-amount",     TYPE_INT,    {.integer  = 5}},
    {"font-size",         TYPE_INT,    {.integer  = 17}},
    {"font",              TYPE_STRING, {.string   = "fonts/consola.ttf"}},
    {"draw-text-blended", TYPE_INT,    {.integer  = 0}},
    {"scroll-speed",      TYPE_FLOAT,  {.floating = 0.0125f}}
};
const unsigned invars_count = ARRLEN(invars);

struct Pair uservars[VARS_MAX] = {0};
unsigned uservars_count = 0;

static int is_digit(int c) {
    return (c - '0') >= 0 && (c - '0') < 10;
}

static int is_str_num(char *str) {
    return is_digit(str[strlen(str)-1]);
}

float invars_get_float(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].floating;
        }
    }

    fprintf(stderr, "Could not find variable %s!\n", varname); fflush(stderr);
    return -2;
}

int invars_get_integer(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].integer;
        }
    }

    fprintf(stderr, "Could not find variable %s!\n", varname); fflush(stderr);    
    return -2;
}

char *invars_get_string(const char *varname) {
    for (int i = 0; i < invars_count; ++i) {
        if (0==strcmp(varname, invars[i].identifier)) {
            return invars[i].string;
        }
    }

    fprintf(stderr, "Could not find variable %s!\n", varname); fflush(stderr);    
    return NULL;
}

static struct Pair *find_variable(struct Lisp *lisp, const char *name) {
    struct Pair *vars = invars;
    unsigned count = invars_count;

    for (int q = 0; q < 2; ++q) {
        for (int j = 0; j < count; ++j) {
            if (0==strcmp(name, vars[j].identifier)) return &vars[j];
        }

        vars = uservars;
        count = uservars_count;
    }
    
    return NULL;
}

/* TODO: This needs some cleaning up. Lots of repeated code, yucky. */
static int evaluate_tokens(struct Lisp *lisp) {
    int i;
    struct Pair *p = NULL;

    int curfunc = FUNCTION_NONE;
    int argc = 0;
    
    for (i = 0; i < lisp->count; ++i) {
        switch (lisp->toks[i].token) {
        case TOKEN_START_FUNCTION: case TOKEN_END_FUNCTION:
            argc = 0;
            break;
        case TOKEN_FUNCTION:
            if (0==strcmp(lisp->toks[i].name, "print"))      curfunc = FUNCTION_PRINT;
            if (0==strcmp(lisp->toks[i].name, "set-int"))    curfunc = FUNCTION_SET_INT;
            if (0==strcmp(lisp->toks[i].name, "set-float"))  curfunc = FUNCTION_SET_FLOAT;
            if (0==strcmp(lisp->toks[i].name, "set-string")) curfunc = FUNCTION_SET_STRING;
            
            if (0==strcmp(lisp->toks[i].name, "+")) curfunc = FUNCTION_ADD;
            if (0==strcmp(lisp->toks[i].name, "-")) curfunc = FUNCTION_SUB;
            if (0==strcmp(lisp->toks[i].name, "/")) curfunc = FUNCTION_DIV;
            if (0==strcmp(lisp->toks[i].name, "*")) curfunc = FUNCTION_MUL;
            break;
        case TOKEN_IDENTIFIER:
            switch (curfunc) {
            case FUNCTION_ADD: case FUNCTION_SUB: case FUNCTION_MUL: case FUNCTION_DIV:
                p = NULL;

                if ((p = find_variable(lisp, lisp->toks[i].name))) {
                    float add;
                    
                    if (lisp->toks[i+1].token == TOKEN_IDENTIFIER) {
                        struct Pair *e = find_variable(lisp, lisp->toks[++i].name);

                        if (p->type == TYPE_INT) {
                            add = e->integer;
                        } else {
                            add = e->floating;
                        }
                    } else if (lisp->toks[i+1].token == TOKEN_VALUE) {
                        if (p->type == TYPE_INT) {
                            add = atoi(lisp->toks[i+1].name);
                        } else {
                            add = atof(lisp->toks[i+1].name);
                        }
                    }
                    
                    if (p->type == TYPE_INT) {
                        if (curfunc == FUNCTION_ADD) {
                            p->integer += (int)add;
                        } else if (curfunc == FUNCTION_SUB) {
                            p->integer -= (int)add;
                        } else if (curfunc == FUNCTION_MUL) {
                            p->integer *= (int)add;
                        } else if (curfunc == FUNCTION_DIV) {
                            p->integer /= (int)add;
                        }
                    } else if (p->type == TYPE_FLOAT) {
                        if (curfunc == FUNCTION_ADD) {
                            p->floating += add;
                        } else if (curfunc == FUNCTION_SUB) {
                            p->floating -= add;
                        } else if (curfunc == FUNCTION_MUL) {
                            p->floating *= add;
                        } else if (curfunc == FUNCTION_DIV) {
                            p->floating /= add;
                        }
                    }
                }
                break;
            case FUNCTION_PRINT:;
                p = NULL;
                
                if ((p = find_variable(lisp, lisp->toks[i].name))) {
                    switch (p->type) {
                    case TYPE_INT:
                        printf("%d\n", p->integer); fflush(stdout);
                        break;
                    case TYPE_FLOAT:
                        printf("%f\n", p->floating); fflush(stdout);
                        break;
                    case TYPE_STRING:
                        printf("%s\n", p->string); fflush(stdout);
                        break;
                    default:
                        printf("Unknown print type.\n"); fflush(stdout);
                        break;
                    }
                }
                break;
            case FUNCTION_SET_FLOAT: case FUNCTION_SET_INT:
                p = NULL;

                if (!(p = find_variable(lisp, lisp->toks[i].name))) {
                    strcpy(uservars[uservars_count].identifier, lisp->toks[i].name);
                    uservars_count++;
                }

                if (!p) p = &uservars[uservars_count-1];
                
                if (lisp->toks[i+1].token == TOKEN_IDENTIFIER) {
                    struct Pair *e = find_variable(lisp, lisp->toks[i+1].name);
                    
                    if (curfunc == FUNCTION_SET_FLOAT) {
                        p->type = TYPE_FLOAT;
                        p->floating = e->floating;
                    } else {
                        p->type = TYPE_INT;
                        p->integer = e->integer;
                    }
                } else if (lisp->toks[i+1].token == TOKEN_VALUE) {
                    if (curfunc == FUNCTION_SET_FLOAT) {
                        p->type = TYPE_FLOAT;
                        p->floating = atof(lisp->toks[i+1].name);
                    } else {
                        p->type = TYPE_INT;
                        p->integer = atoi(lisp->toks[i+1].name);
                    }
                }
                break;

                ++i;
                break;
            case FUNCTION_SET_STRING:
                p = NULL;

                if (!(p = find_variable(lisp, lisp->toks[i].name))) {
                    strcpy(uservars[uservars_count].identifier, lisp->toks[i].name);
                    uservars[uservars_count].type = TYPE_STRING;
                    uservars_count++;
                }

                if (!p) p = &uservars[uservars_count-1];

                if ((p = find_variable(lisp, lisp->toks[i].name))) {
                    ++i;
                    if (lisp->toks[i].token == TOKEN_START_STRING) {
                        ++i;
                        p->type = TYPE_STRING;
                        memset(p->string, 0, STRING_MAX);
                        strcpy(p->string, lisp->toks[i].name);
                        ++i;    /* +1 to get past the TOKEN_END_STRING */
                    } else if (lisp->toks[i].token == TOKEN_IDENTIFIER) {
                        struct Pair *e = find_variable(lisp, lisp->toks[i].name);
                        if (e->type != TYPE_STRING) {
                            fprintf(stderr, "set-string requires a string to be set to");
                            return 1;
                        }
                            
                        p->type = TYPE_STRING;
                        memset(p->string, 0, STRING_MAX);
                        strcpy(p->string, e->string);
                    }
                }
                break;
            default:
                break;
            }
            
            argc++;
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
    char curtok[STRING_MAX] = {0};
    int k=0;
    bool isinstring = false;

    /* Separate into tokens by newline or spaces. */
    while (src[i]) {
        if (src[i] == ';') {
            /* Comment has started; skip to EOL. */
            while (src[i] != '\n' || src[i] == 0) ++i;
            
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
            memset(curtok, 0, STRING_MAX);
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
            
            memset(curtok, 0, STRING_MAX);
            k = 0;
        }
        if (src[i] == '(' || src[i] == ')') {
            curtok[0] = src[i];
            curtok[1] = 0;
        }
        if (isspace(src[i]) || src[i] == '(' || src[i] == ')') {
            if (*curtok) {
                strcpy(toks[j++].name, curtok);
                memset(curtok, 0, STRING_MAX);
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
            } else if (is_str_num(toks[i].name)) {
                toks[i].token = TOKEN_VALUE;
            } else {
                toks[i].token = TOKEN_IDENTIFIER;
            }
        }
    }

    evaluate_tokens(lisp);

    free(src);    
    return lisp;
}

void lisp_free(struct Lisp *lisp) {
    free(lisp->toks);
    free(lisp);
}


