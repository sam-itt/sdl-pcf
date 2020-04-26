#ifndef SDL_PCF_H
#define SDL_PCF_H
#include <stdbool.h>

#include "SDL_surface.h"
#include "pcfread.h"

typedef struct{
    FontRec xfont;
}SDL_PcfFont;


SDL_PcfFont *SDL_PcfOpenFont(const char *filename);
void SDL_PcfCloseFont(SDL_PcfFont *self);
bool SDL_PcfWriteChar(int c, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool SDL_PcfWrite(const char *str, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void SDL_PcfGetSizeRequest(const char *str, SDL_PcfFont *font, Uint32 *w, Uint32 *h);


void SDL_PcfDumpGlpyh(SDL_PcfFont *font, int c);
#endif /* SDL_PCF_H */
