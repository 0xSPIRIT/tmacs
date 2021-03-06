#include "modeline.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string.h>
#include <time.h>

#include "util.h"
#include "lisp.h"

/* Draws the modeline to the screen. */
void modeline_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buffer) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect, line;
    SDL_Color color = { 0, 0, 0, 255 };

    char str[64] = {0};

    int char_w, char_h;

    TTF_SizeText(font, buffer->name, &char_w, &char_h);

    line.x = buffer->x;
    line.y = buffer->y + (window_height - char_h * 2 - 6);
    line.w = buffer->w;
    line.h = char_h;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &line);

    sprintf(str, "%s   %d%% L%d", buffer->name, (int)(100*(cbuf->desired_yoff / (float)char_h)/buffer->length), point_y);

    if (draw_text_blended) {
        surface = TTF_RenderText_Blended(font, str, color);
    } else {
        surface = TTF_RenderText_Solid(font, str, color);
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    rect.x = buffer->x + 12;
    rect.y = minibuf->y - char_h - 3;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

