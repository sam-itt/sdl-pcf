/*

Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/*
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * Adapted for SDL_bdf by Samuel Cuella, 2020
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pcf.h"
#include "pcfread.h"
#include "SDL_GzRW.h"
#include "utilbitmap.h"

#define GLYPHPADOPTIONS 4
#define NO_SUCH_CHAR	-1

#ifndef MAX
#define   MAX(a,b)    (((a)>(b)) ? a : b)
#endif

/* Read PCF font files */

static int  position;


#define IS_EOF(file) (SDL_GzRWEof((file)) == 1)

static Uint32 pcfGetLSB32(SDL_RWops *file)
{
    position += 4;
    return SDL_ReadLE32(file);
}

static Uint32
pcfGetINT32(SDL_RWops *file, Uint32 format)
{
    position += 4;
    if (PCF_BYTE_ORDER(format) == MSBFirst)  /*Data is Big endian*/
        return SDL_ReadBE32(file);
    return SDL_ReadLE32(file);
}

static Uint16
pcfGetINT16(SDL_RWops *file, Uint32 format)
{
    position += 2;
    if (PCF_BYTE_ORDER(format) == MSBFirst)
        return SDL_ReadBE16(file);
    return SDL_ReadLE16(file);
}

static Uint8 pcfGetINT8(SDL_RWops *file, Uint32 format)
{
    position++;
    return SDL_ReadU8(file);
}

static      PCFTablePtr
pcfReadTOC(SDL_RWops *file, int *countp)
{
    Uint32      version;
    PCFTablePtr tables;
    int         count;
    int         i;

    position = 0;
    version = pcfGetLSB32(file);
    if (version != PCF_FILE_VERSION)
	return (PCFTablePtr) NULL;
    count = pcfGetLSB32(file);
    if (IS_EOF(file)) return (PCFTablePtr) NULL;
    if (count < 0 || count > INT32_MAX / sizeof(PCFTableRec)) {
	SDL_SetError("pcfReadTOC(): invalid file format");
	return NULL;
    }
    tables = SDL_calloc(count, sizeof(PCFTableRec));
    if (!tables) {
	SDL_SetError("pcfReadTOC(): Couldn't allocate tables (%d*%d)",
		 count, (int) sizeof(PCFTableRec));
	return (PCFTablePtr) NULL;
    }
    for (i = 0; i < count; i++) {
	tables[i].type = pcfGetLSB32(file);
	tables[i].format = pcfGetLSB32(file);
	tables[i].size = pcfGetLSB32(file);
	tables[i].offset = pcfGetLSB32(file);
	if (IS_EOF(file)) goto Bail;
    }

    *countp = count;
    return tables;

 Bail:
    SDL_free(tables);
    return (PCFTablePtr) NULL;
}

/*
 * PCF supports two formats for metrics, both the regular
 * jumbo size, and 'lite' metrics, which are useful
 * for most fonts which have even vaguely reasonable
 * metrics
 */

static bool
pcfGetMetric(SDL_RWops *file, Uint32 format, xCharInfo *metric)
{
    metric->leftSideBearing = pcfGetINT16(file, format);
    metric->rightSideBearing = pcfGetINT16(file, format);
    metric->characterWidth = pcfGetINT16(file, format);
    metric->ascent = pcfGetINT16(file, format);
    metric->descent = pcfGetINT16(file, format);
    metric->attributes = pcfGetINT16(file, format);
    if (IS_EOF(file)) return false;

    return true;
}

static bool
pcfGetCompressedMetric(SDL_RWops *file, Uint32 format, xCharInfo *metric)
{
    metric->leftSideBearing = pcfGetINT8(file, format) - 0x80;
    metric->rightSideBearing = pcfGetINT8(file, format) - 0x80;
    metric->characterWidth = pcfGetINT8(file, format) - 0x80;
    metric->ascent = pcfGetINT8(file, format) - 0x80;
    metric->descent = pcfGetINT8(file, format) - 0x80;
    metric->attributes = 0;
    if (IS_EOF(file)) return false;

    return true;
}

/*
 * Position the file to the begining of the specified table
 * in the font file
 */
static bool
pcfSeekToType(SDL_RWops *file, PCFTablePtr tables, int ntables,
	      Uint32 type, Uint32 *formatp, Uint32 *sizep)
{
    int         i;

    for (i = 0; i < ntables; i++)
	if (tables[i].type == type) {
	    if (position > tables[i].offset)
		return false;
        if(!SDL_RWseek(file, tables[i].offset - position, RW_SEEK_CUR))
		return false;
	    position = tables[i].offset;
	    *sizep = tables[i].size;
	    *formatp = tables[i].format;
	    return true;
	}
    return false;
}

static bool
pcfHasType (PCFTablePtr tables, int ntables, Uint32 type)
{
    int         i;

    for (i = 0; i < ntables; i++)
	if (tables[i].type == type)
	    return true;
    return false;
}

/*
 * pcfGetProperties
 *
 * Reads the font properties from the font file, filling in the FontInfo rec
 * supplied.  Used by by both ReadFont and ReadFontInfo routines.
 */

static bool
pcfGetProperties(FontInfoPtr pFontInfo, SDL_RWops *file,
		 PCFTablePtr tables, int ntables)
{
    FontPropPtr props = 0;
    int         nprops;
    char       *isStringProp = 0;
    Uint32      format;
    int         i;
    Uint32      size;
    int         string_size;
    char       *strings;

    /* font properties */

    if (!pcfSeekToType(file, tables, ntables, PCF_PROPERTIES, &format, &size))
	goto Bail;
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
	goto Bail;
    nprops = pcfGetINT32(file, format);
    if (nprops <= 0 || nprops > INT32_MAX / sizeof(FontPropRec)) {
	SDL_SetError("pcfGetProperties(): invalid nprops value (%d)", nprops);
	goto Bail;
    }
    if (IS_EOF(file)) goto Bail;
    props = SDL_calloc(nprops, sizeof(FontPropRec));
    if (!props) {
	SDL_SetError("pcfGetProperties(): Couldn't allocate props (%d*%d)",
	       nprops, (int) sizeof(FontPropRec));
	goto Bail;
    }
    isStringProp = SDL_calloc(nprops, sizeof(char));
    if (!isStringProp) {
	SDL_SetError("pcfGetProperties(): Couldn't allocate isStringProp (%d*%d)",
	       nprops, (int) sizeof(char));
	goto Bail;
    }
    for (i = 0; i < nprops; i++) {
	props[i].name = pcfGetINT32(file, format);
	isStringProp[i] = pcfGetINT8(file, format);
	props[i].value = pcfGetINT32(file, format);
	if (props[i].name < 0
	    || (isStringProp[i] != 0 && isStringProp[i] != 1)
	    || (isStringProp[i] && props[i].value < 0)) {
	    SDL_SetError("pcfGetProperties(): invalid file format %ld %d %ld",
		     props[i].name, isStringProp[i], props[i].value);
	    goto Bail;
	}
	if (IS_EOF(file)) goto Bail;
    }
    /* pad the property array */
    /*
     * clever here - nprops is the same as the number of odd-units read, as
     * only isStringProp are odd length
     */
    if (nprops & 3)
    {
	i = 4 - (nprops & 3);
	(void)SDL_RWseek(file, i, RW_SEEK_CUR);
	position += i;
    }
    if (IS_EOF(file)) goto Bail;
    string_size = pcfGetINT32(file, format);
    if (string_size < 0) goto Bail;
    if (IS_EOF(file)) goto Bail;
    strings = SDL_malloc(string_size);
    if (!strings) {
      SDL_SetError("pcfGetProperties(): Couldn't allocate strings (%d)", string_size);
	goto Bail;
    }
    SDL_RWread(file, strings, string_size, 1);
    if (IS_EOF(file)) goto Bail;
    position += string_size;
    for (i = 0; i < nprops; i++) {
	if (props[i].name >= string_size) {
	    SDL_SetError("pcfGetProperties(): String starts out of bounds (%ld/%d)", props[i].name, string_size);
	    goto Bail;
	}
/*	props[i].name = MakeAtom(strings + props[i].name,
				 strnlen(strings + props[i].name, string_size - props[i].name), true);*/
    props[i].name = 0L;

	if (isStringProp[i]) {
	    if (props[i].value >= string_size) {
		SDL_SetError("pcfGetProperties(): String starts out of bounds (%ld/%d)", props[i].value, string_size);
		goto Bail;
	    }
        props[i].value = 0L;
/*	    props[i].value = MakeAtom(strings + props[i].value,
				      strnlen(strings + props[i].value, string_size - props[i].value), true);*/
	}
    }
    free(strings);
    pFontInfo->isStringProp = isStringProp;
    pFontInfo->props = props;
    pFontInfo->nprops = nprops;
    return true;
Bail:
    free(isStringProp);
    free(props);
    return false;
}


/*
 * pcfReadAccel
 *
 * Fill in the accelerator information from the font file; used
 * to read both BDF_ACCELERATORS and old style ACCELERATORS
 */

static bool
pcfGetAccel(FontInfoPtr pFontInfo, SDL_RWops *file,
	    PCFTablePtr tables, int ntables, Uint32 type)
{
    Uint32      format;
    Uint32	size;

    if (!pcfSeekToType(file, tables, ntables, type, &format, &size) ||
	IS_EOF(file))
	goto Bail;
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT) &&
	!PCF_FORMAT_MATCH(format, PCF_ACCEL_W_INKBOUNDS))
    {
	goto Bail;
    }
    pFontInfo->noOverlap = pcfGetINT8(file, format);
    pFontInfo->constantMetrics = pcfGetINT8(file, format);
    pFontInfo->terminalFont = pcfGetINT8(file, format);
    pFontInfo->constantWidth = pcfGetINT8(file, format);
    pFontInfo->inkInside = pcfGetINT8(file, format);
    pFontInfo->inkMetrics = pcfGetINT8(file, format);
    pFontInfo->drawDirection = pcfGetINT8(file, format);
    pFontInfo->anamorphic = false;
    pFontInfo->cachable = true;
     /* natural alignment */ pcfGetINT8(file, format);
    pFontInfo->fontAscent = pcfGetINT32(file, format);
    pFontInfo->fontDescent = pcfGetINT32(file, format);
    pFontInfo->maxOverlap = pcfGetINT32(file, format);
    if (IS_EOF(file)) goto Bail;
    if (!pcfGetMetric(file, format, &pFontInfo->minbounds))
	goto Bail;
    if (!pcfGetMetric(file, format, &pFontInfo->maxbounds))
	goto Bail;
    if (PCF_FORMAT_MATCH(format, PCF_ACCEL_W_INKBOUNDS)) {
	if (!pcfGetMetric(file, format, &pFontInfo->ink_minbounds))
	    goto Bail;
	if (!pcfGetMetric(file, format, &pFontInfo->ink_maxbounds))
	    goto Bail;
    } else {
	pFontInfo->ink_minbounds = pFontInfo->minbounds;
	pFontInfo->ink_maxbounds = pFontInfo->maxbounds;
    }
    return true;
Bail:
    return false;
}

/**
 * Reads a pcf file and fill-in a FontRec struct.
 *
 * @param bit bit-level endianness of the machine where the code is running
 * @param byte endianness of the machine where the code is running
 * @param glyph line padding in number of bytes: 1, 2, 4. Each glpyh's bitmap
 * lines will align to the next multiple of @param glyph bytes. This allows to
 * determine the size occupied by a line and the increment to go from one line
 * to another, but it is *not* analoguous to the type used to store the lit bits.
 *
 * To know the size of the line, you need to know the width in pixels and the alignement.
 * note that is *NOT* the type used to store the actual pixels, but the number of bytes
 * you need to read form each CharInfoRec.bits to go get a full line. That's also the
 * "stride", or increment to go from one line to another in the glyph's bitmap (CharInfoRec.bits).
 *
 * If we have 24-pixels wide glyphs and decide to store the bits in bytes, we need 3 bytes to
 * represent one line (8 bits * 3). If @param glyph is 2(shorts, i.e int16) one more byte is needed
 * to reach a number of shorts ( 3 bytes is one short and a half), one padding byte will be added
 * and the line "stride", will be two shorts, 32 bits, comprising 3 bytes of useful data and 1 byte
 * of padding.
 *
 * @param scan The type size (1(uint8), 2(uint16), 4(uint32)) used to store the actual pixels.
 *
 * Examples:
 * +-----------------+----------------------+
 * | Storage type    | Glpyh width (pixels) |
 * | (x bits)        +----+-----+-----+-----+
 * |                 | 4  | 14  | 24  | 36  |
 * +-----------------+----+-----+-----+-----+
 * | Bytes(8)        | 1  | 2   | 3   | 5   |
 * +-----------------+----+-----+-----+-----+
 * | Shorts(16)      | 1  | 1   | 2   | 3   |
 * +-----------------+----+-----+-----+-----+
 * | Ints(32)        | 1  | 1   | 1   | 2   |
 * +-----------------+----+-----+-----+-----+
 *
 * Cells are the final data size. Reads as follows:
 * A glyph which has a wdith of 4 pixels (each line of the glyph is 4 pixels)
 * will take up 1 byte when stored in bytes, 1 short when stored in shorts,
 * and 1 int when stored in ints. Same goes for a 24 pixel wide glyph, each line
 * of it will take 3 bytes if stored bytes, 2 shorts if stored in shorts and
 * 1 int when stored in ints.
 *
 * If the glygh size is 24 and the type size is 1, a line of the glyph needs 3 bytes to be fully
 * described.
 *
 * If @param glyph is 1 as well, there will be no padding. However if @param glyph is
 * 2(uint16) or 4(uint32), those 3 bytes will be padded with more to match the alignment. If the
 * aligment is 2(shorts, 16 bits), we have 3 bytes and we need to get to a number of full shorts.
 * (3 bytes is 1 short and a half), we will add 1 padding byte and those 4 bytes (3 relevant,
 * 1 padding) will make up for the shorts we need (2 in this case). Same reasonning with ints,
 * we add 1 more byte and we get 4 bytes, one int.
 *
 */
int
pcfReadFont(FontPtr pFont, SDL_RWops *file,
	    int bit, int byte, int glyph, int scan)
{
    Uint32      format;
    Uint32      size;
    BitmapFontPtr  bitmapFont = 0;
    int         i;
    PCFTablePtr tables = 0;
    int         ntables;
    int         nmetrics;
    int         nbitmaps;
    int         sizebitmaps;
    int         nink_metrics;
    CharInfoPtr metrics = 0;
    xCharInfo  *ink_metrics = 0;
    char       *bitmaps = 0;
    CharInfoPtr **encoding = 0;
    int         nencoding = 0;
    int         encodingOffset;
    Uint32      bitmapSizes[GLYPHPADOPTIONS];
    Uint32     *offsets = 0;
    bool	hasBDFAccelerators;

    pFont->info.nprops = 0;
    pFont->info.props = 0;
    pFont->info.isStringProp=0;

    if (!(tables = pcfReadTOC(file, &ntables)))
        goto Bail;

    /* properties */

    if (!pcfGetProperties(&pFont->info, file, tables, ntables))
    	goto Bail;

    /* Use the old accelerators if no BDF accelerators are in the file */

    hasBDFAccelerators = pcfHasType (tables, ntables, PCF_BDF_ACCELERATORS);
    if (!hasBDFAccelerators)
        if (!pcfGetAccel (&pFont->info, file, tables, ntables, PCF_ACCELERATORS))
            goto Bail;

    /* metrics */

    if (!pcfSeekToType(file, tables, ntables, PCF_METRICS, &format, &size)) {
    	goto Bail;
    }
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT) &&
	    !PCF_FORMAT_MATCH(format, PCF_COMPRESSED_METRICS)) {
    	goto Bail;
    }
    if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
	    nmetrics = pcfGetINT32(file, format);
    else
    	nmetrics = pcfGetINT16(file, format);
    if (SDL_GzRWEof(file)) goto Bail;
    if (nmetrics < 0 || nmetrics > INT32_MAX / sizeof(CharInfoRec)) {
        SDL_SetError("pcfReadFont(): invalid file format");
        goto Bail;
    }
    metrics = SDL_calloc(nmetrics, sizeof(CharInfoRec));
    if (!metrics) {
    	SDL_SetError("pcfReadFont(): Couldn't allocate metrics (%d*%d)",
		 nmetrics, (int) sizeof(CharInfoRec));
    	goto Bail;
    }
    for (i = 0; i < nmetrics; i++)
        if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT)) {
            if (!pcfGetMetric(file, format, &(metrics + i)->metrics))
            goto Bail;
        } else {
            if (!pcfGetCompressedMetric(file, format, &(metrics + i)->metrics))
            goto Bail;
        }

    /* bitmaps */

    if (!pcfSeekToType(file, tables, ntables, PCF_BITMAPS, &format, &size))
    	goto Bail;
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
	    goto Bail;

    nbitmaps = pcfGetINT32(file, format);
    if (nbitmaps != nmetrics || IS_EOF(file))
    	goto Bail;
    /* nmetrics is already ok, so nbitmap also is */
    offsets = SDL_calloc(nbitmaps, sizeof(Uint32));
    if (!offsets) {
    	SDL_SetError("pcfReadFont(): Couldn't allocate offsets (%d*%d)",
		 nbitmaps, (int) sizeof(Uint32));
	    goto Bail;
    }
    for (i = 0; i < nbitmaps; i++) {
    	offsets[i] = pcfGetINT32(file, format);
	    if (IS_EOF(file)) goto Bail;
    }

    for (i = 0; i < GLYPHPADOPTIONS; i++) {
    	bitmapSizes[i] = pcfGetINT32(file, format);
	    if (IS_EOF(file)) goto Bail;
    }

    sizebitmaps = bitmapSizes[PCF_GLYPH_PAD_INDEX(format)];
    /* guard against completely empty font */
    bitmaps = SDL_malloc(sizebitmaps ? sizebitmaps : 1);
    if (!bitmaps) {
      SDL_SetError("pcfReadFont(): Couldn't allocate bitmaps (%d)", sizebitmaps ? sizebitmaps : 1);
    	goto Bail;
    }
    SDL_RWread(file, bitmaps, sizebitmaps, 1);
    if (IS_EOF(file)) goto Bail;
    position += sizebitmaps;

    if (PCF_BIT_ORDER(format) != bit)
	BitOrderInvert((unsigned char *)bitmaps, sizebitmaps);
    if ((PCF_BYTE_ORDER(format) == PCF_BIT_ORDER(format)) != (bit == byte)) {
        switch (bit == byte ? PCF_SCAN_UNIT(format) : scan) {
        case 1:
            break;
        case 2:
            TwoByteSwap((unsigned char *)bitmaps, sizebitmaps);
            break;
        case 4:
            FourByteSwap((unsigned char *)bitmaps, sizebitmaps);
            break;
        }
    }
    if (PCF_GLYPH_PAD(format) != glyph) {
        char       *padbitmaps;
        int         sizepadbitmaps;
        int         old,
                    new;
        xCharInfo  *metric;

        sizepadbitmaps = bitmapSizes[PCF_SIZE_TO_INDEX(glyph)];
        padbitmaps = SDL_malloc(sizepadbitmaps);
        if (!padbitmaps) {
              SDL_SetError("pcfReadFont(): Couldn't allocate padbitmaps (%d)", sizepadbitmaps);
            goto Bail;
        }
        new = 0;
        for (i = 0; i < nbitmaps; i++) {
            old = offsets[i];
            metric = &metrics[i].metrics;
            offsets[i] = new;
            new += RepadBitmap(bitmaps + old, padbitmaps + new,
                       PCF_GLYPH_PAD(format), glyph,
                  metric->rightSideBearing - metric->leftSideBearing,
                       metric->ascent + metric->descent);
        }
        free(bitmaps);
        bitmaps = padbitmaps;
    }
    for (i = 0; i < nbitmaps; i++)
    	metrics[i].bits = bitmaps + offsets[i];

    free(offsets);
    offsets = NULL;

    /* ink metrics ? */

    ink_metrics = NULL;
    if (pcfSeekToType(file, tables, ntables, PCF_INK_METRICS, &format, &size)) {
        format = pcfGetLSB32(file);
        if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT) &&
            !PCF_FORMAT_MATCH(format, PCF_COMPRESSED_METRICS)) {
            goto Bail;
        }
        if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
            nink_metrics = pcfGetINT32(file, format);
        else
            nink_metrics = pcfGetINT16(file, format);
        if (IS_EOF(file)) goto Bail;
        if (nink_metrics != nmetrics)
            goto Bail;
        /* nmetrics already checked */
        ink_metrics = SDL_calloc(nink_metrics, sizeof(xCharInfo));
        if (!ink_metrics) {
            SDL_SetError("pcfReadFont(): Couldn't allocate ink_metrics (%d*%d)",
                     nink_metrics, (int) sizeof(xCharInfo));
            goto Bail;
        }
        for (i = 0; i < nink_metrics; i++)
            if (PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT)) {
                if (!pcfGetMetric(file, format, ink_metrics + i))
                    goto Bail;
            } else {
                if (!pcfGetCompressedMetric(file, format, ink_metrics + i))
                    goto Bail;
            }
    }

    /* encoding */

    if (!pcfSeekToType(file, tables, ntables, PCF_BDF_ENCODINGS, &format, &size))
	goto Bail;
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
	goto Bail;

    pFont->info.firstCol = pcfGetINT16(file, format);
    pFont->info.lastCol = pcfGetINT16(file, format);
    pFont->info.firstRow = pcfGetINT16(file, format);
    pFont->info.lastRow = pcfGetINT16(file, format);
    pFont->info.defaultCh = pcfGetINT16(file, format);
    if (IS_EOF(file)) goto Bail;
    if (pFont->info.firstCol > pFont->info.lastCol ||
       pFont->info.firstRow > pFont->info.lastRow ||
       pFont->info.lastCol-pFont->info.firstCol > 255) goto Bail;

    nencoding = (pFont->info.lastCol - pFont->info.firstCol + 1) *
	(pFont->info.lastRow - pFont->info.firstRow + 1);

    encoding = calloc(NUM_SEGMENTS(nencoding), sizeof(CharInfoPtr*));
    if (!encoding) {
        SDL_SetError("pcfReadFont(): Couldn't allocate encoding (%d*%d)",
             nencoding, (int) sizeof(CharInfoPtr));
        goto Bail;
    }

    pFont->info.allExist = true;
    for (i = 0; i < nencoding; i++) {
        encodingOffset = pcfGetINT16(file, format);
        if (IS_EOF(file)) goto Bail;
        if (encodingOffset == 0xFFFF) {
            pFont->info.allExist = false;
        } else {
            if(!encoding[SEGMENT_MAJOR(i)]) {
                encoding[SEGMENT_MAJOR(i)]=
                    calloc(BITMAP_FONT_SEGMENT_SIZE, sizeof(CharInfoPtr));
                if(!encoding[SEGMENT_MAJOR(i)])
                    goto Bail;
            }
            ACCESSENCODINGL(encoding, i) = metrics + encodingOffset;
        }
    }

    /* BDF style accelerators (i.e. bounds based on encoded glyphs) */

    if (hasBDFAccelerators)
	if (!pcfGetAccel (&pFont->info, file, tables, ntables, PCF_BDF_ACCELERATORS))
	    goto Bail;

    bitmapFont = SDL_malloc(sizeof *bitmapFont);
    if (!bitmapFont) {
        SDL_SetError("pcfReadFont(): Couldn't allocate bitmapFont (%d)",
             (int) sizeof *bitmapFont);
        goto Bail;
    }

    bitmapFont->version_num = PCF_FILE_VERSION;
    bitmapFont->num_chars = nmetrics;
    bitmapFont->num_tables = ntables;
    bitmapFont->metrics = metrics;
    bitmapFont->ink_metrics = ink_metrics;
    bitmapFont->bitmaps = bitmaps;
    bitmapFont->encoding = encoding;
    bitmapFont->pDefault = (CharInfoPtr) 0;
    if (pFont->info.defaultCh != (unsigned short) NO_SUCH_CHAR) {
        unsigned int r,
                    c,
                    cols;

        r = pFont->info.defaultCh >> 8;
        c = pFont->info.defaultCh & 0xFF;
        if (pFont->info.firstRow <= r && r <= pFont->info.lastRow &&
            pFont->info.firstCol <= c && c <= pFont->info.lastCol) {
            cols = pFont->info.lastCol - pFont->info.firstCol + 1;
            r = r - pFont->info.firstRow;
            c = c - pFont->info.firstCol;
            bitmapFont->pDefault = ACCESSENCODING(encoding, r * cols + c);
        }
    }
//    bitmapFont->bitmapExtra = (BitmapExtraPtr) 0;
    pFont->fontPrivate = (void *) bitmapFont;
//    pFont->get_glyphs = bitmapGetGlyphs;
//    pFont->get_metrics = bitmapGetMetrics;
//    pFont->unload_font = pcfUnloadFont;
//    pFont->unload_glyphs = NULL;
    pFont->bit = bit;
    pFont->byte = byte;
    pFont->glyph = glyph;
    pFont->scan = scan;
    free(tables);
    return Successful;
Bail:
    free(ink_metrics);
    if(encoding) {
        for(i=0; i<NUM_SEGMENTS(nencoding); i++)
            free(encoding[i]);
    }
    free(encoding);
    free(bitmaps);
    free(metrics);
    free(pFont->info.props);
    pFont->info.nprops = 0;
    pFont->info.props = 0;
    free (pFont->info.isStringProp);
    free(bitmapFont);
    free(tables);
    free(offsets);
    return AllocError;
}

int
pcfReadFontInfo(FontInfoPtr pFontInfo, SDL_RWops *file)
{
    PCFTablePtr tables;
    int         ntables;
    Uint32      format;
    Uint32      size;
    int         nencoding;
    bool	hasBDFAccelerators;

    pFontInfo->isStringProp = NULL;
    pFontInfo->props = NULL;
    pFontInfo->nprops = 0;

    if (!(tables = pcfReadTOC(file, &ntables)))
	goto Bail;

    /* properties */

    if (!pcfGetProperties(pFontInfo, file, tables, ntables))
	goto Bail;

    /* Use the old accelerators if no BDF accelerators are in the file */

    hasBDFAccelerators = pcfHasType (tables, ntables, PCF_BDF_ACCELERATORS);
    if (!hasBDFAccelerators)
	if (!pcfGetAccel (pFontInfo, file, tables, ntables, PCF_ACCELERATORS))
	    goto Bail;

    /* encoding */

    if (!pcfSeekToType(file, tables, ntables, PCF_BDF_ENCODINGS, &format, &size))
	goto Bail;
    format = pcfGetLSB32(file);
    if (!PCF_FORMAT_MATCH(format, PCF_DEFAULT_FORMAT))
	goto Bail;

    pFontInfo->firstCol = pcfGetINT16(file, format);
    pFontInfo->lastCol = pcfGetINT16(file, format);
    pFontInfo->firstRow = pcfGetINT16(file, format);
    pFontInfo->lastRow = pcfGetINT16(file, format);
    pFontInfo->defaultCh = pcfGetINT16(file, format);
    if (IS_EOF(file)) goto Bail;
    if (pFontInfo->firstCol > pFontInfo->lastCol ||
       pFontInfo->firstRow > pFontInfo->lastRow ||
       pFontInfo->lastCol-pFontInfo->firstCol > 255) goto Bail;

    nencoding = (pFontInfo->lastCol - pFontInfo->firstCol + 1) *
	(pFontInfo->lastRow - pFontInfo->firstRow + 1);

    pFontInfo->allExist = true;
    while (nencoding--) {
	if ((Uint16)pcfGetINT16(file, format) == 0xFFFF)
	    pFontInfo->allExist = false;
	if (IS_EOF(file)) goto Bail;
    }
    if (IS_EOF(file)) goto Bail;

    /* BDF style accelerators (i.e. bounds based on encoded glyphs) */

    if (hasBDFAccelerators)
	if (!pcfGetAccel (pFontInfo, file, tables, ntables, PCF_BDF_ACCELERATORS))
	    goto Bail;

    free(tables);
    return Successful;
Bail:
    pFontInfo->nprops = 0;
    free (pFontInfo->props);
    free (pFontInfo->isStringProp);
    free(tables);
    return AllocError;
}

void
pcfUnloadFont(FontPtr pFont)
{
    BitmapFontPtr  bitmapFont;
    int i,nencoding;

    bitmapFont = pFont->fontPrivate;
    free(bitmapFont->ink_metrics);
    if(bitmapFont->encoding) {
        nencoding = (pFont->info.lastCol - pFont->info.firstCol + 1) *
	    (pFont->info.lastRow - pFont->info.firstRow + 1);
        for(i=0; i<NUM_SEGMENTS(nencoding); i++)
            free(bitmapFont->encoding[i]);
    }
    free(bitmapFont->encoding);
    free(bitmapFont->bitmaps);
    free(bitmapFont->metrics);
    free(pFont->info.isStringProp);
    free(pFont->info.props);
    free(bitmapFont);
}
