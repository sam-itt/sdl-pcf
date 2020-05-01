#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "SDL_error.h"
#include "SDL_pixels.h"
#include "pcf.h"
#include "pcfread.h"
#include "SDL_pcf.h"
#include "SDL_GzRW.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"

typedef void (*PixelLighter)(Uint8 *ptr, Uint32 color);



PCF_Font *PCF_FontInit(PCF_Font *self, const char *filename)
{
    SDL_RWops *stream;
    int glyph = 4; /*see pcfReadFont comments in pcfread.c*/
    int scan = 1;
    int rv;


    stream = SDL_RWFromGzFile(filename, "rb");
    if(!stream){
        return NULL;/*SDL Error has been set by the reader*/
    }

    rv = pcfReadFont(&(self->xfont), stream, LSBFirst, LSBFirst, glyph, scan);
    SDL_RWclose(stream);
    SDL_FreeRW(stream);
    if(rv != Successful)
        return NULL;

    return self;
}

/**
 * Opens a PCF font file. Supports both .pcf and .pcf.gz.
 *
 * @param filename The file to open
 * @returns a PCF_Font opaque struct representing the font.
 * The caller must call PCF_CloseFont when done using the font.
 */
PCF_Font *PCF_OpenFont(const char *filename)
{
    PCF_Font *rv;

    rv = SDL_calloc(1, sizeof(PCF_Font));
    if(!PCF_FontInit(rv, filename)){
        SDL_free(rv);
        return NULL;
    }
    return rv;
}

/**
 * Free resources taken up by a loaded font.
 * Caller code must always call PCF_CloseFont on all fonts
 * it allocates. Each PCF_OpenFont must be paired with a
 * matching PCF_CloseFont.
 *
 * @param self The font to free.
 */
void PCF_CloseFont(PCF_Font *self)
{
    pcfUnloadFont(&(self->xfont));
    SDL_free(self);
}

static void lit_pixel_1bpp(Uint8 *ptr, Uint32 color)
{
    *ptr = color;
}

static void lit_pixel_2bpp(Uint8 *ptr, Uint32 color)
{
    *(Uint16 *)ptr = color;
}

static void lit_pixel_3bpp(Uint8 *ptr, Uint32 color)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    ptr[0] = (color >> 16) & 0xff;
    ptr[1] = (color >> 8) & 0xff;
    ptr[2] = color & 0xff;
#else
    ptr[0] = color & 0xff;
    ptr[1] = (color >> 8) & 0xff;
    ptr[2] = (color >> 16) & 0xff;
#endif
}

static void lit_pixel_4bpp(Uint8 *ptr, Uint32 color)
{
    *(Uint32 *)ptr = color;
}

static PixelLighter SDL_SurfaceGetLighter(SDL_Surface *surface)
{

    switch(surface->format->BytesPerPixel) { //
    case 1:
        return lit_pixel_1bpp;
    case 2:
        return lit_pixel_2bpp;
    case 3:
        return lit_pixel_3bpp;
    case 4:
        return lit_pixel_4bpp;
    default:
        return NULL; /*Shouldn't be reached*/
    }
}

/**
 * Writes a character on screen, and advance the location by one char width.
 * If the surface is too small to fit the char or if the glyph is partly out
 * of the surface (start writing a 18 pixel wide char 2 pixels before the edge)
 * only the pixels that can be written will be drawn, resulting in a partly
 * drawn glyph and the function will return false.
 *
 * @param c The ASCII code of the char to write. You can of course use 'a'
 * instead of 97.
 * @param font The font to use to write the char. Opened by PCF_OpenFont.
 * @param color The color of text. Must be in @param destination format (use
 * SDL_MapRGB/SDL_MapRGBA to build a suitable value).
 * @param destination The surface to write to.
 * @param location Where to write on the surface. Can be NULL to write at
 * 0,0. If not NULL, location will be advanced by the width.
 * @return True on success(the whole char has been written), false on error/partial
 * draw. Details of the failure can be retreived with SDL_GetError().
 */
bool PCF_FontWriteChar(PCF_Font *font, int c, Uint32 color, SDL_Surface *destination, SDL_Rect *location)
{
    int w, h;
    int line_bsize;
    CharInfoRec *glyph;
    BitmapFontRec *bitmapFont;
    unsigned char byte;
    unsigned char *glyph_line;
    int nbytes;
    bool rv;
    Uint8 *pixels, *line_start, *xlimit;
    PixelLighter lit_pixel;
    int line_y;
    rv = true;

    if(c == ' ')
        goto end;

    location = location ? location : &(SDL_Rect){0,0,0,0};

    lit_pixel = SDL_SurfaceGetLighter(destination);
    if(!lit_pixel){
        SDL_SetError("%s: no function to lit pixels on %d bpp surfaces such as %p",
            __FUNCTION__,
            destination->format->BytesPerPixel,
            destination
        );
        return false;
    }

    bitmapFont  = font->xfont.fontPrivate;
    if(c >= bitmapFont->num_chars){
        SDL_SetError("%s: no glyph for char %d, falling back to default glyph", __FUNCTION__, c);
        glyph = font->xfont.fontPrivate->pDefault;
        rv = false;
    }else{
        glyph = &bitmapFont->metrics[c];
    }

    /*TODO: Check if can do with FontRec struct members*/
    w = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
    h = glyph->metrics.ascent + glyph->metrics.descent;

    /*start after the end of the surface, nothing to draw*/
    if(location->x >= destination->w || location->y >= destination->h){
        return false;
    }

    /* FontRec.Glyph is line padding in number of bytes. See pcfReadFont
     * comments for a detailed explaination
     * */
    line_bsize = ceil(w/(font->xfont.glyph * 8.0))*font->xfont.glyph; /*in bytes*/
    nbytes = ceil(w/(8.0)); /*actual glyph width in bytes (w/o padding)*/
    SDL_LockSurface(destination);
    for(int i = 0; i < h; i++){
        glyph_line = (unsigned char*)glyph->bits + (i * line_bsize);
        line_y = location->y+i < destination->h ? location->y+i : destination->h-1; /*clip y*/
        line_start = (Uint8 *)destination->pixels + (line_y * destination->pitch);
        pixels = line_start + location->x * destination->format->BytesPerPixel;
        xlimit = line_start + destination->w * destination->format->BytesPerPixel;
        for(int j = 0; j < nbytes; j++){
            byte = *(unsigned char*)(glyph_line + j);
            for(int k = 0; k < 8; k++){
                if(byte & (1 << k)){
                    if(pixels < xlimit) /*Clip x*/
                        lit_pixel(pixels, color);
                }
                pixels += destination->format->BytesPerPixel;
            }
        }
    }
    SDL_UnlockSurface(destination);

end:
    location->x += font->xfont.fontPrivate->metrics->metrics.characterWidth;
    return rv;
}

/**
 * Writes a string on screen. This function will try it's best to write
 * as many chars as possible: If the surface is not wide enough to accomodate
 * the whole string, it will stop at the very last pixel (and return false).
 * This function doesn't wrap lines. Use PCF_FontGetSizeRequest to get needed
 * space for a given string/font.
 *
 * @param str The string to write.
 * @param font The font to use. Opened by PCF_OpenFont.
 * @param color The color of text. Must be in @param destination format (use
 * SDL_MapRGB/SDL_MapRGBA to build a suitable value).
 * @param destination The surface to write to.
 * @param location Where to write on the surface. Can be NULL to write at
 * 0,0. If not NULL, location will be advanced by the width of the string.
 * @return True on success(the whole string has been written), false on error/partial
 * draw. Details of the failure can be retreived with SDL_GetError().
 */
bool PCF_FontWrite(PCF_Font *font, const char *str, Uint32 color, SDL_Surface *destination, SDL_Rect *location)
{
    bool rv;
    int end;
    SDL_Rect cursor = (SDL_Rect){0, 0, 0 ,0};

    /* ATM this function draws each glpyh individually using PCF_FontWriteChar.
     * This could be optimized by drawing on a destination line basis
     * TODO: Bench and try
     * */

    end = strlen(str);
    if(!location)
        location = &cursor;

    rv = true;
    for(int i = 0; i < end; i++){
        if(!PCF_FontWriteChar(font, str[i], color, destination, location))
            rv = false;
    }

    return rv;
}

/**
 * Computes space (pixels width*height) needed to draw a string using a given
 * font. Both @param w and @param h can be NULL depending on which metric you
 * are interested in. The function won't fail if both are NULL, it'll just be
 * useless.
 *
 * @param str The string whose size you need to know.
 * @param font The font you want to use to write that string
 * @param w Pointer to somewhere to place the resulting width. Can be NULL.
 * @param h Pointer to somewhere to place the resulting height. Can be NULL.
 *
 */
void PCF_FontGetSizeRequest(PCF_Font *font, const char *str, Uint32 *w, Uint32 *h)
{
    int len;

    len = strlen(str);
    if(w)
        *w = font->xfont.fontPrivate->metrics->metrics.characterWidth * len;
    if(h)
        *h = font->xfont.fontPrivate->metrics->metrics.ascent + font->xfont.fontPrivate->metrics->metrics.descent;
}

/**
 * Dump a char drawing on stdout using on char per pixel, '#' for lit pixels
 * and '.' for others. Helps with debugging the code. Not public, shoudln't
 * be very useful for regular users.
 *
 * @param font The font to work with.
 * @param c    The ascii code of the char to dump. Can use 'a' instead of 97.
 */
void PCF_FontDumpGlpyh(PCF_Font *font, int c)
{
    int w, h;
    int line_bsize;
    CharInfoRec *glyph;
    BitmapFontRec *bitmapFont;

    bitmapFont  = font->xfont.fontPrivate;
    printf("Number of chars in font: %d\n",  bitmapFont->num_chars);
    if(c >= bitmapFont->num_chars){
        printf("No glyph for char %d\n",c);
        return;
    }
    glyph = &bitmapFont->metrics[c];

    w = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
    h = glyph->metrics.ascent + glyph->metrics.descent;

    printf("\tleft side bearing: %d\n",glyph->metrics.leftSideBearing);
    printf("\tright side bearing: %d\n",glyph->metrics.rightSideBearing);
    printf("\twidth: %d\n",glyph->metrics.characterWidth);
    printf("\tascent: %d\n",glyph->metrics.ascent);
    printf("\tdescent: %d\n",glyph->metrics.descent);
    printf("\tattributes: %d\n",glyph->metrics.attributes);


    /* FontRec.Glyph seems to be the line padding in number of bytes.
     * byte = 1, short = 2, int = 4
     * The pcf format has PCF_GLYPH_PAD_INDEX(format)
     * which indicates if lines are aligned to bytes(1), shorts(2)
     * or ints(4). pcfReadFont compares that to its glyph param and
     * re-pad the data to fit the format.
     * */
    printf("font->glyph is %d\n", font->xfont.glyph);
    line_bsize = ceil(w/(font->xfont.glyph * 8.0))*font->xfont.glyph; /*in bytes*/
    printf("Each glyph line will be %d bytes\n",line_bsize);


    unsigned char *glyph_line;
    unsigned char byte;
    glyph_line = (unsigned char*)glyph->bits;
    for(int i = 0; i < h; i++){
        glyph_line = (unsigned char*)glyph->bits + (i * line_bsize);
        int nbytes = ceil(w/(8.0));
        for(int j = 0; j < nbytes; j++){
            byte = *(unsigned char*)(glyph_line + j);
            for(int k = 0; k < 8; k++){
                printf("%c", (byte & (1 << k)) ? '#': '.');
            }
        }
        printf("\n");
    }
    printf("\n");
}


/**
 * Creates and return a pre-drawn set of characters.
 * The font can be closed afterwards. The return value must be freed by the
 * caller using PCF_FreeStaticFont().
 *
 * Once drawn, static fonts are immutable: You can't add characters on the fly,
 * or change colors. You'll need to create a new static font to do that. The
 * purpose of PCF_StaticFont is to integrate with rendering systems based on
 * fixed bitmap data + coordinates, like SDL_Renderer or OpenGL.
 *
 * @param font  The font to draw with
 * @param color The color of the pre-rendered glyphs
 * @param nsets The number of glyph sets that follows
 * @param ...   Sets of glyphs to include in the cache, as const char*. You can
 * use pre-defined sets such as PCF_ALPHA, PCF_DIGIT, etc. WARNING: The function
 * doesn't check for duplicates characters.
 * @returns a newly allocated PCF_StaticFont or NULL on error. The error will be
 * available with SDL_GetError()
 *
 */
PCF_StaticFont *PCF_FontCreateStaticFont(PCF_Font *font, SDL_Color *color, int nsets, ...)
{
    Uint32 w,h;
    PCF_StaticFont *rv;
    va_list ap;
    const char *tmp;
    char *iter;

    rv = SDL_calloc(1, sizeof(PCF_StaticFont));
    if(!rv){
        SDL_SetError("Couldn't allocate memory for new PCF_StaticFont\n");
        return NULL;
    }

    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, const char*);
        rv->nglyphs += strlen(tmp);
    }
    va_end(ap);

    rv->glyphs = SDL_calloc(rv->nglyphs + 1, sizeof(char));
    iter = rv->glyphs;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, const char*);
        strcpy(iter, tmp);
        iter += strlen(tmp);
    }
    va_end(ap);

    PCF_FontGetSizeRequest(font, rv->glyphs, &w, &h);
    /*Creates a 32bit surface by default which might be overkill*/
    rv->raster = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    rv->nglyphs = strlen(rv->glyphs);
    qsort(rv->glyphs, rv->nglyphs, sizeof(char), (__compar_fn_t) strcmp);

    rv->metrics = font->xfont.fontPrivate->metrics->metrics;
    PCF_FontWrite(
        font, rv->glyphs,
        SDL_MapRGBA(rv->raster->format, color->r, color->g, color->b, color->a),
        rv->raster, NULL
    );

    return rv;
}

/**
 * Frees memory used by a static font. Each static font created using
 * PCF_FontCreateStaticFont should be released using this function.
 *
 * @param self The PCF_StaticFont to free.
 */
void PCF_FreeStaticFont(PCF_StaticFont *self)
{
    SDL_free(self->glyphs);
    SDL_FreeSurface(self->raster);
    SDL_free(self);
}

/**
 * Find the area in self->raster holding a glyph for c. The area is
 * suitable for a SDL_BlitSurface or a SDL_RenderCopy operation using
 * self->raster as a source
 *
 * @param font The static font to search in.
 * @param c    The char to search for.
 * @param glyph Location where to put the coordinates, when found.
 * @return true if @font has a glyph for c, false otherwise.
 */
bool PCF_StaticFontGetCharRect(PCF_StaticFont *font, int c, SDL_Rect *glyph)
{
    int i;

    if(c == ' ')
        return false;

    for(i = 0; i < font->nglyphs; i++){
        if(font->glyphs[i] == c)
            break;
    }

    if(i == font->nglyphs) /*TODO: Use the default glyph*/
        return SDL_SetError("%s: %c: glpyh not found in font %p",__FUNCTION__, c, font) > 0;

    /*The raster is a single glpyh height: All glyphs
     * begin at 0,0 and end at raster->h-1 (height-wise)*/
    glyph->y = 0;
    /* First char(0) goes(x-wise) from 0 to width-1. Next char
     * starts at width, ends at width+width-1, etc.*/
    glyph->x = i * font->metrics.characterWidth;
    glyph->h = font->raster->h;
    glyph->w = font->metrics.characterWidth;

    return true;
}

/**
 * Computes space (pixels width*height) needed to draw a string using a given
 * font. Both @param w and @param h can be NULL depending on which metric you
 * are interested in. The function won't fail if both are NULL, it'll just be
 * useless.
 *
 * @param str The string whose size you need to know.
 * @param font The font you want to use to write that string
 * @param w Pointer to somewhere to place the resulting width. Can be NULL.
 * @param h Pointer to somewhere to place the resulting height. Can be NULL.
 *
 */
void PCF_StaticFontGetSizeRequest(PCF_StaticFont *font, const char *str, Uint32 *w, Uint32 *h)
{
    int len;

    len = strlen(str);
    if(w)
        *w = font->metrics.characterWidth * len;
    if(h)
        *h = font->metrics.ascent + font->metrics.descent;
}

