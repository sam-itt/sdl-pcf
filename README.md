# SDL_pcf - SDL Bitmap fonts made easy
------------------------------------------
Have you ever wanted pixel-perfect bitmap fonts for your SDL-based project?
Search no more, the solution is here!

![Pixel-perfect bitmap font with SDL](https://github.com/sam-itt/sdl-pcf/raw/master/sdl-pcf-bitmap-font.gif)


SDL_pcf lets you make use of thousands of X11 bitmap fonts, including [terminus](http://terminus-font.sourceforge.net/). It uses **proven** PCF format handling code straight from the latest offical release of [libxfont](https://github.com/freedesktop/libXfont), tailored and adapted to fit nicely with SDL.

# Using PCF bitmap fonts in your project
----------------------------------------
Using PCF fonts is as simple as:
```C
SDL_PcfFont *font;
font = SDL_PcfOpenFont("ter-x24n.pcf.gz");
SDL_PcfWrite("Hello world !", font, white, screenSurface, &location);
```
You just need to include 4 source files and 4 headers in your project. No library building script is provided (yet, contributions are welcome).

# Documentation and demo code
----------------------------------------
SDL_pcf currently has 4 public functions:
- SDL_PcfOpenFont(const char *filename)
- SDL_PcfCloseFont(SDL_PcfFont *self)
- SDL_PcfWriteChar
- SDL_PcfWrite
- SDL_PcfGetSizeRequest

Names have been chosen to be self explanatory, but documentation is
also provided as javadoc-like comment blocs before each function in SDL_pcf.c.

A demo code is provided in test.c that can be built on Unix using make:
```sh
$ make
$ ./test-pcf
#press <ESC> to quit
```
### Dependencies:
- SDL (1 or 2)
- zlib
