#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "SDL_keycode.h"
#include "SDL_pcf.h"
#include "SDL_surface.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

PCF_TextPlacement placement;

typedef struct{
    void *value;
    PCF_NumberType type;
}ValueSample;

ValueSample samples[3];
int nsamples = 3;
int current_sample;

int current_precision;

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
#else
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
#endif
        break;
    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

void pretty_placement(PCF_TextPlacement placement)
{
    if(placement & RightToCol)
        printf("RightToCol");
    else if(placement & CenterOnCol)
        printf("CenterOnCol");
    else if(placement & LeftToCol)
        printf("LeftToCol");
    printf(" | ");
    if(placement & BelowRow)
        printf("BelowRow");
    else if(placement & CenterOnRow)
        printf("CenterOnRow");
    else if(placement & AboveRow)
        printf("AboveRow");
    printf("\n");
}

const char *pretty_type(PCF_NumberType type)
{
    switch(type){
        case TypeInt: return "TypeInt";
        case TypeIntUnsigned: return "TypeIntUnsigned";
        case TypeFloat: return "TypeFloat";
        case TypeDouble: return "TypeDouble";
    }
    return "TypeUnknown";
}

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event)
{
    PCF_TextPlacement col;
    PCF_TextPlacement row;

    if(placement & RightToCol)
        col = RightToCol;
    else if(placement & CenterOnCol)
        col = CenterOnCol;
    else if(placement & LeftToCol)
        col = LeftToCol;

    if(placement & BelowRow)
        row = BelowRow;
    else if(placement & CenterOnRow)
        row = CenterOnRow;
    else if(placement & AboveRow)
        row = AboveRow;

    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
        case SDLK_c: //Cyle through col placement values
            if(event->state != SDL_PRESSED)
                break;
            if( col == RightToCol)
                col = CenterOnCol;
            else if (col == CenterOnCol)
                col = LeftToCol;
            else
                col = RightToCol;
            break;
        case SDLK_r: //Cycle through row placement values
            if(event->state != SDL_PRESSED)
                break;
            if( row == BelowRow)
                row = CenterOnRow;
            else if (row == CenterOnRow)
                row = AboveRow;
            else
                row = BelowRow;
            break;
        case SDLK_UP:
            if(event->state != SDL_PRESSED)
                break;
            current_precision++;
            break;
        case SDLK_DOWN:
            if(event->state != SDL_PRESSED)
                break;
            if(current_precision > 0)
                current_precision--;
            break;
        case SDLK_v: //Cycle through values
            if(event->state != SDL_PRESSED)
                break;
            if(current_sample < nsamples)
                current_sample++;
            else
                current_sample = 0;
            break;
    }
    placement = col | row;
    return false;
}

/*Return true to quit the app*/
bool handle_events(void)
{
    SDL_Event event;

    while(SDL_WaitEvent(&event) == 1){
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


int main(int argc, char *argv[])
{
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    Uint32 black, white, red;
    PCF_Font *font;
    Uint32 col, row;
    bool done;

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
    red = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    SDL_FillRect(screenSurface, NULL, black);

    col = (screenSurface->w - 1)/2; // -1  because we don't want to know how many pixels but its coordinates
    row = (screenSurface->h - 1)/2;

    SDL_LockSurface(screenSurface);
    /*Trace the vertical center line*/
    for(int x = 0; x < screenSurface->w; x++)
        putpixel(screenSurface, x, row, red);
    /*Trace the horizontal center line*/
    for(int y = 0; y < screenSurface->h; y++)
        putpixel(screenSurface, col, y, red);
    SDL_UnlockSurface(screenSurface);

    font = PCF_OpenFont("ter-x24n.pcf.gz");
    if(!font){
        printf("%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int ival = 2;
    float fval = 3.141592;
    double dval = 3.141592;

    samples[0].value = &ival;
    samples[0].type = TypeInt;
    samples[1].value = &fval;
    samples[1].type = TypeFloat;
    samples[2].value = &dval;
    samples[2].type = TypeDouble;

    done = false;
    placement = CenterOnCol | CenterOnRow;
    current_sample = 0;
    current_precision = 0;
    do{
        SDL_FillRect(screenSurface, NULL, black);
        SDL_LockSurface(screenSurface);
        /*Trace the vertical center line*/
        for(int x = 0; x < screenSurface->w; x++)
            putpixel(screenSurface, x, row, red);
        /*Trace the horizontal center line*/
        for(int y = 0; y < screenSurface->h; y++)
            putpixel(screenSurface, col, y, red);
        SDL_UnlockSurface(screenSurface);

        printf("%s - precision %d\n", pretty_type(samples[current_sample].type), current_precision);
        PCF_FontWriteNumberAt(font, samples[current_sample].value, samples[current_sample].type, current_precision, white, screenSurface, col, row, placement);
        SDL_UpdateWindowSurface(window);
        done = handle_events();
    }while(!done);

    PCF_CloseFont(font);

    SDL_DestroyWindow(window);
    SDL_Quit();

	exit(EXIT_SUCCESS);
}

