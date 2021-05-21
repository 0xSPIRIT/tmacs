#include "isearch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

bool isearch_mode = false;
int sx, sy;

void toggle_isearch_mode() {
    isearch_mode = !isearch_mode;
    sx = point_x;
    sy = point_y;
    line_cut_str(minibuf->lines+0, 0, minibuf->lines[0].length);
}

void isearch_add_char(int c) {
    line_insert_char_at(minibuf->lines+0, c, minibuf->lines[0].length);
}

void isearch_update_point() {
    if (!minibuf->lines[0].string[0]) return;

    int c=0;

    point_x = sx;
    point_y = sy;

    while (true) {
        if (minibuf->lines[0].string[c] == cbuf->lines[point_y].string[point_x]) {
            ++c;
            if (c == minibuf->lines[0].length) return;
        } else {
            c = 0;
        }
        
        point_x++;
        if (point_x >= cbuf->lines[point_y].length) point_y++;
        if (point_y > cbuf->length-1) {
            point_x = sx;
            point_y = sy;
            return;
        }
    }
    
    point_x = sx;
    point_y = sy;
}
