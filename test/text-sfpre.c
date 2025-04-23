#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_gpu.h>

#include "SDL_pcf.h"
#include "SDL_rect.h"
#include "SDL_stdinc.h"
#include "src/SDL_pcf.h.in"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


int testdrive_gpu(PCF_StaticFont *sfont, SDL_Rect location, Uint32 msg_w, Uint32 msg_h, size_t npatches, PCF_StaticFontPatch *patches);

int testdrive_blit(PCF_StaticFont *sfont, SDL_Rect location, Uint32 msg_w, Uint32 msg_h, size_t npatches, PCF_StaticFontPatch *patches);

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
    bool done;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    Uint32 black, white, blue;
    PCF_Font *font;
    PCF_StaticFont *sfont;
    SDL_Rect glyph;
    Uint32 msg_w,msg_h;
    int i;

    bool tight = false;
    char *message;


    if(argc < 2){
        printf("Usage: %s message [--tight]\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    for(int i = 1; i < argc; i++){
        if(!strcmp(argv[i], "--tight")){
            tight = true;
        }else{
            message = argv[i];
        }
    }


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
    blue  = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0xFF);



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
    PCF_StaticFontCreateTexture(sfont);


    int msglen = strlen(message);
    PCF_StaticFontGetSizeRequest(sfont, message, tight, &msg_w, &msg_h);

    PCF_StaticFontPatch *patches = SDL_calloc(msglen, sizeof(PCF_StaticFontPatch));

    SDL_Rect location = {SCREEN_WIDTH/2 -1, SCREEN_HEIGHT/2 -1,0 ,0};
    location.x -= (msg_w/2 -1);
    location.w = sfont->metrics.characterWidth;
    location.h = msg_h;

    SDL_Rect cursor = location;

    size_t npatches = PCF_StaticFontPreWriteString(sfont, msglen, message, tight, &cursor, msglen, patches);


    SDL_FillRect(screenSurface, NULL, black);

    SDL_Rect background = (SDL_Rect){location.x, location.y, msg_w, msg_h};
    SDL_FillRect(screenSurface, &background, blue);

    int j = 0;
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;

        done = handle_events();
        if( j < npatches){
            SDL_Rect dst = (SDL_Rect){
                patches[j].dst.x,patches[j].dst.y,
                patches[j].src.w,patches[j].src.h
            };
            SDL_BlitSurface(sfont->raster, &patches[j].src, screenSurface, &dst);
            j++;
        }
        SDL_UpdateWindowSurface(window);

        if(elapsed < 400){
            SDL_Delay(400 - elapsed);
        }
        last_ticks = ticks;
    }while(!done);
    PCF_FreeStaticFont(sfont);

    SDL_DestroyWindow(window);
    SDL_Quit();
}

