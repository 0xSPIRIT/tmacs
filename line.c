#include "line.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buffer.h"

#include "util.h"

/* Instantiates a new line given a pointer. Does not allocate. */
struct Line *line_new(struct Line *line) {
    line->capacity = 32;
    line->length = 0;
    line->string = tcalloc(line->capacity, 1);
    
    return line;
}

/* Insert char at point. Reallocates the line when necessary. */
void line_insert_char(struct Line *line, int c) {
    int i;

    if (!insert_mode || point_x+1 >= line->length) {
        line->length++;
    }
    
    if (line->length >= line->capacity) {
        int add = line->capacity;
        
        line->capacity += add;
        line->string = trealloc(line->string, line->capacity);
        memset(line->string + line->length, 0, add);
    }
    
    if (!insert_mode) {
        for (i = line->length; i > point_x; --i) {
            line->string[i] = line->string[i-1];
        }
    }
    
    line->string[point_x++] = c;
}

/* Insert char at k. Reallocates when necessary. */
void line_insert_char_at(struct Line *line, int c, int k) {
    int i;

    if (!insert_mode || point_x+1 >= line->length) {
        line->length++;
    }
    
    if (line->length >= line->capacity) {
        int add = line->capacity;
        
        line->capacity += add;
        line->string = trealloc(line->string, line->capacity);
        memset(line->string + line->length, 0, add);
    }
    
    if (!insert_mode) {
        for (i = line->length; i > k; --i) {
            line->string[i] = line->string[i-1];
        }
    }
    
    line->string[k] = c;
}


/*
   Insert string at point. Reallocates the line when necessary.
   
   Probably horribly inefficient if you're adding a long string because
   line_insert_char() does a loop over most of the string for every character you add.
*/
void line_insert_str(struct Line *line, char *str) {
    size_t length = strlen(str);
    int i;

    for (i = 0; i < length; ++i) {
        line_insert_char(line, str[i]);
    }
}

/* Removes a character from the line. */
void line_cut_char(struct Line *line, int n) {
    int i;
    
    for (i = n; i < line->length; ++i) {
        line->string[i] = line->string[i+1];
    }
    line->length--;
    line->string[line->length] = 0;
}

/* Removes characters from n to k from the line. */
void line_cut_str(struct Line *line, int n, int k) {
    int i;

    for (i = n; i < k; ++i) {
        line_cut_char(line, i);
    }
}

/* Centers a line on the screen. */
void line_center(struct Line *line, int char_w) {
    int i=0;
    int w = window_width / char_w;

    bool im = insert_mode;

    insert_mode = false;

    while (line->string[i] == ' ') {
        line_cut_char(line, i);
    }

    w -= line->length;

    point_x = 0;
    while (point_x < w/2-1) {
        line_insert_char(line, ' ');
    }

    if (point_x < 0) point_x = 0;
    if (point_x > line->length) point_x = line->length;

    insert_mode = im;
}

/* Checks if the line has 0 length or is solely comprised of spaces. */
int is_line_blank(struct Line *line) {
    if (line->length == 0) return true;

    int i=0;
    while (i < line->length) {
        if (!isspace(line->string[i++])) {
            return false;
        }
    }

    return true;
}
