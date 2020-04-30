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
}PCF_Font;

typedef struct{
    SDL_Surface *raster;
    char *glyphs;
    Uint16 nglyphs;
    xCharInfo   metrics;
}PCF_StaticFont;

PCF_Font *PCF_OpenFont(const char *filename);
void PCF_CloseFont(PCF_Font *self);
bool PCF_FontWriteChar(int c, PCF_Font *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool PCF_FontWrite(const char *str, PCF_Font *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void PCF_FontGetSizeRequest(const char *str, PCF_Font *font, Uint32 *w, Uint32 *h);

void PCF_FontDumpGlpyh(PCF_Font *font, int c);


PCF_StaticFont *PCF_FontCreateStaticFont(PCF_Font *font, SDL_Color *color, int nsets, ...);
void PCF_FreeStaticFont(PCF_StaticFont *self);
bool PCF_StaticFontWriteChar(PCF_StaticFont *font, int c, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool PCF_StaticFontWrite(const char *str, PCF_StaticFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void PCF_StaticFontGetSizeRequest(const char *str, PCF_StaticFont *font, Uint32 *w, Uint32 *h);

#endif /* SDL_PCF_H */
