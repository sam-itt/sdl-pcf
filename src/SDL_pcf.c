#if HAVE_CONFIG_H
# include <config.h>
#endif

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

#define SDLExt_RectLastX(rect) ((rect)->x + (rect)->w - 1)
#define SDLExt_RectLastY(rect) ((rect)->y + (rect)->h - 1)
#define SDLExt_RectMidY(rect) ((rect)->y + roundf(((rect)->h-1)/2.0f))
#define SDLExt_RectMidX(rect) ((rect)->x + roundf(((rect)->w-1)/2.0f))

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})


typedef void (*PixelLighter)(Uint8 *ptr, Uint32 color);

static void filter_dedup(char *base, size_t len);
static bool number_to_ascii(void *value, PCF_NumberType type, int8_t precision, char *buffer, size_t buffer_len);


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
    if(self->xfont.refcnt <= 0){
        pcfUnloadFont(&(self->xfont));
        SDL_free(self);
    }else{
        self->xfont.refcnt--;
    }
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
 * instead of 97. Be sure to cast values to unsigned char when dealing with
 * values over 127.
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
    if(c >= bitmapFont->num_chars || c < 0){
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
        line_y = location->y+i;
        /*clip y both ways*/
        if(line_y > destination->h-1) break;
        if(line_y < 0) continue;
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
 * @param tight If true, the rendering will use ink metrics (tight bounding box) instead
 * of full font metrics. This trims empty space above and below the text.
 * @param destination The surface to write to.
 * @param location Where to write on the surface. Can be NULL to write at
 * 0,0. If not NULL, location will be advanced by the width of the string.
 * @return True on success(the whole string has been written), false on error/partial
 * draw. Details of the failure can be retreived with SDL_GetError().
 */
bool PCF_FontWrite(PCF_Font *font, const char *str, Uint32 color, bool tight, SDL_Surface *destination, SDL_Rect *location)
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

    if(tight){
        Uint32 offset = PCF_FontGetStringTopInkOffset(font, str);
        location->y -= offset;
    }

    rv = true;
    for(int i = 0; i < end; i++){
        if(!PCF_FontWriteChar(font, (unsigned char)str[i], color, destination, location))
            rv = false;
    }

    return rv;
}

/**
 * @brief Same as PCF_FontWrite, expect that the meaning of location x and y
 * start coordinates can be toggled with the subsquent parameters.
 *
 * @param str The string to write.
 * @param font The font to use. Opened by PCF_OpenFont.
 * @param color The color of text. Must be in @param destination format (use
 * SDL_MapRGB/SDL_MapRGBA to build a suitable value).
 * @param tight If true, the rendering will use ink metrics (tight bounding box) instead
 * of full font metrics. This trims empty space above and below the text.
 * @param destination The surface to write to.
 * @param col The column to write relative to, see @p placement
 * @param line The line to write relative to, see @p placement
 * @param placement How to place the text relatively to given @p col and @p
 * line, bitfield of one of RightToCol, CenterOnCol, LeftToCol and one of
 * BelowRow, CenterOnRow, AboveRow.
 * @return True on success(the whole string has been written), false on
 * error/partial draw. Details of the failure can be retreived with
 * SDL_GetError().
 */
bool PCF_FontWriteAt(PCF_Font *font, const char *str, Uint32 color, bool tight, SDL_Surface *destination, Uint32 col, Uint32 row, PCF_TextPlacement placement)
{
    bool rv;
    int end;
    SDL_Rect cursor = (SDL_Rect){0, 0, 0 ,0};
    Uint32 width, height;

    PCF_FontGetSizeRequest(font, str, tight, &width, &height);

    if(placement & RightToCol){
        cursor.x = col;
    }
    else if(placement & CenterOnCol){
        cursor.x = col - roundf((width - 1)/2.0f);
    }else if(placement & LeftToCol){
        cursor.x = col - (width - 1); /*-1: Land on x=col with the last char*/
    }else{
        SDL_SetError("%s: No setting for col placement in %d",
            __FUNCTION__,
            placement
        );
        return false;
    }

    if(placement & BelowRow){
        cursor.y = row;
    }else if(placement & CenterOnRow){
        int ink_ascent = PCF_FontGetStringMaxInkAscent(font, str);
        int empty_top_pix = PCF_FontMetrics(font).ascent - ink_ascent;
        int glyph_middle = empty_top_pix + roundf(ink_ascent/2.0f);

        cursor.y = row - glyph_middle;
    }else if(placement & AboveRow){
        cursor.y = row - height - 1;
    }else{
        SDL_SetError("%s: No setting for line placement in %d",
            __FUNCTION__,
            placement
        );
        return false;
    }

    return PCF_FontWrite(font, str, color, tight, destination, &cursor);
}

/**
 * @brief Writes a number
 *
 * Works just like PCF_FontWrite but avoid having to do a number->string
 * conversion in the client code.
 *
 * @param font @see PCF_FontWrite
 * @param value A pointer to the value to write. Can be either a pointer to:
 * int, unsigned int, float, double.
 * @param type Tell the function the type pointed by @p value, using one of
 * PCF_NumberType enum values: TypeInt, TypeIntUnsigned, TypeFloat,
 * TypeDouble.
 * @param precision For int types, the padding to apply, if any. A padding of 2
 * will make numbers below 10 to print as 01,02, etc. For floating-point types,
 * the number of digits to truncate (NOT round) after. 3.141592 with precision=3
 * is "3.141". If 0, the dot and the decimal part will be ignored, e.g 3.141592
 * will be printed as "3".
 * @param color @see PCF_FontWrite
 * @param tight If true, the rendering will use ink metrics (tight bounding box) instead
 * of full font metrics. This trims empty space above and below the text.
 * @param destination @see PCF_FontWrite
 * @param location @see PCF_FontWrite
 * @return False if the number->string conversion fails (SDL_GetError will give
 * details on the failure) otherwise same behavior as PCF_FontWrite
 */
bool PCF_FontWriteNumber(PCF_Font *font, void *value, PCF_NumberType type, int8_t precision, Uint32 color, bool tight, SDL_Surface *destination, SDL_Rect *location)
{
    char buffer[10]; /*9999999999 or 999999999 with a floating dot*/
    if(!number_to_ascii(value, type, precision, buffer, 10))
        return false;
    return PCF_FontWrite(font, buffer, color, tight, destination, location);
}

/**
 * @brief Writes a number at a given location
 *
 * Works just like PCF_FontWriteNumber combined with PCF_FontWriteAt.
 *
 * @param font @see PCF_FontWriteAt
 * @param value @see PCF_FontWriteNumber
 * @param type @see PCF_FontWriteNumber
 * @param precision @see PCF_FontWriteNumber
 * @param color @see PCF_FontWriteAt
 * @param tight @see PCF_FontWriteAt
 * @param destination @see PCF_FontWriteAt
 * @param location @see PCF_FontWriteAt
 * @return False if the number->string conversion fails (SDL_GetError will give
 * details on the failure) otherwise same behavior as PCF_FontWriteAt
 */
bool PCF_FontWriteNumberAt(PCF_Font *font, void *value, PCF_NumberType type, int8_t precision, Uint32 color, bool tight, SDL_Surface *destination, Uint32 col, Uint32 row, PCF_TextPlacement placement)
{
    char buffer[10]; /*9999999999 or 999999999 with a floating dot*/
    if(!number_to_ascii(value, type, precision, buffer, 10))
        return false;

    return PCF_FontWriteAt(font, buffer, color, tight, destination, col, row, placement);
}


/**
 * Writes a character on a SDL_Renderer, and advance the given location by one
 * char width.
 * If the renderer is too small to fit the char or if the glyph is partly out
 * of the surface (start writing a 18 pixel wide char 2 pixels before the edge)
 * only the pixels that can be written will be drawn, resulting in a partly
 * drawn glyph and the function will return false.
 *
 * Note that there is no color parameter: This is controlled at the
 * SDL_Renderer level with SDL_SetRenderDrawColor.
 *
 * @param c The ASCII code of the char to write. You can of course use 'a'
 * instead of 97.
 * @param font The font to use to write the char. Opened by PCF_OpenFont.
 * @param renderer The renderer that will be used to draw.
 * @param location Location within the renderer. Can be NULL to write at
 * 0,0. If not NULL, location will be advanced by the width.
 * @return True on success(the whole char has been written), false on error/partial
 * draw. Details of the failure can be retreived with SDL_GetError().
 */
bool PCF_FontRenderChar(PCF_Font *font, int c, SDL_Renderer *renderer, SDL_Rect *location)
{
    int w, h;
    int line_bsize;
    CharInfoRec *glyph;
    BitmapFontRec *bitmapFont;
    unsigned char byte;
    unsigned char *glyph_line;
    int nbytes;
    bool rv;
    int y, x;
    int rw, rh;
    rv = true;

    if(c == ' ')
        goto end;

    location = location ? location : &(SDL_Rect){0,0,0,0};

    bitmapFont  = font->xfont.fontPrivate;
    if(c >= bitmapFont->num_chars || c < 0){
        SDL_SetError("%s: no glyph for char %d, falling back to default glyph", __FUNCTION__, c);
        glyph = font->xfont.fontPrivate->pDefault;
        rv = false;
    }else{
        glyph = &bitmapFont->metrics[c];
    }

    /*TODO: Check if can do with FontRec struct members*/
    w = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
    h = glyph->metrics.ascent + glyph->metrics.descent;

    SDL_GetRendererOutputSize(renderer, &rw, &rh);

    /*start after the end of the output size, nothing to draw*/
    if(location->x >= rw || location->y >= rh){
        return false;
    }

    /* FontRec.Glyph is line padding in number of bytes. See pcfReadFont
     * comments for a detailed explaination
     * */
    line_bsize = ceil(w/(font->xfont.glyph * 8.0))*font->xfont.glyph; /*in bytes*/
    nbytes = ceil(w/(8.0)); /*actual glyph width in bytes (w/o padding)*/
    for(int i = 0; i < h; i++){
        glyph_line = (unsigned char*)glyph->bits + (i * line_bsize);
        y = location->y+i;
        /*clip y both ways*/
        if(y < 0) continue;
        if(y > rh-1) break;
        x = location->x;
        for(int j = 0; j < nbytes; j++){
            byte = *(unsigned char*)(glyph_line + j);
            for(int k = 0; k < 8; k++){
                if(byte & (1 << k)){
                    if(x < rw) /*Clip x*/
                        SDL_RenderDrawPoint(renderer, x ,y);
                }
                x++;
            }
        }
    }

end:
    location->x += font->xfont.fontPrivate->metrics->metrics.characterWidth;
    return rv;
}

/**
 * Writes a string on renderer. This function will try it's best to write
 * as many chars as possible: If the renderer is not wide enough to accomodate
 * the whole string, it will stop at the very last pixel (and return false).
 * This function doesn't wrap lines. Use PCF_FontGetSizeRequest to get needed
 * space for a given string/font.
 *
 * @param str The string to write.
 * @param font The font to use. Opened by PCF_OpenFont.
 * @param color The color of text. If not NULL, it will overrede the current
 * renderer's color. If NULL, the current renderer's color will be used.
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param renderer The rendering context to use.
 * @param location Where to write on the renderer. Can be NULL to write at
 * 0,0. If not NULL, location will be advanced by the width of the string.
 * @return True on success(the whole string has been written), false on error/partial
 * draw. Details of the failure can be retreived with SDL_GetError().
 */
bool PCF_FontRender(PCF_Font *font, const char *str, SDL_Color *color, bool tight, SDL_Renderer *renderer, SDL_Rect *location)
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

    if(color)
        SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);

    if(tight){
        Uint32 offset = PCF_FontGetStringTopInkOffset(font, str);
        location->y -= offset;
    }

    rv = true;
    for(int i = 0; i < end; i++){
        if(!PCF_FontRenderChar(font, str[i], renderer, location))
            rv = false;
    }

    return rv;
}

/**
 * Computes the maximun height from baseline for a given string.
 * The would be 8 for the example given in PCF_FontGetStringTopInkOffset.
 *
 * @param font The font you want to use to write that string
 * @param str The string to write.
 *
 * @returns the height
 */
Uint32 PCF_FontGetStringMaxInkAscent(PCF_Font *font, const char *str)
{
    int len;
    BitmapFontRec *bitmapFont;
    int rv;

    len = strlen(str);

    rv = 0;
    bitmapFont  = font->xfont.fontPrivate;
    for(int i = 0; i < len; i++){
        int c = (unsigned char)str[i];
        if(c >= bitmapFont->num_chars) continue;
        rv = MAX(rv, PCF_FontInkMetrics(font)[c].ascent);
    }
    return rv;
}

/**
 * Computes the number of empty pixel rows above the topmost inked row for a
 * given string. For example for the "Pcf" string written in Terminus 12, that
 * would be 2. As you can see the number of empty pixel lines is two:
 *
 * ........ ........ ........
 * ........ ........ ........
 * ####.... ........ ...##...
 * #...#... ........ ..#.....
 * #...#... .###.... .###....
 * #...#... #...#... ..#.....
 * ####.... #....... ..#.....
 * #....... #....... ..#.....
 * #....... #...#... ..#.....
 * #....... .###.... ..#.....
 * ........ ........ ........
 * ........ ........ ........
 *
 * @param font The font you want to use to write that string
 * @param str The string to write.
 *
 * @returns the offset
 */
Uint32 PCF_FontGetStringTopInkOffset(PCF_Font *font, const char *str)
{
    return PCF_FontMetrics(font).ascent - PCF_FontGetStringMaxInkAscent(font, str);
}

/**
 * Computes space (pixels width*height) needed to draw a string using a given
 * font. Both @param w and @param h can be NULL depending on which metric you
 * are interested in. The function won't fail if both are NULL, it'll just be
 * useless.
 *
 * @param str The string whose size you need to know.
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param font The font you want to use to write that string
 * @param narrow Removes unused space at the top of the glyphs, if applicable.
 * @param w Pointer to somewhere to place the resulting width. Can be NULL.
 * @param h Pointer to somewhere to place the resulting height. Can be NULL.
 *
 */
void PCF_FontGetSizeRequest(PCF_Font *font, const char *str, bool tight, Uint32 *w, Uint32 *h)
{
    int len;

    len = strlen(str);
    if(w)
        *w = font->xfont.fontPrivate->metrics->metrics.characterWidth * len;
    if(h){
        if(tight){
            int ascent_max = 0;
            int descent_max = 0;

            BitmapFontRec *bitmapFont;
            bitmapFont  = font->xfont.fontPrivate;
            for(int i = 0; i < len; i++){
                int c = str[i];
                if(c >= bitmapFont->num_chars) continue;
                ascent_max = MAX(ascent_max, PCF_FontInkMetrics(font)[c].ascent);
                descent_max = MAX(descent_max, PCF_FontInkMetrics(font)[c].descent);
            }
            *h = ascent_max + descent_max;
        }else{
            *h = font->xfont.fontPrivate->metrics->metrics.ascent + font->xfont.fontPrivate->metrics->metrics.descent;
        }
    }
}


/**
 * Same PCF_FontGetSizeRequest as but fills an SDL_Rect. Rect x and y
 * get initialized to 0.
 *
 * @param str The string whose size you need to know.
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param font The font you want to use to write that string
 * @param narrow Removes unused space at the top of the glyphs, if applicable.
 * @param rect Pointer to an existing SDL_Rect (cannot be NULL) to fill with
 * the size request.
 */
void PCF_FontGetSizeRequestRect(PCF_Font *font, const char *str, bool tight, SDL_Rect *rect)
{
    Uint32 w, h;

    PCF_FontGetSizeRequest(font, str, tight, &w, &h);

    rect->x = 0;
    rect->y = 0;
    rect->w = w;
    rect->h = h;
}


/**
 * Dump a char drawing on stdout using on char per pixel, '#' for lit pixels
 * and '.' for others. Helps with debugging the code. Not public, shoudln't
 * be very useful for regular users.
 *
 * @param font The font to work with.
 * @param c    The ascii code of the char to dump. Can use 'a' instead of 97.
 */
void PCF_FontDumpGlyph(PCF_Font *font, int c)
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
    printf("Index of char in font: %d\n", c);
    glyph = &bitmapFont->metrics[c];

    w = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
    h = glyph->metrics.ascent + glyph->metrics.descent;

    printf("\tleft side bearing: %d\n",glyph->metrics.leftSideBearing);
    printf("\tright side bearing: %d\n",glyph->metrics.rightSideBearing);
    printf("\twidth: %d\n",glyph->metrics.characterWidth);
    printf("\tascent: %d\n",glyph->metrics.ascent);
    printf("\tdescent: %d\n",glyph->metrics.descent);
    printf("\tattributes: %d\n",glyph->metrics.attributes);

    printf("Ink metrics:\n");
    xCharInfo *ink = &bitmapFont->ink_metrics[c];
    printf("\tleft side bearing: %d\n",ink->leftSideBearing);
    printf("\tright side bearing: %d\n",ink->rightSideBearing);
    printf("\twidth: %d\n",ink->characterWidth);
    printf("\tascent: %d\n",ink->ascent);
    printf("\tdescent: %d\n",ink->descent);
    printf("\tattributes: %d\n",ink->attributes);

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
 * use pre-defined sets such as PCF_ALPHA, PCF_DIGIT, etc. The function will
 * filter out duplicated characters.
 * @returns a newly allocated PCF_StaticFont or NULL on error. The error will be
 * available with SDL_GetError()
 *
 */
PCF_StaticFont *PCF_FontCreateStaticFont(PCF_Font *font, SDL_Color *color, int nsets, ...)
{
    va_list ap;
    char *tmp;
    size_t tlen;
    PCF_StaticFont *rv;

    tlen = 0;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, char*);
        tlen += strlen(tmp);
    }
    va_end(ap);

    va_start(ap, nsets);
    rv = PCF_FontCreateStaticFontVA(font, color, nsets, tlen, ap);
    va_end(ap);

    return rv;
}

/**
 * va_list version of PCF_FontCreateStaticFont. The only difference is that
 * this function needs to be provided with the total(cumulative) length of
 * all the strings that it gets through ap. This is due to the fact that
 * va_list can't be rewinded when passed as an argument to a non-variadic
 * function
 *
 * @param font See PCF_FontCreateStaticFont @param font
 * @param color See PCF_FontCreateStaticFont @param color
 * @param nsets See PCF_FontCreateStaticFont @param nsets
 * @param tlen Total (cumulative) len of the strings passed in.
 * @param ap List of @param nsets char*
 * @return See PCF_FontCreateStaticFont @return.
 */
PCF_StaticFont *PCF_FontCreateStaticFontVA(PCF_Font *font, SDL_Color *color, int nsets, size_t tlen, va_list ap)
{
    Uint32 w,h;
    PCF_StaticFont *rv;
    const char *tmp;
    char *iter;
    Uint32 col;

    rv = SDL_calloc(1, sizeof(PCF_StaticFont));
    if(!rv){
        SDL_SetError("Couldn't allocate memory for new PCF_StaticFont\n");
        return NULL;
    }

    rv->nglyphs = tlen;
    rv->glyphs = SDL_calloc(rv->nglyphs + 1, sizeof(char));
    rv->glyph_heights = SDL_calloc(rv->nglyphs, sizeof(InkHeight));

    iter = rv->glyphs;
    for(int i = 0; i < nsets; i++){
        tmp = va_arg(ap, const char*);
        strcpy(iter, tmp);
        iter += strlen(tmp);
    }

    PCF_FontGetSizeRequest(font, rv->glyphs, false, &w, &h);
    /*The static font will hold an implicit default glyph at it's very end*/
    w += font->xfont.fontPrivate->pDefault->metrics.characterWidth;
    /*Creates a 32bit surface by default which might be overkill*/
    rv->raster = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    rv->text_color = *color;
    col =  SDL_MapRGBA(rv->raster->format, color->r, color->g, color->b, color->a);
    rv->nglyphs = strlen(rv->glyphs);
    qsort(rv->glyphs, rv->nglyphs, sizeof(char), (__compar_fn_t) strcmp);
    filter_dedup(rv->glyphs, rv->nglyphs);

    rv->metrics = font->xfont.fontPrivate->metrics->metrics;
    for(int i = 0; i < rv->nglyphs; i++){
        int c = (unsigned char)rv->glyphs[i];
        rv->glyph_heights[i] = (InkHeight){
            .ascent = PCF_FontInkMetrics(font)[c].ascent,
            .descent = PCF_FontInkMetrics(font)[c].descent
        };
    }

    PCF_FontWrite(
        font, rv->glyphs,
        col, false,
        rv->raster, NULL
    );
    PCF_FontWriteChar(font, -1, col, rv->raster, &(SDL_Rect){
        .y = 0,
        .x = rv->raster->w - font->xfont.fontPrivate->pDefault->metrics.characterWidth, /*double checked: ok*/
        .w = font->xfont.fontPrivate->pDefault->metrics.characterWidth,
        .h = font->xfont.fontPrivate->pDefault->metrics.ascent + font->xfont.fontPrivate->pDefault->metrics.descent
    });

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
    if(self->refcnt <= 0){
        SDL_free(self->glyphs);
        SDL_FreeSurface(self->raster);
#if USE_SDL2_TEXTURE
        SDL_DestroyTexture(self->texture);
#elif USE_SGPU_TEXTURE
        GPU_FreeImage(self->texture);
#endif
        SDL_free(self);
    }else{
        self->refcnt--;
    }
}

/**
 * Finds the index of a given char. Internal use only.
 *
 * @param font The static font to search in.
 * @param c    The char to search for.
 *
 * @return the index or negative if not found
 */
static inline int PCF_StaticFontGetGlyphIndex(PCF_StaticFont *font, int c)
{
    char *pos = memchr(font->glyphs, c, font->nglyphs);
    return pos - font->glyphs;
}

/**
 * Find the area in self->raster holding a glyph for c. The area is
 * suitable for a SDL_BlitSurface or a SDL_RenderCopy operation using
 * self->raster as a source
 *
 * @param font The static font to search in.
 * @param c    The char to search for.
 * @param glyph Location where to put the coordinates, when found.
 * @return 0 for whitespace (@glpyh untouched), non-zero if @param font
 * has something printable for @param c: 1 if the char as been found,
 * -1 otherwise. When returning -1, glpyh has been set to the default glyph.
 */
int PCF_StaticFontGetCharRect(PCF_StaticFont *font, int c, SDL_Rect *glyph)
{
    int i;
    int rv;

    if(c == ' ')
        return 0;

    rv = 1;
    i = PCF_StaticFontGetGlyphIndex(font, c);
    if(i < 0){
        rv = SDL_SetError("%s: %c: glpyh not found in font %p",__FUNCTION__, c, font);
        /*make i point to the implicit default char*/
        i = font->nglyphs;
    }

    /*The raster is a single glpyh height: All glyphs
     * begin at 0,0 and end at raster->h-1 (height-wise)*/
    glyph->y = 0;
    /* First char(0) goes(x-wise) from 0 to width-1. Next char
     * starts at width, ends at width+width-1, etc.*/
    glyph->x = i * font->metrics.characterWidth;
    glyph->h = font->raster->h;
    glyph->w = font->metrics.characterWidth;

    return rv;
}

/**
 * Computes space (pixels width*height) needed to draw a string using a given
 * font. Both @param w and @param h can be NULL depending on which metric you
 * are interested in. The function won't fail if both are NULL, it'll just be
 * useless.
 *
 * @param str The string whose size you need to know.
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param font The font you want to use to write that string
 * @param w Pointer to somewhere to place the resulting width. Can be NULL.
 * @param h Pointer to somewhere to place the resulting height. Can be NULL.
 *
 */
void PCF_StaticFontGetSizeRequest(PCF_StaticFont *font, const char *str, bool tight, Uint32 *w, Uint32 *h)
{
    int len;

    len = strlen(str);
    if(w)
        *w = font->metrics.characterWidth * len;
    if(h){
        if(tight){
            int ascent_max = 0;
            int descent_max = 0;

            BitmapFontRec *bitmapFont;
            for(int i = 0; i < len; i++){
                int glyph_index = PCF_StaticFontGetGlyphIndex(font, str[i]);
                if(glyph_index < 0) continue;
                ascent_max = MAX(ascent_max, font->glyph_heights[glyph_index].ascent);
                descent_max = MAX(descent_max, font->glyph_heights[glyph_index].descent);
            }
            *h = ascent_max + descent_max;
        }else{
            *h = font->metrics.ascent + font->metrics.descent;
        }
    }


}

/**
 * Same PCF_StaticFontGetSizeRequest as but fills an SDL_Rect. Rect x and y
 * get initialized to 0.
 *
 * @param str The string whose size you need to know.
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param font The font you want to use to write that string
 * @param rect Pointer to an existing SDL_Rect (cannot be NULL) to fill with
 * the size request.
 */
void PCF_StaticFontGetSizeRequestRect(PCF_StaticFont *font, const char *str, bool tight, SDL_Rect *rect)
{
    Uint32 w, h;

    PCF_StaticFontGetSizeRequest(font, str, tight, &w, &h);

    rect->x = 0;
    rect->y = 0;
    rect->w = w;
    rect->h = h;
}

/**
 * @see PCF_FontGetStringMaxInkAscent
 *
 * @param font The font you want to use to write that string
 * @param str The string to write.
 *
 * @returns the height
 */
Uint32 PCF_StaticFontGetStringMaxInkAscent(PCF_StaticFont *font, const char *str)
{
    int len;
    int rv;

    len = strlen(str);

    rv = 0;
    for(int i = 0; i < len; i++){
        int glyph_index = PCF_StaticFontGetGlyphIndex(font, (unsigned char)str[i]);
        if(glyph_index < 0) continue;
        rv = MAX(rv, font->glyph_heights[glyph_index].ascent);
    }
    return rv;
}

/**
 * See PCF_FontGetStringTopInkOffset.
 *
 * @param font The font you want to use to write that string
 * @param str The string to write.
 *
 * @returns the offset
 */
Uint32 PCF_StaticFontGetStringTopInkOffset(PCF_StaticFont *font, const char *str)
{
    return font->metrics.ascent - PCF_StaticFontGetStringMaxInkAscent(font, str);
}


/**
 * @brief Generate a set of areas to blit from/to in order to write @p str using @p font
 *
 * This function will generate up to @p npatches char patches at location @p patches. Each
 * patch has two top-left corners of font widthxheight rectangles one being the source (blit from
 * @p font->raster or @p font->texture) and the other destination (where to blit the char to have
 * a continuous one-line string)
 *
 * @param font a PCF_StaticFont
 * @param str the string to write
 * @param tight If true, the rendering will use ink metrics (tight bounding box) instead
 * of full font metrics. This trims empty space above and below the text.
 * @param len the length of the string to write, -1 to compute it.
 * @p location of top-left start position the cursor or NULL to start at 0,0. If not NULL,
 * this function will advance the location at the end of the string and size it to the maximum
 * area used by one glyph.
 * @p npatches size of @p patches. Must be the same size of str (spaces won't output a patch)
 * @p patches pointer to a large enough array of PCF_StaticFontPatches
 * @return number of patches actually written
 */
size_t PCF_StaticFontPreWriteString(PCF_StaticFont *font, int len, const char *str, bool tight,
                                    SDL_Rect *location, size_t npatches, PCF_StaticFontPatch *patches)
{
    size_t rv;
    SDL_Rect glyph;
    SDL_Rect *cursor;
    Uint32 offset;

    offset = tight ? PCF_StaticFontGetStringTopInkOffset(font, str) : 0;

    cursor = location ? location : &(SDL_Rect){
        .x = 0,
        .y = 0
    };
    /* We don't use these values in the function but set them for the calling code
     * to know what we have done
     */
    cursor->w = PCF_StaticFontCharWidth(font);
    cursor->h = PCF_StaticFontCharHeight(font) - offset;

    if(len < 0)
        len = strlen(str);
    rv = 0;
    for(int i = 0; i < len && rv < npatches; i++){
        if( PCF_StaticFontGetCharRect(font, str[i], &glyph) != 0){ /*0 means white space*/
            patches[rv].src = (SDL_Rect){
                glyph.x,
                glyph.y + offset,
                glyph.w,
                glyph.h - offset
            };
            patches[rv].dst = (SDL_Point){cursor->x, cursor->y};
            rv++;
        }
        cursor->x += PCF_StaticFontCharWidth(font);
    }

    return rv;
}

/**
 * @brief Generate a set of areas to blit from/to in order to write @p str
 * using @p font, with an optional offset.
 *
 * This function will generate up to @p npatches char patches at location
 * @p patches. Each patch has a source SDL_Rect area (blit from  @p font->raster
 * or @p font->texture) and an SDL_Point that represent the destination origin
 * where to blit the char to have a continuous one-line string) width and height
 * are already given in the src member.
 *
 * @param font a PCF_StaticFont
 * @param str the string to write
 * @param tight If true, the rendering will use ink metrics (tight bounding box)
 * instead of full font metrics. This trims empty space above and below the text.
 * @param len the length of the string to write, -1 to compute it.
 * @param location of top-left start position the cursor or NULL to start at 0,0.
 * @param xoffset Offset in pixels relative to location. For example a -10 offset
 * means that the string will be written as if it was moved to the left by 10 pixels.
 * With a 12px wide font, only the last 2 pixels of the first glyph would be visible.
 * @param npatches size of @p patches. Must of be the same size as str
 * (spaces will output a patch with src.x and src.y both set to -1)
 * @param patches pointer to a large enough array of PCF_StaticFontPartPatches
 * @return number of patches actually written
 */
size_t PCF_StaticFontPreWriteStringOffset(PCF_StaticFont *font,
                                          int len, const char *str,
                                          bool tight, SDL_Rect *location,
                                          int xoffset, int yoffset,
                                          size_t npatches, PCF_StaticFontPatch *patches)
{
    size_t rv;
    SDL_Rect glyph;
    SDL_Rect cursor;
    Uint32 offset;

    offset = tight ? PCF_StaticFontGetStringTopInkOffset(font, str) : 0;

    cursor = (SDL_Rect){
        .x = (location ? location->x : 0) + xoffset,
        .y = (location ? location->y : 0) + yoffset,
        .w = PCF_StaticFontCharWidth(font),
        .h = PCF_StaticFontCharHeight(font)
    };
    cursor.h -= offset;

    if(len < 0)
        len = strlen(str);
    rv = 0;

    int skip = xoffset < 0 ? abs(xoffset)/PCF_StaticFontCharWidth(font) : 0;
    cursor.x += skip * PCF_StaticFontCharWidth(font);
    for(int i = skip; i < len && rv < npatches; i++){
        /*TODO SDLExt_RectAbove/Below/Before/After*/
        if(   cursor.x > SDLExt_RectLastX(location)
           || cursor.y > SDLExt_RectLastY(location))
            break;

        /*If we are here, there is an intersection*/
        if(PCF_StaticFontGetCharRect(font, str[i], &glyph) != 0){ /*0 means white space*/
            patches[rv].src = (SDL_Rect){
                glyph.x,
                glyph.y + offset,
                glyph.w,
                glyph.h - offset
            };
        }else{
            /* We do this to have a patch for spaces that can be used to
             * show the cursor in SoFIS (see text_box_render() in text-box.c).
             * in that case we need the dst part to know where on the screen
             * the space should vs just skipping the patch altogether like in
             * the non-offset version.
             * TODO: Document and maybe make that configurable in both functions
             * TODO: Consider having an empty glyph (space) at the beginining of
             * the string that would simplify the code. At the expense of a couple
             * of bytes (code is likeyly larger anyways).
             * */
            patches[rv].src = (SDL_Rect){
                -1,
                -1,
                PCF_StaticFontCharWidth(font),
                PCF_StaticFontCharHeight(font) - offset
            };
        }
        SDL_Rect intersect;
        SDL_IntersectRect(&cursor, location, &intersect);

        patches[rv].dst = (SDL_Point){intersect.x, intersect.y};
        if(intersect.w < patches[rv].src.w){
            if(cursor.x < location->x){
                int delta = patches[rv].src.w - intersect.w;
                /*A negative start point (x,y) means a space. keep it*/
                if(patches[rv].src.x >= 0)
                    patches[rv].src.x += delta;
                patches[rv].src.w -= delta;
            }else{
                patches[rv].src.w = intersect.w;
            }
        }
        /* TODO: Compute only once, line height won't change
         * as we go xward*/
        if(intersect.h < patches[rv].src.h){
            if(cursor.y < location->y){
                int delta = patches[rv].src.h - intersect.h;
                /*A negative start point (x,y) means a space. keep it*/
                if(patches[rv].src.y >= 0)
                    patches[rv].src.y += delta;
                patches[rv].src.h -= delta;
            }else{
                patches[rv].src.h = intersect.h;
            }
        }

        cursor.x += PCF_StaticFontCharWidth(font);
        rv++;
    }

    return rv;
}


/**
 * Check whether @param font can be used to write all chars given in
 * @param sequence in color @param color.
 *
 * @param color The color you want to write in. If NULL the function
 * will assume that the desired color is the same as the font native
 * color, i.e NULL is equivalent to &font->color.
 * @param sequence All the chars you may want to use
 * @return true if all chars of @param sequence can be written in
 * @param color, false otherwise.
 */
bool PCF_StaticFontCanWrite(PCF_StaticFont *font, SDL_Color *color, const char *sequence)
{
    int len;

    if(color && memcmp(&font->text_color, color, sizeof(SDL_Color)) != 0){
        return false;
    }

    len = strlen(sequence);
    if(len == font->nglyphs)
        return strcmp(sequence, font->glyphs) == 0;

    for(int i = 0; i < len; i++){
        if(!strchr(font->glyphs, sequence[i]))
            return false;
    }

    return true;
}

#if USE_SDL2_TEXTURE
void PCF_StaticFontCreateTexture(PCF_StaticFont *font, SDL_Renderer *renderer)
{
    font->texture = SDL_CreateTextureFromSurface(renderer, font->raster);
    /*TODO: Check if it's appropriate to free the surface*/
}
#elif USE_SGPU_TEXTURE
void PCF_StaticFontCreateTexture(PCF_StaticFont *font)
{
    font->texture = GPU_CopyImageFromSurface(font->raster);
    /*TODO: Check if it's appropriate to free the surface*/
}
#endif



/*
 * Remove duplicates characters from base. base must be sorted
 * so that duplicates follow each other (i.e. use qsort() beforehand).
 *
 * @param base The string to filter
 * @param len THe string len, -1 to have the function compute it.
 */
static void filter_dedup(char *base, size_t len)
{
    len = len > -1 ? len : strlen(base);

    for(int i = 1; i < len; i++){
        if(base[i] == base[i-1]){
            int next; /*Next different char idx*/
            for(next = i; next < len && base[next] == base[i]; next++);
            /* Index doesn't go OOB, last iteration will
             * access (and move back) the final '\0' */
            for(int j = next; j < len+1; j++){
                base[i+(j-next)] = base[j];
            }
        }
    }
}


/*
 * Convert a number(int/float/etc) into a string buffer
 * for printing
 *
 */
static bool number_to_ascii(void *value, PCF_NumberType type, int8_t precision, char *buffer, size_t buffer_len)
{
    switch(type){
        case TypeInt:
            if(precision)
                snprintf(buffer, buffer_len, "%.*d", precision, *(int *)value);
            else
                snprintf(buffer, buffer_len, "%d", *(int *)value);
            break;
        case TypeIntUnsigned:
            if(precision)
                snprintf(buffer, buffer_len, "%.*d", precision, *(unsigned int *)value);
            else
                snprintf(buffer, buffer_len, "%d", *(unsigned int *)value);
            break;
        case TypeFloat:
            snprintf(buffer, buffer_len, "%.*f", precision >= 0 ? precision : 6, *(float *)value);
            break;
        case TypeDouble:
            snprintf(buffer, buffer_len, "%.*f", precision >= 0 ? precision : 6, *(double *)value);
            break;
        default:
            SDL_SetError("%s: Unknown value type: %d",
                __FUNCTION__,
                type
            );
            return false;
    }
    return true;
}
