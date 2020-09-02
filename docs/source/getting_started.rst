Using SDL_pcf
===============================================================================

SDL_pcf works with bitmap fonts distributed as PCF files. Those fonts
originates from Unix X11 and are distributed as a collection of files each file
being a representation of characters in the font in a specific size/variant,
variant being italic and bold.

For example, the file that comes with SDL_pcf tests, **ter-x24n.pcf.gz** is the
Terminus font, "normal" variant, 24 points. The "bold" variant would be
**ter-x24b.pcf.gz**. Chances are high that you already have a couple of fonts
installed on your system.

Find them out with:

.. code-block:: bash

   $ find /usr/share/fonts/ -name "*.pcf" -or -name "*.pcf.gz"

SDL_pcf can be used in two ways:

- Direct writing of any character of the font to an SDL_Surface or using a
  SDL_Renderer.
- Pre-render a set of characters to a surface or a texture and then blit
  those pre-rendererd characters to their destination.
  This can be hardware accelerated (SDL_Renderer or SDL_gpu).

Direct writing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This mode is the simplest and the more flexible, all characters from the font
can be drawn. However it requires pixel-level access as SDL_pcf will use the
data from the PCF font to light the appropriate pixels.

It can be used either with SDL_Surfaces or go through the SDL_Renderer API.

A typical use case would be like:

.. code-block:: c
  :linenos:
  :emphasize-lines: 43-49

   #include <stdio.h>
   #include <stdlib.h>
   #include <SDL.h>

   #include "SDL_pcf.h"

   #define SCREEN_WIDTH 640
   #define SCREEN_HEIGHT 480

   int main(int argc, char *argv[])
   {
       SDL_Window* window = NULL;
       SDL_Surface* screenSurface = NULL;
       Uint32 black, white;
       PCF_Font *font;

       if (SDL_Init(SDL_INIT_VIDEO) < 0) {
           fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
           return 1;
       }

       window = SDL_CreateWindow(
                   "SDL_pcf test drive",
                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                   SCREEN_WIDTH, SCREEN_HEIGHT,
                   SDL_WINDOW_SHOWN
                   );
       if (window == NULL) {
           fprintf(stderr, "could not create window: %s\n", SDL_GetError());
           return 1;
       }

       screenSurface = SDL_GetWindowSurface(window);
       if(!screenSurface){
           printf("Error: %s\n",SDL_GetError());
           exit(-1);
       }

       white = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
       black  = SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00);
       SDL_FillRect(screenSurface, NULL, black);

       font = PCF_OpenFont("ter-x24n.pcf.gz");
       if(!font){
           printf("%s\n", SDL_GetError());
           exit(EXIT_FAILURE);
       }

       PCF_FontWrite(font, "Hello, World!", white, screenSurface, NULL);
       SDL_UpdateWindowSurface(window);

       SDL_Delay(4000);

       PCF_CloseFont(font);

       SDL_DestroyWindow(window);
       SDL_Quit();

       exit(EXIT_SUCCESS);
   }

Build it with:

.. code-block:: bash

   $ gcc simple-test.c -o simple-test `pkg-config sdl2 SDL2_pcf --libs --cflags`

The relevant part is highlighted. The call to :c:func:`PCF_FontWrite` will
write the **"Hello, World!"** string at 0,0 on **screenSurface**.

You can see a more complex example of this in **test/ayba.c**.

Static fonts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Static fonts are a set of pre-rendered characters built from a font. This allows
to avoid the need of direct pixel access to the destination and allows faster
blitting and hardware acceleration.

Static fonts are built from a loaded font and a set of characters. They are thus
limited to that set.

**Note:** Static fonts can be compiled to use either SDL_Renderer(default)
**or** SDL_gpu. This is defined at compile time and can be check using
:c:macro:`PCF_TEXTURE_TYPE` which will be either equal to
:c:macro:`PCF_TEXTURE_SDL2` or :c:macro:`PCF_TEXTURE_GPU`.

SDL_pcf provide support functions to locate the character to copy but the actual
blitting must be done by the client code using the StaticFont struct **texture**
member which will be of **SDL_Texture**(default) or **GPU_Image** type, depending
of build-time configuration (see note below).

You can see a good example of how all of this works together in:
- **test/ayba-sf.c** for the SDL_Renderer API
- **test/ayba-sf-gpu.c** for usage with SDL_gpu
