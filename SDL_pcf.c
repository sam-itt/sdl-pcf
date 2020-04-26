#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pcf.h"
#include "pcfread.h"
#include "SDL_pcf.h"
#include "SDL_GzRW.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"

typedef void (*PixelLighter)(Uint8 *ptr, Uint32 color);



SDL_PcfFont *SDL_PcfFontInit(SDL_PcfFont *self, const char *filename)
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

SDL_PcfFont *SDL_PcfOpenFont(const char *filename)
{
    SDL_PcfFont *rv;

    rv = SDL_calloc(1, sizeof(SDL_PcfFont));
    if(!SDL_PcfFontInit(rv, filename)){
        SDL_free(rv);
        return NULL;
    }
    return rv;
}

void SDL_PcfCloseFont(SDL_PcfFont *self)
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


bool SDL_PcfWriteChar(int c, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location)
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

    rv = true;
    bitmapFont  = font->xfont.fontPrivate;
    if(c >= bitmapFont->num_chars){
        SDL_SetError("%s: no glyph for char %d, falling back to default glyph", __FUNCTION__, c);
        glyph = font->xfont.fontPrivate->pDefault;
        rv = false;
    }else{
        glyph = &bitmapFont->metrics[c];
    }


    /*Check if can do with font members*/
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
    location->x += font->xfont.fontPrivate->metrics->metrics.characterWidth;
    return rv;
}


bool SDL_PcfWrite(const char *str, SDL_PcfFont *font, Uint32 color, SDL_Surface *destination, SDL_Rect *location)
{
    bool rv;
    int end;
    SDL_Rect cursor = (SDL_Rect){0, 0, 0 ,0};

    end = strlen(str);
    if(!location)
        location = &cursor;

    rv = true;
    for(int i = 0; i < end; i++){
        if(!SDL_PcfWriteChar(str[i], font, color, destination, location))
            rv = false;
        cursor.x += font->xfont.fontPrivate->metrics->metrics.characterWidth;
    }

    return rv;
}

void SDL_PcfGetSizeRequest(const char *str, SDL_PcfFont *font, Uint32 *w, Uint32 *h)
{
    int len;

    len = strlen(str);
    if(w)
        *w = font->xfont.fontPrivate->metrics->metrics.characterWidth * len;
    if(h)
        *h = font->xfont.fontPrivate->metrics->metrics.ascent + font->xfont.fontPrivate->metrics->metrics.descent;
}

/*@param c: ascii code for the char to dump*/
void SDL_PcfDumpGlpyh(SDL_PcfFont *font, int c)
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

