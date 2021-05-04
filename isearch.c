#include "isearch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

bool isearch_mode = false;

void toggle_isearch_mode() {
    isearch_mode = !isearch_mode;

    memset(minibuf->lines[0].string, 0, minibuf->lines[0].capacity);
    minibuf->length = 0;
}

void isearch_add_char(int c) {
    if (minibuf->lines[0].length >= minibuf->lines[0].capacity) {
        minibuf->lines[0].capacity += 10;
        minibuf->lines[0].string = realloc(minibuf->lines[0].string, minibuf->lines[0].capacity);
        memset(minibuf->lines[0].string + minibuf->lines[0].length, 0, 10);
    }
    minibuf->lines[0].string[minibuf->lines[0].length++] = c;

    isearch_update_point();
}

void isearch_update_point() {
    char *curr = tcalloc(cbuf->lines[point_y].length, 1);
    int i=0, k=0;
    int c = cbuf->lines[point_y].string[point_x];
    
    while (c) {
        if (c == minibuf->lines[0].string[i]) {
            curr[k++] = c;
            if (k == minibuf->lines[0].length) {
                
            }
        } else {
            memset(curr, 0, i+1);
            k=0;
        }
        
        point_x++;
        if (point_x > cbuf->lines[point_y].length) {
            point_y++;
            point_x = 0;
            if (point_y >= cbuf->length) {
                point_x = point_y = 0;
                toggle_isearch_mode();
            }
        }
        c = cbuf->lines[point_y].string[point_x];
    }

    free(curr);
}
