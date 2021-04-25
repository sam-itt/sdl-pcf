#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_gpu.h>

#include "SDL_pcf.h"

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
    GPU_Target* gpu_screen = NULL;

    Uint32 black, white;
    PCF_Font *font;
    PCF_StaticFont *sfont;
    SDL_Rect glyph;
    GPU_Rect g_glyph;
    Uint32 msg_w,msg_h;
    bool done;
    int i;

    if(PCF_TEXTURE_TYPE != PCF_TEXTURE_GPU){
        printf("Cannot run SDL_gpu sample, no SDL_gpu support in SDL_pcf\n");
        exit(EXIT_FAILURE);
    }
    printf("Using SDL_gpu GPU_Blit\n");

    GPU_SetRequiredFeatures(GPU_FEATURE_BASIC_SHADERS);
    gpu_screen = GPU_InitRenderer(GPU_RENDERER_OPENGL_2, SCREEN_WIDTH, SCREEN_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
    if(gpu_screen == NULL){
        GPU_LogError("Initialization Error: Could not create a renderer with proper feature support for this demo.\n");
        return 1;
    }

    GPU_ClearRGB(gpu_screen, 0x00, 0x00, 0x00);

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
                g_glyph = (GPU_Rect){glyph.x, glyph.y, glyph.w, glyph.h};
                GPU_Blit(sfont->texture, &g_glyph, gpu_screen,
                    g_glyph.w/2.0 + location.x,
                    g_glyph.h/2.0 + location.y
                );
                /* Not waiting before GPU_Flip'ing the first char will result
                 * in chars being discarded. SDL_gpu bug? */
                if(j == 0)
                    SDL_Delay(500);
            }
            location.x += sfont->metrics.characterWidth;
            j++;
        }
        GPU_Flip(gpu_screen);

        if(elapsed < 400){
            SDL_Delay(400 - elapsed);
        }
        last_ticks = ticks;
    }while(!done);
    PCF_FreeStaticFont(sfont);
    GPU_Quit();
    return 0;
}
