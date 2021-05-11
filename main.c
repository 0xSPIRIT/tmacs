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

SDL_Window *window;
SDL_Renderer *renderer;

int window_width = 640;
int window_height = 720;

bool running;
    
int main(int argc, char **argv) {
    struct Lisp *lisp;
    
    TTF_Font *font;

    int char_w, char_h;
    Uint8 point_alpha = 255;

    char *init_fn = NULL;
    
    if (argc == 2) {
        init_fn = argv[1];
    }

    lisp = lisp_interpret("config.l");

    lisp_free(lisp);

    /* This is slow; if we're going to have to do this for dozens of variables, find a better way. Maybe hardcode the indices? */
    
    tab_width         = invars_get_integer("tab-width");
    smooth_scroll     = invars_get_integer("smooth-scroll");
    vsync             = invars_get_integer("vsync");
    scroll_amount     = invars_get_integer("scroll-amount");
    font_size         = invars_get_integer("font-size");
    draw_text_blended = invars_get_integer("draw-text-blended");
    scroll_speed      = invars_get_float  ("scroll-speed");
    font_name         = invars_get_string ("font");

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    window = SDL_CreateWindow("tmacs",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              window_width,
                              window_height,
                              SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Could not create SDL Window");
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
    running = true;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    font = TTF_OpenFont(font_name, font_size);
    if (!font) {
        fprintf(stderr, "Failed to open font.");
        return 1;
    }

    TTF_SizeText(font, "-", &char_w, &char_h);

    buffers[0] = buffer_new("*scratch*", false);
    cbuf = buffers[0];
    if (init_fn) {
        buffer_load_file(cbuf, init_fn);
    }

    buffers[BUFFERS_MAX-1] = minibuf = buffer_new("minibuffer", true);
    buffers[BUFFERS_MAX-1]->x = 10;
    buffers[BUFFERS_MAX-1]->y = window_height - char_h - 3;
    
    SDL_SetWindowTitle(window, cbuf->name);

    Uint32 point_time = 0,
        last = SDL_GetTicks(),
        delta = 0;

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
                case SDLK_m: if (is_control_held(keys)) {
                        minibuffer_toggle();
                        point_time = 0;
                    }
                    break;
                case SDLK_RETURN:
                    buffer_newline();
                    point_time = 0;
                    break;
                case SDLK_BACKSPACE:
                    if (is_control_held(keys) && is_shift_held(keys)) {
                        if (cbuf->length == 1) {
                            memset(cbuf->lines[0].string, 0, cbuf->lines[0].capacity);
                            cbuf->lines[0].length = 0;
                            point_x = 0;
                        } else {
                            buffer_cutline(point_y);
                            if (point_y >= cbuf->length) point_y = cbuf->length - 1;
                        }
                    } else if (point_x > 0) {
                        line_cut_char(&cbuf->lines[point_y], --point_x);
                    } else if (point_y > 0 && cbuf->lines[point_y].length == 0) {
                        buffer_cutline(point_y--);
                        point_x = cbuf->lines[point_y].length;
                    } else if (point_y > 0 && point_x == 0) {
                        char *s = talloc(strlen(cbuf->lines[point_y].string + point_x)+1);
                        int px;
                        
                        strcpy(s, cbuf->lines[point_y].string + point_x);

                        buffer_cutline(point_y--);
                        point_x = px = cbuf->lines[point_y].length;

                        line_insert_str(cbuf->lines + point_y, s);

                        point_x = px;

                        free(s);
                    }
                    break;
                case SDLK_INSERT: case SDLK_KP_0:
                    insert_mode = !insert_mode;
                    point_time = 0;
                    break;
                case SDLK_d: if (is_control_held(keys)) case SDLK_DELETE: case SDLK_KP_PERIOD:
                    if (point_x < cbuf->lines[point_y].length)
                        line_cut_char(cbuf->lines+point_y, point_x);
                    break;
                case SDLK_SPACE:
                    break;
                case SDLK_g:
                     if (is_control_held(keys)) {
                        mark_reset();
                        minibuffer_log("");
                        if (cbuf == minibuf) {
                            point_x = 0;
                            minibuffer_toggle();
                            point_time = 0;
                        }
                    }
                    break;
                case SDLK_c:
                    if (is_control_held(keys)) {
                        mark_copy(cbuf);
                    }
                    break;
                case SDLK_q:
                    if (is_control_held(keys)) {
                        line_center(cbuf->lines+point_y, char_w);
                    }
                    break;
                case SDLK_l:
                    if (is_control_held(keys)) {
                        cbuf->desired_yoff = char_h+(char_h*point_y - (window_height - (char_h * 2 - 6)) / 2);
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

                            free(end);
                        }
                    }
                    if (point_y >= cbuf->length) point_y = cbuf->length - 1;
                    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;
                    break;
                case SDLK_x:
                    if (is_control_held(keys) && mark.on) {
                        mark_kill(cbuf);
                    }
                    break;
                    
                case SDLK_PAGEDOWN:;
                    int d = (minibuf->y-char_h);
                    d /= char_h;
                    d *= char_h;
                    cbuf->desired_yoff += d;
                    break;
                case SDLK_PAGEUP:;
                    int e = (minibuf->y-char_h);
                    e /= char_h;
                    e *= char_h;
                    cbuf->desired_yoff -= e;
                    if (cbuf->desired_yoff < 0) cbuf->desired_yoff = 0;
                    break;
                    
                case SDLK_TAB:
                    if (is_control_held(keys)) {
                        int c = buffer_index;
                        c++;
                        if (c >= buffers_count) c = 0;
                        buffer_switch(c);
                        cbuf = buffers[buffer_index];
                    } else {
                        for (int i = 0; i < tab_width; ++i) {
                            line_insert_char(cbuf->lines + point_y, ' ');
                        }
                        point_time = 0;
                    }
                    break;

                case SDLK_w:
                    if (is_control_held(keys)) {
                        buffer_kill();
                    }
                    break;

                case SDLK_j:
                    if (is_control_held(keys)) {
                        if (cbuf == minibuf) {
                            buffer_newline();
                            point_time = 0;
                        } else {
                            minibuffer_toggle();
                            minibuffer_log("switch ");
                            point_x = minibuf->lines[0].length;
                            point_time = 0;
                        }
                    }
                    break;
                    
                case SDLK_s:
                    if (is_control_held(keys)) {
                        if (is_shift_held(keys)) {
                            minibuffer_toggle();
                            minibuffer_log("write ");
                            point_x = minibuf->lines[0].length;
                            point_time = 0;
                        } else {
                            buffer_save(cbuf);
                        }
                    }
                    break;
                    
                case SDLK_o:
                    if (is_control_held(keys)) {
                        minibuffer_toggle();
                        minibuffer_log("open ");
                        point_x = minibuf->lines[0].length;
                        point_time = 0;
                    }
                    break;
                case SDLK_p: if (is_control_held(keys))
                case SDLK_UP:
                    point_y--;
                    if (point_y < 0) point_y = 0;
                    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;

                    if (cbuf != minibuf && point_y * char_h < cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    
                    point_time = 0;
                    break;
                case SDLK_n: if (is_control_held(keys))
                case SDLK_DOWN:
                    point_y++;
                    if (point_y >= cbuf->length) point_y = cbuf->length - 1;
                    if (point_x > cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;

                    if (cbuf != minibuf && point_y * char_h >= char_h * (int)((cbuf->desired_yoff + minibuf->y - char_h*2) / char_h)) {
                        cbuf->desired_yoff = (point_y * char_h) - char_h * ((int)(minibuf->y - char_h*2) / char_h);
                    }
                     
                    point_time = 0;
                    break;
                case SDLK_b:
                    if (is_meta_held(keys)) {
                        while (point_x > 0 && isspace(cbuf->lines[point_y].string[--point_x]));
                        while (point_x > 0 && !isspace(cbuf->lines[point_y].string[--point_x]));
                    }

                    if (is_control_held(keys))
                case SDLK_LEFT:
                    point_x--;
                    if (point_x < 0) point_x = 0;
                    point_time = 0;
                    
                    if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    break;
                    
                case SDLK_f:
                    if (is_meta_held(keys)) {
                        while (point_x < cbuf->lines[point_y].length && isspace(cbuf->lines[point_y].string[point_x++]));
                        while (point_x < cbuf->lines[point_y].length && !isspace(cbuf->lines[point_y].string[point_x++]));
                    }
                    
                    if (is_control_held(keys))
                case SDLK_RIGHT:
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
                case SDLK_e: if (is_control_held(keys))
                case SDLK_END: case SDLK_KP_1:
                    point_x = cbuf->lines[point_y].length;
                    point_time = 0;
                    
                    if (cbuf != minibuf && point_y*char_h > window_height-char_h*2 + cbuf->desired_yoff) {
                        cbuf->desired_yoff = point_y * char_h;
                    }
                    break;

                case SDLK_v:
                    if (is_control_held(keys)) {
                        char *cb;
                        cb = SDL_GetClipboardText();

                        while (*cb) {
                            /* Handling different end-of-line character sequences. */
                            if (*cb == '\n') {
                                buffer_newline();
                            } else if (*cb == '\r' && *(cb+1) == '\n') {
                                buffer_newline();
                                ++cb;
                            } else if (*cb == '\r') {
                                buffer_newline();
                            } else {
                                line_insert_char(cbuf->lines+point_y, *cb);
                            }
                            ++cb;
                        }
                    }
                    break;
                }
            }
            if (event.type == SDL_TEXTINPUT) {
                if (*event.text.text == ' ' && is_control_held(keys)) {
                    mark_start(point_x, point_y);
                } else {
                    line_insert_str(&cbuf->lines[point_y], event.text.text);
                }

                point_time = 0;

                if (cbuf != minibuf)
                    minibuffer_log("");
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
            point_y = (int)(my / char_h) + ((int)(cbuf->yoff/char_h));

            if (point_y < 0) point_y = 0;
            if (point_y >= cbuf->length) point_y = cbuf->length - 1;
            if (point_x >= cbuf->lines[point_y].length) point_x = cbuf->lines[point_y].length;
            point_time = 0;
        }
        
        SDL_SetRenderDrawColor(renderer, 33, 33, 33, 255);
        SDL_RenderClear(renderer);

        if (mark.on) {
            mark_update(point_x, point_y);
            mark_draw(renderer, cbuf, font);
        }

        point_rect.x = cbuf->x + point_x * char_w;
        point_rect.y = cbuf->y + point_y * char_h - cbuf->yoff;
        point_rect.w = insert_mode ? char_w : 1;
        point_rect.h = char_h;

        clear_rect.x = 0;
        clear_rect.y = minibuf->y - char_h;
        clear_rect.w = window_width;
        clear_rect.h = char_h*3;
        
        buffer_draw(renderer, font, buffers[buffer_index]);
            
        SDL_SetRenderDrawColor(renderer, 33, 33, 33, 255);
        SDL_RenderFillRect(renderer, &clear_rect);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, point_alpha);
        SDL_RenderDrawRect(renderer, &point_rect);

        buffer_draw(renderer, font, minibuf);

        modeline_draw(renderer, font, buffers[buffer_index]);
            
        SDL_RenderPresent(renderer);

        cbuf->px = point_x;
        cbuf->py = point_y;
    }

    for (int i = 0; i < BUFFERS_MAX; ++i) {
        if (buffers[i]) buffer_free(buffers[i]);
    }

    TTF_CloseFont(font);
    
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    
    SDL_Quit();
    TTF_Quit();
    
    return 0;
}
