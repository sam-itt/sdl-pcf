#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"

int main(int argc, char *argv[])
{
    PCF_Font *font;

    if(argc < 3){
        printf("Usage: %s font-filename char\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    font = PCF_OpenFont(argv[1]);
    if(!font){
        printf("%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    PCF_FontDumpGlyph(font, *argv[2]);
    PCF_CloseFont(font);

	exit(EXIT_SUCCESS);
}

