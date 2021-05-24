#define SDL_MAIN_HANDLED
                        
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "buffer.h"
#include "modeline.h"
#include "util.h"
#include "mark.h"
#include "lisp.h"
#include "isearch.h"

Uint32 point_time = 0,
    last,
    delta = 0;

int memory_debug = 0;

SDL_Window *window;
SDL_Renderer *renderer;

int window_width = 640;
int window_height = 720;

int char_w, char_h;

bool running;

char *separator_charset = "_()+-{}.,[]<>!@#$%^&*=~;:'\"/\\";
int seplen;

int memory_counter = 0;

static inline void clamp_y_min() {
    if (point_y < 0) point_y = 0;
    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;

    if (cbuf != minibuf && point_y * char_h < cbuf->desired_yoff) {
        cbuf->desired_yoff = point_y * char_h - char_h * floor((window_height/2)/char_h);
    }

    if (cbuf->desired_yoff < 0) cbuf->desired_yoff = 0;
    
    point_time = 0;
}

static inline void clamp_y_max() {
    if (point_y >= cbuf->length) {
        point_y = cbuf->length - 1;
        point_x = cbuf->lines[point_y].length;
    }
    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;

    if (cbuf != minibuf && point_y * char_h >= char_h * (int)((cbuf->desired_yoff + minibuf->y - char_h*2) / char_h)) {
        cbuf->desired_yoff = (point_y * char_h) - char_h * ((int)(minibuf->y - char_h*2) / char_h) + (char_h * floor((window_height/2)/char_h));
    }
}

static inline bool is_separator(int c) {
    if (isspace(c)) return true;

    for (int i = 0; i < seplen; ++i) {
        if (c == separator_charset[i])
            return true;
    }
    return false;
}

static inline void point_forward_paragraph() {
    point_x = 0;
    while (point_y < cbuf->length && is_line_blank(cbuf->lines+(point_y))) point_y++;
    while (point_y < cbuf->length && !is_line_blank(cbuf->lines+(point_y))) point_y++;

    point_time = 0;
}

static inline void point_backward_paragraph() {
    point_x = 0;
    while (point_y > 0 && is_line_blank(cbuf->lines+(point_y))) point_y--;
    while (point_y > 0 && !is_line_blank(cbuf->lines+(point_y))) point_y--;
    
    point_time = 0;
}

static inline void point_forward_word() {
    while (point_x < cbuf->lines[point_y].length && isspace(cbuf->lines[point_y].string[point_x++]));
    while (point_x < cbuf->lines[point_y].length && !is_separator(cbuf->lines[point_y].string[point_x++]));
    if (point_x < cbuf->lines[point_y].length) point_x--;
    
    point_time = 0;
}

static inline void point_backward_word() {
    while (point_x > 0 && isspace(cbuf->lines[point_y].string[--point_x]));
    while (point_x > 0 && !is_separator(cbuf->lines[point_y].string[--point_x]));
    if (point_x > 0) point_x++;
    
    point_time = 0;
}

static inline void backward_kill_word() {
    mark_start(point_x, point_y);
    point_backward_word();
    mark_update(point_x, point_y);
    mark_kill(cbuf);

    point_time = 0;
}

static inline void forward_kill_word() {
    mark_start(point_x, point_y);
    point_forward_word();
    mark_update(point_x, point_y);
    mark_kill(cbuf);

    point_time = 0;
}

static inline void end_of_buffer() {
    point_y = cbuf->length-1;
    point_x = cbuf->lines[point_y].length;
    cbuf->desired_yoff = char_h*point_y - window_height / 2;
    point_time = 0;
}

static inline void start_of_buffer() {
    point_x = point_y = 0;
    cbuf->desired_yoff = 0;
    point_time = 0;
}

static inline void mark_whole_buffer() {
    start_of_buffer();
    mark_start(point_x, point_y);
    end_of_buffer();
    mark_update(point_x, point_y);
    end_of_buffer();
}

int main(int argc, char **argv) {
    struct Lisp *lisp;
    
    TTF_Font *font;

    Uint8 point_alpha = 255;

    char *init_fn[BUFFERS_MAX-1] = { NULL };
    size_t len = 0;
    
    char *cfg_file = "config.l";
    
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (argv[i][0] == '-') {
                if (0==strcmp(argv[i]+1, "cfg") || 0==strcmp(argv[i]+1, "config")) {
                    cfg_file = argv[++i];
                } else if (0==strcmp(argv[i]+1, "f") || 0==strcmp(argv[i]+1, "file")) {
                    init_fn[len++] = argv[i];
                } else if (0==strcmp(argv[i]+1, "mem")) {
                    memory_debug = 1;
                }
            } else {
                init_fn[len++] = argv[i];
            }
        }
    }

    lisp = lisp_interpret(cfg_file);

    /* This is slow; if we're going to have to do this for dozens of variables, find a better way. Maybe hardcode the indices? */
    
    tab_width         = invars_get_integer("tab-width");
    smooth_scroll     = invars_get_integer("smooth-scroll");
    vsync             = invars_get_integer("vsync");
    scroll_amount     = invars_get_integer("scroll-amount");
    font_size         = invars_get_integer("font-size");
    draw_text_blended = invars_get_integer("draw-text-blended");
    scroll_speed      = invars_get_float  ("scroll-speed");
    font_name         = invars_get_string ("font");

    seplen = strlen(separator_charset);
    
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    window = SDL_CreateWindow("tmacs",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              window_width,
                              window_height,
                              SDL_WINDOW_RESIZABLE);
    tassert(window);
    
    renderer = SDL_CreateRenderer(window, -1, vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
    tassert(renderer);

    running = true;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    font = TTF_OpenFont(font_name, font_size);
    if (!font) {
        fprintf(stderr, "Failed to open font.");
        return 1;
    }

    TTF_SizeText(font, "-", &char_w, &char_h);

    if (len == 0) {
        buffers[0] = buffer_new("*scratch*", false);
    } else {
        for (int i = 0; i < len; ++i) {
            buffers[i] = buffer_new("", false);
            buffers_count = i+1;
            buffer_load_file(buffers[i], init_fn[i]);
        }
    }
    
    cbuf = buffers[0];

    buffers[BUFFERS_MAX-1] = minibuf = buffer_new("minibuffer", true);
    buffers[BUFFERS_MAX-1]->x = 10;
    buffers[BUFFERS_MAX-1]->y = window_height - char_h - 3;
    
    SDL_SetWindowTitle(window, cbuf->name);
    
    last = SDL_GetTicks();
    
    while (running) {
        SDL_Event event;
        SDL_Rect point_rect, clear_rect;
        const Uint8 *keys;

        point_time += (delta = SDL_GetTicks() - last);
        last = SDL_GetTicks();

        keys = SDL_GetKeyboardState(NULL);

        point_alpha = 255 * (point_time < 500);
        if (point_time >= 1000) point_time = 0;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                    minibuf->y = window_height - char_h - 3;
                }
            }
            if (event.type == SDL_MOUSEWHEEL) {
                if (cbuf != minibuf) {
                    cbuf->desired_yoff -= char_h * scroll_amount * event.wheel.y;
                    if (cbuf->desired_yoff < 0) cbuf->desired_yoff = 0;
                }
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_x:
                    if (is_control_held(keys)) {
                        if (!isearch_mode) {
                            toggle_isearch_mode();
                        } else {
                            sx = point_x;
                            sy = point_y;
                            isearch_update_point();
                            point_time = 0;
                        }
                    }
                    break;
                case SDLK_m: if (is_control_held(keys)) {
                        minibuffer_toggle();
                        point_time = 0;
                    }
                    break;
                case SDLK_RETURN:
                    buffer_newline(true);
                    point_time = 0;
                    break;
                case SDLK_BACKSPACE:
                    if (isearch_mode) {
                        if (minibuf->lines[0].length >= 1) {
                            line_cut_char(minibuf->lines + 0, minibuf->lines[0].length-1);
                            if (minibuf->lines[0].length == 0) {
                                point_x = sx;
                                point_y = sy;
                            }
                        }
                        break;
                    }
                    cbuf->mark.on = false;
                    if (is_control_held(keys) && is_shift_held(keys)) {
                        if (cbuf->length == 1) {
                            memset(cbuf->lines[0].string, 0, cbuf->lines[0].capacity);
                            cbuf->lines[0].length = 0;
                            point_x = 0;
                        } else {
                            buffer_cutline(point_y);
                            if (point_y >= cbuf->length) point_y = cbuf->length - 1;
                        }
                    } else if (is_control_held(keys) || is_meta_held(keys)) {
                        backward_kill_word();
                    } else if (point_x > 0) {
                        line_cut_char(&cbuf->lines[point_y], --point_x);
                    } else if (point_y > 0 && cbuf->lines[point_y].length == 0) {
                        buffer_cutline(point_y--);
                        point_x = cbuf->lines[point_y].length;
                    } else if (point_y > 0 && point_x == 0) {
                        char *s = tcalloc(strlen(cbuf->lines[point_y].string + point_x)+1, 1);
                        int px;
                        
                        strcpy(s, cbuf->lines[point_y].string + point_x);

                        buffer_cutline(point_y--);
                        point_x = px = cbuf->lines[point_y].length;

                        line_insert_str(cbuf->lines + point_y, s);

                        point_x = px;

                        tfree(s);
                    }
                    break;
                case SDLK_INSERT: case SDLK_KP_0:
                    insert_mode = !insert_mode;
                    point_time = 0;
                    break;
                case SDLK_d: if (is_control_held(keys)) {
                    case SDLK_DELETE: case SDLK_KP_PERIOD:
                        if (point_x < cbuf->lines[point_y].length)
                            line_cut_char(cbuf->lines+point_y, point_x);
                    } else if (is_meta_held(keys)) { forward_kill_word(); }
                    break;
                case SDLK_SPACE:
                    break;
                case SDLK_g:
                    if (is_control_held(keys)) {
                        cbuf->mark.on = false;
                        minibuffer_log("Quit");
                        
                        isearch_mode = false;
                        if (cbuf == minibuf) {
                            point_x = 0;
                            minibuffer_toggle();
                            point_time = 0;
                        }
                    }
                    break;
                case SDLK_t:
                    if (is_control_held(keys)) {
                        line_center(cbuf->lines+point_y, char_w);
                    }
                    break;
                case SDLK_l:
                    if (is_control_held(keys)) {
                        cbuf->desired_yoff = char_h+(char_h*point_y - (window_height - (char_h * 2 - 6)) / 2);
                        if (cbuf->desired_yoff < 0) cbuf->desired_yoff = 0;
                    }
                    break;
                case SDLK_k:
                    if (is_control_held(keys)) {
                        if (cbuf->lines[point_y].length == 0) {
                            if (cbuf->length > 1) {
                                buffer_cutline(point_y);
                            }
                        } else {
                            char *end;
                            end = tcalloc(cbuf->lines[point_y].length - point_x + 1, 1);
                            strcpy(end, cbuf->lines[point_y].string + point_x);
                            end[cbuf->lines[point_y].length - point_x] = 0;

                            SDL_SetClipboardText(end);

                            line_cut_str(cbuf->lines + point_y, point_x, cbuf->lines[point_y].length);

                            tfree(end);
                        }
                        if (point_y >= cbuf->length) point_y = cbuf->length - 1;
                        if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;
                        
                        point_time = 0;
                    } else if (is_meta_held(keys)) {
                        mark_whole_buffer();
                    }
                    break;
                case SDLK_w:
                    if (cbuf->mark.on) {
                        if (is_control_held(keys)) {
                            mark_kill(cbuf);
                        } else if (is_meta_held(keys)) {
                            //mark_copy(cbuf);
                        }
                    }
                    break;

                case SDLK_v:
                    if (is_control_held(keys) && cbuf != minibuf) goto pgdn;
                    if (is_meta_held(keys) && cbuf != minibuf) goto pgup;
                    break;
                    
                case SDLK_PAGEDOWN:
                pgdn:;
                    int d = (minibuf->y-char_h);
                    d /= char_h;
                    d *= char_h;
                    cbuf->desired_yoff += d;
                    point_y += d/char_h;
                    point_time = 0;
                    break;
                case SDLK_PAGEUP:
                pgup:;
                    int e = (minibuf->y-char_h);
                    e /= char_h;
                    e *= char_h;
                    cbuf->desired_yoff -= e;
                    if (cbuf->desired_yoff < 0) cbuf->desired_yoff = 0;
                    point_y -= e/char_h;
                    point_time = 0;
                    break;
                    
                case SDLK_TAB:
                    if (is_control_held(keys)) {
                        int c = buffer_index + 1;
                        if (c >= buffers_count) c = 0;
                        buffer_switch(c);
                        cbuf = buffers[buffer_index];
                    } else {
                        if (is_shift_held(keys)) {
                            point_x = 0;
                            for (int i = 0; i < tab_width; ++i) {
                                if (cbuf->lines[point_y].string[0] != ' ') break;
                                line_cut_char(cbuf->lines + point_y, 0);
                            }
                        } else {
                            for (int i = 0; i < tab_width; ++i) {
                                line_insert_char(cbuf->lines + point_y, ' ');
                            }
                        }
                        point_time = 0;
                    }
                    break;

                case SDLK_q:
                    if (is_control_held(keys)) {
                        buffer_kill();
                    } else if (is_meta_held(keys)) {
                        minibuffer_toggle();
                        minibuffer_log("shell-command \"\"");
                        point_x = minibuf->lines[0].length-1;
                        point_time = 0;
                    }
                    break;

                case SDLK_j:
                    if (is_control_held(keys)) {
                        if (cbuf == minibuf) {
                            buffer_newline(true);
                            point_time = 0;
                        } else {
                            minibuffer_toggle();
                            minibuffer_log("switch-to ");
                            point_x = minibuf->lines[0].length;
                            point_time = 0;
                        }
                    }
                    break;
                    
                case SDLK_s:
                    if (is_control_held(keys)) {
                        if (is_shift_held(keys)) {
                            minibuffer_toggle();
                            minibuffer_log("write-to \"\"");
                            point_x = minibuf->lines[0].length-1;
                            point_time = 0;
                        } else {
                            buffer_save(cbuf);
                        }
                    }
                    break;
                    
                case SDLK_o:
                    if (is_control_held(keys) && cbuf != minibuf) {
                        minibuffer_toggle();
                        minibuffer_log("open \"\"");
                        point_x = minibuf->lines[0].length-1;
                        point_time = 0;
                    }
                    break;
                    
                case SDLK_PERIOD:
                    if (is_shift_held(keys) && is_meta_held(keys)) {
                        end_of_buffer();
                    }
                    break;
                case SDLK_COMMA:
                    if (is_shift_held(keys) && is_meta_held(keys)) {
                        start_of_buffer();
                    }
                    break;
                    
                case SDLK_UP:
                    if (is_control_held(keys)) {
                        point_backward_paragraph();
                        clamp_y_min();
                    } else {
                        goto up;
                    }
                    break;
                case SDLK_p:
                    if (is_control_held(keys)) {
                        if (is_shift_held(keys)) {
                            point_backward_paragraph();
                            clamp_y_min();
                            break;
                        } else {
                            goto up;
                        }
                    } else break;

                up:
                    point_y--;
                    clamp_y_min();

                    break;
                case SDLK_DOWN:
                    if (is_control_held(keys)) {
                        point_forward_paragraph();
                        clamp_y_max();
                    } else {
                        goto down;
                    }
                    break;
                    
                case SDLK_n:
                    if (is_control_held(keys)) {
                        if (!is_shift_held(keys)) {
                            goto down;
                        } else {
                            point_forward_paragraph();
                            clamp_y_max();
                            break;
                        }
                    } else break;
                    
                down:
                    point_y++;
                    clamp_y_max();
                     
                    point_time = 0;
                    break;
                    
                case SDLK_b:
                    if (is_meta_held(keys)) {
                        point_backward_word();
                        break;
                    }

                    if (!is_control_held(keys)) break;
                    
                    case SDLK_LEFT:
                        if (event.key.keysym.sym == SDLK_LEFT && is_control_held(keys)) {
                            point_backward_word();
                            break;
                        }
                        point_x--;
                    if (point_x < 0) point_x = 0;
                    point_time = 0;
                    
                    if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    break;
                    
                case SDLK_f:
                    if (is_meta_held(keys)) {
                        point_forward_word();
                        break;
                    }

                    if (!is_control_held(keys)) break;
                    
                    case SDLK_RIGHT:
                        if (event.key.keysym.sym == SDLK_RIGHT && is_control_held(keys)) {
                            point_forward_word();
                            break;
                        }
                        
                    point_x++;
                    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;
                    point_time = 0;
                    
                    if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    break;
                case SDLK_a:
                    if (is_control_held(keys)) {
                    case SDLK_HOME:
                    case SDLK_KP_7:
                        point_x = 0;
                        point_time = 0;
                    
                        if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                            cbuf->desired_yoff = point_y * char_h;
                        }
                    }
                    break;
                case SDLK_e:
                    if (is_control_held(keys))
                    case SDLK_END: case SDLK_KP_1:
                        point_x = cbuf->lines[point_y].length;
                    point_time = 0;
                    
                    if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    break;

                case SDLK_y:
                    if (is_control_held(keys)) {
                        char *clip = SDL_GetClipboardText();

                        while (*clip) {
                            /* Handling different end-of-line character sequences. */
                            if (*clip == '\n') {
                                buffer_newline(false);
                                LOG("LF");
                            } else if (*clip == '\r' && *(clip+1) == '\n') {
                                buffer_newline(false);
                                ++clip;
                                LOG("CRLF");
                            } else if (*clip == '\r') {
                                buffer_newline(false);
                                LOG("CR");
                            } else {
                                line_insert_char(cbuf->lines+point_y, *clip);
                            }
                            ++clip;
                        }
                    }
                    break;
                }
            }
            if (event.type == SDL_TEXTINPUT) {
                if (isearch_mode) {
                    isearch_add_char(*event.text.text);
                    isearch_update_point();
                    point_time = 0;
                } else {
                    if (*event.text.text == ' ' && is_control_held(keys)) {
                        if (cbuf->mark.on && cbuf->mark.sx == cbuf->mark.ex && cbuf->mark.sy == cbuf->mark.ey) {
                            cbuf->mark.on = false;
                            if (cbuf != minibuf)
                                minibuffer_log("Mark deactivated");
                        } else {
                            mark_start(point_x, point_y);
                            if (cbuf != minibuf)
                                minibuffer_log("Mark set");
                        }
                    } else {
                        line_insert_str(&cbuf->lines[point_y], event.text.text);
                        cbuf->mark.on = false;
                    }
                }

                point_time = 0;
            }
        }

        int mx, my;
        Uint32 mouse = SDL_GetMouseState(&mx, &my);
        
        if (smooth_scroll) {
            cbuf->yoff = lerp(cbuf->yoff, cbuf->desired_yoff, delta * scroll_speed);
        } else {
            cbuf->yoff = cbuf->desired_yoff;
        }

        if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            point_x = (int)(mx / char_w);
            point_y = (int)(my / char_h) + floor((cbuf->yoff/char_h));

            if (point_y < 0) point_y = 0;
            if (point_y >= cbuf->length) point_y = cbuf->length - 1;
            if (point_x >= cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;
            point_time = 0;
        }
        
        SDL_SetRenderDrawColor(renderer, 33, 33, 33, 255);
        SDL_RenderClear(renderer);

        point_rect.x = cbuf->x + point_x * char_w;
        point_rect.y = cbuf->y + point_y * char_h - cbuf->yoff;
        point_rect.w = char_w;
        point_rect.h = char_h;

        clear_rect.x = 0;
        clear_rect.y = minibuf->y - char_h;
        clear_rect.w = window_width;
        clear_rect.h = char_h*3;

        if (cbuf->mark.on) {
            mark_update(point_x, point_y);
            mark_draw(renderer, font);
        }
        buffer_draw(renderer, font, buffers[buffer_index]);
        
        SDL_SetRenderDrawColor(renderer, 33, 33, 33, 255);
        SDL_RenderFillRect(renderer, &clear_rect);

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, point_alpha / 3);
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 255, 200, 32);
        }

        if (insert_mode) {
            SDL_RenderDrawRect(renderer, &point_rect);
        } else {
            SDL_RenderFillRect(renderer, &point_rect);
        }

        buffer_draw(renderer, font, minibuf);

        modeline_draw(renderer, font, buffers[buffer_index]);

        if (!isearch_mode && cbuf != minibuf && (cbuf->px != point_x || cbuf->py != point_y)) {
            minibuffer_log("");
        }
            
        cbuf->px = point_x;
        cbuf->py = point_y;

        SDL_RenderPresent(renderer);
    }

    for (int i = 0; i < BUFFERS_MAX; ++i) {
        if (buffers[i]) buffer_free(buffers[i]);
    }
    
    lisp_free(lisp);

    TTF_CloseFont(font);

    print_memory_counter();
    
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    
    SDL_Quit();
    TTF_Quit();
    
    return 0;
}
