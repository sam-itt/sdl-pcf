#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "SDL_GzRW.h"
#include "SDL_rwops.h"
#include "pcf.h"
#include "pcfread.h"

/*Use glyph to avoid collision with C 'char' type*/
/*@param c: ascii code for the char to dump*/
void dump_glpyh(FontRec *font, int c)
{
    int w, h;
    int line_bsize;
    CharInfoRec *glyph;
    BitmapFontRec *bitmapFont;

    bitmapFont  = font->fontPrivate;
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
    printf("font->glyph is %d\n", font->glyph);
    line_bsize = ceil(w/(font->glyph * 8.0))*font->glyph; /*in bytes*/
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
    printf("\n");
}



int main(int argc, char *argv[])
{
    SDL_RWops *file;
    FontRec font;
    BitmapFontRec *bitmapFont;
    CharInfoRec *ci;


    if(argc < 2){
        printf("Usage: %s filename\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    file = SDL_RWFromGzFile(argv[1], "rb");
    if(!file){
        printf("Couldn't open font %s\n",argv[1]);
        exit(EXIT_FAILURE);
    }

    int glyph = 4;
    int scan = 1;
    pcfReadFont(&font, file, LSBFirst, LSBFirst, glyph, scan);
    SDL_RWclose(file);
    SDL_FreeRW(file);

    dump_glpyh(&font, 'G');

	exit(EXIT_SUCCESS);
}

