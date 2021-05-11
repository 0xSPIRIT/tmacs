#include "minibuffer.h"

#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

/* Toggle between minibuffer and the recent buffer. */
void minibuffer_toggle() {
    if (cbuf == minibuf) {
        cbuf = buffers[buffer_index];
    } else {
        cbuf = minibuf;
    }

    point_x = cbuf->px;
    point_y = cbuf->py;
    
    if (cbuf->is_minibuf) {
        line_cut_str(cbuf->lines+0, 0, cbuf->lines[0].length);
        cbuf->lines[0].length = 0;

        point_x = point_y = 0;
    }
}

/*
   Executes the current command in the minibuffer.
   Assumes that cbuf is the minibuffer.
*/
void minibuffer_execute_command() {
    char *str = cbuf->lines[0].string;
    char words[64][12];

    int i=0, j=0, k=0;

    memset(words, 0, 64*12);

    while (i < cbuf->lines[0].length) {
        words[j][k] = str[i];
        ++i;
        ++k;

        if (str[i] == ' ') {
            ++j;
            ++i;
            k = 0;
        }
    }
    ++j;

    /* TODO: Make this account for having multiple buffers instead of hardcoding it. */
    if (0==strcmp(words[0], "w") || 0==strcmp(words[0], "write")) {
        if (j == 1) {
            buffer_save(buffers[buffer_index]);
        } else if (j == 2) {
            buffer_save_new(buffers[buffer_index], words[1]);
        }
    } else if (0==strcmp(words[0], "o") || 0==strcmp(words[0], "open")) {
        buffers[buffers_count++] = buffer_new(words[1], false);
        buffer_index = buffers_count-1;
        buffer_load_file(buffers[buffer_index], words[1]);
        minibuffer_toggle();
    } else if (0==strcmp(words[0], "q") || 0==strcmp(words[0], "quit") || 0==strcmp(words[0], "exit")) {
        running = false;
    } else {
        minibuffer_log("Unknown Command.");
    }

    point_x = 0;
}

/* Sets the minibuffer's contents to msg. Does not set focus to the minibuffer. */
void minibuffer_log(const char *msg) {
    memset(minibuf->lines[0].string, 0, minibuf->lines[0].length);
    minibuf->lines[0].length = 0;

    strcpy(minibuf->lines[0].string, msg);
    minibuf->lines[0].length = strlen(msg);
}
