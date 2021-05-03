#include "modeline.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string.h>
#include <time.h>

#include "util.h"

/* Draws the modeline to the screen. */
void modeline_draw(SDL_Renderer *renderer, TTF_Font *font, struct Buffer *buffer) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect, line;
    SDL_Color color = { 0, 0, 0, 255 };

    time_t mytime;
    char *time_str = NULL;

    char str[64] = {0};

    int char_w, char_h;

    TTF_SizeText(font, buffer->name, &char_w, &char_h);

    line.x = 0;
    line.y = window_height - char_h * 2 - 6;
    line.w = window_width;
    line.h = char_h;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &line);

    sprintf(str, "%s   %d%% L%d", buffer->name, 100*point_y/buffer->length, point_y);
    
    surface = TTF_RenderText_Blended(font, str, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    rect.x = 12;
    rect.y = minibuf->y - char_h - 3;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    
    mytime = time(NULL);
    time_str = ctime(&mytime);
    time_str[strlen(time_str)-1] = 0;

    surface = TTF_RenderText_Blended(font, time_str, color);
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    rect.x = window_width - surface->w - 12;
    rect.y = minibuf->y - char_h - 3;
    rect.w = surface->w;
    rect.h = surface->h;
        
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

