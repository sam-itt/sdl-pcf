#ifndef SDL_PCF_H
#define SDL_PCF_H
#include <stdbool.h>

#include "SDL_surface.h"
#include "pcfread.h"

#define PCF_LOWER_CASE "abcdefghijklmnopqrstuvwxyz"
#define PCF_UPPER_CASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define PCF_ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define PCF_DIGITS "0123456789"

typedef struct{
    FontRec xfont;
}SDL_PcfFont;

typedef struct{
    SDL_Surface *raster;
    char *glyphs;
    Uint16 nglyphs;
    xCharInfo   metrics;
}SDL_PcfStaticFont;

SDL_PcfFont *SDL_PcfOpenFont(const char *filename);
void SDL_PcfCloseFont(SDL_PcfFont *self);
bool SDL_PcfWriteChar(int c, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool SDL_PcfWrite(const char *str, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void SDL_PcfGetSizeRequest(const char *str, SDL_PcfFont *font, Uint32 *w, Uint32 *h);

void SDL_PcfDumpGlpyh(SDL_PcfFont *font, int c);


SDL_PcfStaticFont *SDL_PcfCreateStaticFont(SDL_PcfFont *font, SDL_Color *color, int nsets, ...);
void SDL_PcfFreeStaticFont(SDL_PcfStaticFont *self);
bool SDL_PcfStaticFontWriteChar(SDL_PcfStaticFont *font, int c, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool SDL_PcfStaticFontWrite(const char *str, SDL_PcfStaticFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void SDL_PcfStaticFontGetSizeRequest(const char *str, SDL_PcfStaticFont *font, Uint32 *w, Uint32 *h);

#endif /* SDL_PCF_H */
