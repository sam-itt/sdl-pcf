#ifndef SDL_PCF_H
#define SDL_PCF_H
#include <stdbool.h>

#include "SDL_surface.h"
#include "SDL_render.h"
#include "pcfread.h"

#define PCF_LOWER_CASE "abcdefghijklmnopqrstuvwxyz"
#define PCF_UPPER_CASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define PCF_ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define PCF_DIGITS "0123456789"

typedef struct{
    FontRec xfont;
}PCF_Font;

typedef struct{
    int refcnt;
    SDL_Surface *raster;
    char *glyphs;
    Uint16 nglyphs;
    xCharInfo   metrics;
    SDL_Color text_color;
#if HAVE_SDL2
    SDL_Texture *texture;
#endif
}PCF_StaticFont;

PCF_Font *PCF_OpenFont(const char *filename);
void PCF_CloseFont(PCF_Font *self);
bool PCF_FontWriteChar(PCF_Font *font, int c, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
bool PCF_FontWrite(PCF_Font *font, const char *str, Uint32 color, SDL_Surface *destination, SDL_Rect *location);
void PCF_FontGetSizeRequest(PCF_Font *font, const char *str, Uint32 *w, Uint32 *h);
void PCF_FontGetSizeRequestRect(PCF_Font *font, const char *str, SDL_Rect *rect);

PCF_StaticFont *PCF_FontCreateStaticFont(PCF_Font *font, SDL_Color *color, int nsets, ...);
PCF_StaticFont *PCF_FontCreateStaticFontVA(PCF_Font *font, SDL_Color *color, int nsets, size_t tlen, va_list ap);
void PCF_FreeStaticFont(PCF_StaticFont *self);
int PCF_StaticFontGetCharRect(PCF_StaticFont *font, int c, SDL_Rect *glyph);
void PCF_StaticFontGetSizeRequest(PCF_StaticFont *font, const char *str, Uint32 *w, Uint32 *h);
void PCF_StaticFontGetSizeRequestRect(PCF_StaticFont *font, const char *str, SDL_Rect *rect);
bool PCF_StaticFontCanWrite(PCF_StaticFont *font, SDL_Color *color, const char *sequence);
#if HAVE_SDL2
void PCF_StaticFontCreateTexture(PCF_StaticFont *font, SDL_Renderer *renderer);
#endif


void PCF_FontDumpGlpyh(PCF_Font *font, int c);
#endif /* SDL_PCF_H */
