#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_gpu.h>

#include "SDL_pcf.h"
#include "SDL_stdinc.h"
#include "src/SDL_pcf.h.in"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int xoffset = 0;
int yoffset = 0;

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event)
{
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
        case SDLK_LEFT:
            xoffset--;
            break;
        case SDLK_RIGHT:
            xoffset++;
            break;
        case SDLK_UP:
            yoffset++;
            break;
        case SDLK_DOWN:
            yoffset--;
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
    GPU_Rect glyph_grect;
    Uint32 msg_w,msg_h;
    bool done;
    int i;
    char *message;

    if(argc < 2){
        printf("Usage: %s message\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    message = argv[1];

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


    int msglen = strlen(message);
    PCF_StaticFontGetSizeRequest(sfont, message, &msg_w, &msg_h);

    PCF_StaticFontRectPatch *patches = SDL_calloc(msglen, sizeof(PCF_StaticFontRectPatch));


    SDL_Rect location = {SCREEN_WIDTH/2 -1, SCREEN_HEIGHT/2 -1,0 ,0};
    location.x -= (msg_w/2 -1);
    location.w = msg_w;
    location.h = msg_h;

    GPU_Rect str_bg = (GPU_Rect){location.x, location.y, msg_w, msg_h};
    GPU_RectangleFilled2(gpu_screen, str_bg, (SDL_Color){0,0,255,SDL_ALPHA_OPAQUE});

    bool dirty = true;
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        int oldxoffset, oldyoffset;

        oldxoffset = xoffset;
        oldyoffset = yoffset;
        done = handle_events();
        if(dirty || oldxoffset != xoffset || oldyoffset != yoffset){
            SDL_Rect loc = location;
            GPU_RectangleFilled2(gpu_screen, str_bg, (SDL_Color){0,0,255,SDL_ALPHA_OPAQUE});
            size_t npatches = PCF_StaticFontPreWriteStringOffset(sfont,
                                                                msglen, message,
                                                                &location,
                                                                xoffset, yoffset,
                                                                msglen, patches);
            for(int j = 0; j < npatches; j++ ){
                if(patches[j].src.x < 0) continue;
                glyph_grect = (GPU_Rect){patches[j].src.x, patches[j].src.y, patches[j].src.w, patches[j].src.h};
                float x,y;
                x = glyph_grect.w/2.0 + patches[j].dst.x;
                y = glyph_grect.h/2.0 + patches[j].dst.y;

                GPU_Blit(sfont->texture, &glyph_grect, gpu_screen, x, y);
            }
            GPU_Flip(gpu_screen);
            dirty = false;
        }

        last_ticks = ticks;
    }while(!done);
    PCF_FreeStaticFont(sfont);
    GPU_Quit();

    return 0;
}
