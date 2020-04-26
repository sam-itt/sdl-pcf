#ifndef PCFREAD_H
#define PCFREAD_H

#include "SDL_stdinc.h"


typedef struct _FontProp {
    long        name;
    long        value;      /* assumes ATOM is not larger than INT32 */
}           FontPropRec;

typedef struct _FontProp *FontPropPtr;


typedef struct {
    Sint16 leftSideBearing,
           rightSideBearing,
           characterWidth,
           ascent,
           descent;
    Uint16 attributes;
} xCharInfo;

typedef struct _FontInfo {
    unsigned short firstCol;
    unsigned short lastCol;
    unsigned short firstRow;
    unsigned short lastRow;
    unsigned short defaultCh;
    unsigned int noOverlap:1;
    unsigned int terminalFont:1;
    unsigned int constantMetrics:1;
    unsigned int constantWidth:1;
    unsigned int inkInside:1;
    unsigned int inkMetrics:1;
    unsigned int allExist:1;
    unsigned int drawDirection:2;
    unsigned int cachable:1;
    unsigned int anamorphic:1;
    short       maxOverlap;
    short       pad;
    xCharInfo   maxbounds;
    xCharInfo   minbounds;
    xCharInfo   ink_maxbounds;
    xCharInfo   ink_minbounds;
    short       fontAscent;
    short       fontDescent;
    int         nprops;
    FontPropPtr props;
    char       *isStringProp;
}FontInfoRec;
typedef struct _FontInfo *FontInfoPtr;

typedef struct _Font {
    int         refcnt;
    FontInfoRec info;
    char        bit;
    char        byte;
    char        glyph;
    char        scan;
#if 0
    fsBitmapFormat format;
    int         (*get_glyphs) (FontPtr         /* font */,
                   unsigned long   /* count */,
                   unsigned char * /* chars */,
                   FontEncoding    /* encoding */,
                   unsigned long * /* count */,
                   CharInfoPtr *   /* glyphs */);
    int         (*get_metrics) (FontPtr         /* font */,
                unsigned long   /* count */,
                unsigned char * /* chars */,
                FontEncoding    /* encoding */,
                unsigned long * /* count */,
                xCharInfo **    /* glyphs */);
    void        (*unload_font) (FontPtr         /* font */);
    void        (*unload_glyphs) (FontPtr         /* font */);
//    FontPathElementPtr fpe;
#endif
    void        *svrPrivate;
    void        *fontPrivate;
    void        *fpePrivate;
    int     maxPrivate;
    void        **devPrivates;
}FontRec;
typedef struct _Font *FontPtr;



typedef struct _CharInfo {
    xCharInfo   metrics;    /* info preformatted for Queries */
    char       *bits;       /* pointer to glyph image */
}           CharInfoRec;
typedef struct _CharInfo *CharInfoPtr;

typedef struct _BitmapFont {
    unsigned    version_num;
    int         num_chars;
    int         num_tables;
    CharInfoPtr metrics;    /* font metrics, including glyph pointers */
    xCharInfo  *ink_metrics;    /* ink metrics */
    char       *bitmaps;    /* base of bitmaps, useful only to free */
    CharInfoPtr **encoding; /* array of arrays of char info pointers */
    CharInfoPtr pDefault;   /* default character */
//    BitmapExtraPtr bitmapExtra; /* stuff not used by X server */
}           BitmapFontRec, *BitmapFontPtr;


/* number of encoding entries in one segment */
#define BITMAP_FONT_SEGMENT_SIZE 128

#define ACCESSENCODING(enc,i) \
(enc[(i)/BITMAP_FONT_SEGMENT_SIZE]?\
(enc[(i)/BITMAP_FONT_SEGMENT_SIZE][(i)%BITMAP_FONT_SEGMENT_SIZE]):\
0)
#define ACCESSENCODINGL(enc,i) \
(enc[(i)/BITMAP_FONT_SEGMENT_SIZE][(i)%BITMAP_FONT_SEGMENT_SIZE])

#define SEGMENT_MAJOR(n) ((n)/BITMAP_FONT_SEGMENT_SIZE)
#define SEGMENT_MINOR(n) ((n)%BITMAP_FONT_SEGMENT_SIZE)

#define NUM_SEGMENTS(n) \
  (((n)+BITMAP_FONT_SEGMENT_SIZE-1)/BITMAP_FONT_SEGMENT_SIZE)


#endif /* PCFREAD_H */
