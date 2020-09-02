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
PCF_Font *font;
font = PCF_OpenFont("ter-x24n.pcf.gz");
PCF_FontWrite(font, "Hello world !", white, screenSurface, &location);
```
You just need to include SDL_pcf.h and call pkg-config with SDL2_pcf.

# Documentation and demo code
----------------------------------------
Full documentation can be found [here](https://sdl-pcf.readthedocs.io/en/latest/).

Demo code is provided in test/ that can be built on Unix using make:
```sh
$ make check
$ ./test/ayba
#press <ESC> to quit
```
### Dependencies:
- SDL2
- zlib
- Optional support for SDL_gpu
