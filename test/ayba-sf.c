#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>

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
    SDL_Rect glyph;
    Uint32 msg_w,msg_h;
    bool done;
    int i;

    SDL_Renderer *renderer;
    bool use_renderer = false;

    if(argc > 1){
        printf("Using the renderer\n");
        use_renderer = true;
    }else{
        printf("Using SDL_BlitSurface\n");
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

    if(use_renderer){
        SDL_RendererInfo rinfo;
        renderer = SDL_CreateRenderer(window, -1, 0);
        if (renderer == NULL) {
            fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
            return 1;
        }
        SDL_GetRendererInfo(renderer, &rinfo);
        printf("Got renderer: %s\n", rinfo.name);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }else{
        screenSurface = SDL_GetWindowSurface(window);
        if(!screenSurface){
            printf("Error: %s\n",SDL_GetError());
            exit(-1);
        }
        white = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
        black  = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00);
        SDL_FillRect(screenSurface, NULL, black);
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
    if(use_renderer)
        PCF_StaticFontCreateTexture(sfont, renderer);


    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;


    char *message = "All your bases are belong to us";
    int msglen = strlen(message);
    PCF_StaticFontGetSizeRequest(sfont, message, &msg_w, &msg_h);

    SDL_Rect location = {SCREEN_WIDTH/2 -1, SCREEN_HEIGHT/2 -1,0 ,0};
    location.x -= (msg_w/2 -1);
    /*Ignored by SDL_BlitSurface, but required by SDL_Render*/
    location.w = sfont->metrics.characterWidth;
    location.h = sfont->metrics.ascent + sfont->metrics.descent;
    int j = 0;
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;

        done = handle_events();
        if( j < msglen){
            if(PCF_StaticFontGetCharRect(sfont, message[j], &glyph)){
                if(use_renderer)
                    SDL_RenderCopy(renderer, sfont->texture, &glyph, &location);
                else
                    SDL_BlitSurface(sfont->raster, &glyph, screenSurface, &location);
            }
            location.x += sfont->metrics.characterWidth;
            j++;
        }
        if(use_renderer)
            SDL_RenderPresent(renderer);
        else
            SDL_UpdateWindowSurface(window);

        if(elapsed < 400){
            SDL_Delay(400 - elapsed);
        }
        last_ticks = ticks;
    }while(!done);
    PCF_FreeStaticFont(sfont);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

}
