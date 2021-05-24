#include "minibuffer.h"

#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

/* Toggle between minibuffer and the recent buffer. */
void minibuffer_toggle(void) {
    if (cbuf == minibuf) {
        cbuf = buffers[buffer_index];
    } else {
        cbuf = minibuf;
    }
    smart_indent_mode = (cbuf != minibuf);
    
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
void minibuffer_execute_command(void) {
    char *str = cbuf->lines[0].string;
    char words[64][32] = {0};

    int i=0, j=0, k=0;
    int is_in_quote = 0;

    while (i < cbuf->lines[0].length) {
        if (str[i] == '"') {
            is_in_quote = !is_in_quote;
            ++i;
            continue;
        }
        
        words[j][k] = str[i];
        ++i;
        ++k;

        if (str[i] == ' ' && !is_in_quote) {
            if (i+1 == cbuf->lines[0].length) {
                break;
            }
            ++j;
            ++i;
            k = 0;
        }
    }
    ++j;

    if (0==strcmp(words[0], "w") || 0==strcmp(words[0], "write") || 0==strcmp(words[0], "write-to")) {
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
    } else if (0==strcmp(words[0], "switch") || 0==strcmp(words[0], "switch-to")) {
        if (j == 1) {
            buffer_switch(buffer_previous_index);
        } else {
            for (int c = 0; c < buffers_count; ++c) {
                if (0==strcmp(words[1], buffers[c]->name)) {
                    buffer_switch(c);
                    break;
                }
            }

            buffers[buffers_count++] = buffer_new(words[1], false);
            buffer_index = buffers_count-1;
        }
    } else if (0==strcmp(words[0], "shell-command")) {
        LOG(words[1]);
        system(words[1]);
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
