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

    char *curr = tcalloc(minibuf->lines[0].length, 1);
    int i=0;
    bool found = false;

    while (!found) {
        if (cbuf->lines[point_y].string[point_x] == minibuf->lines[0].string[i]) {
            const char s[] = { cbuf->lines[point_y].string[point_x], 0 };
            strcat(curr, s);
            ++i;

            printf("%d, %d\n", i, minibuf->lines[0].length); fflush(stdout);
            
            if (i >= minibuf->lines[0].length) {
                LOG("FOUND");
                found = true;
            } else {
                point_x = sx;
                point_y = sy;
            }
        }

        point_x++;
        if (point_x >= cbuf->lines[point_y].length+1) {
            point_x=0;
            point_y++;
        }
        if (point_y > cbuf->length) {
            LOG("A");
            return;
        }
    }
}
