#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "SDL_pcf.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int main(int argc, char *argv[])
{
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    Uint32 black, white;
    PCF_Font *font;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(
                "SDL_pcf test drive",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                SDL_WINDOW_SHOWN
                );
    if (window == NULL) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return 1;
    }

    screenSurface = SDL_GetWindowSurface(window);
    if(!screenSurface){
        printf("Error: %s\n",SDL_GetError());
        exit(-1);
    }

    white = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    black  = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00);
    SDL_FillRect(screenSurface, NULL, black);

    font = PCF_OpenFont("ter-x24n.pcf.gz");
    if(!font){
        printf("%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    PCF_FontWrite(font, "Hello, World!", white, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);

    SDL_Delay(4000);

    PCF_CloseFont(font);

    SDL_DestroyWindow(window);
    SDL_Quit();

	exit(EXIT_SUCCESS);
}

