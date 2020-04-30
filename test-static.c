#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "SDL_pcf.h"
#include "SDL_surface.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event)
{
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
    }
    return false;
}

/*Return true to quit the app*/
bool handle_events(void)
{
    SDL_Event event;

    while(SDL_PollEvent(&event) == 1){
        switch(event.type){
            case SDL_QUIT:
                return true;
                break;
        case SDL_WINDOWEVENT:
            if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                return true;
            break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                return handle_keyboard(&(event.key));
                break;
        }
    }
    return false;
}


int main(int argc, char **argv)
{
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    Uint32 black, white;
    PCF_Font *font;
    PCF_StaticFont *sfont;
    Uint32 msg_w,msg_h;
    bool done;
    int i;


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

    font = PCF_OpenFont("ter-x24n.pcf.gz");
    if(!font){
        printf("%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    sfont = PCF_FontCreateStaticFont(
        font,
        &(SDL_Color){255,255,255,SDL_ALPHA_OPAQUE},
        1,
        PCF_ALPHA
    );
    PCF_CloseFont(font);
    if(!sfont){
        printf("%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }


    white = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    black  = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00);

    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;

    SDL_FillRect(screenSurface, NULL, black);

    char *message = "All your bases are belong to us";
    int msglen = strlen(message);
    PCF_StaticFontGetSizeRequest(message, sfont, &msg_w, &msg_h);

    SDL_Rect location = {SCREEN_WIDTH/2 -1, SCREEN_HEIGHT/2 -1,0 ,0};
    location.x -= (msg_w/2 -1);
    int j = 0;
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;

        done = handle_events();
        if( j < msglen){
            PCF_StaticFontWriteChar(sfont, message[j], white, screenSurface, &location);
            j++;
        }

        SDL_UpdateWindowSurface(window);

        if(elapsed < 400){
            SDL_Delay(400 - elapsed);
        }
        last_ticks = ticks;
    }while(!done);
    SDL_PcfFreeStaticFont(sfont);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

}
