.. default-domain:: C

Direct Writing
================================================================================


Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1. :c:func:`PCF_OpenFont`
#. :c:func:`PCF_CloseFont`
#. :c:func:`PCF_FontWriteChar`
#. :c:func:`PCF_FontWrite`
#. :c:func:`PCF_FontGetSizeRequest`
#. :c:func:`PCF_FontGetSizeRequestRect`

#. :c:func:`PCF_FontRenderChar`
#. :c:func:`PCF_FontRender`

Functions documentation
~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: PCF_Font *PCF_OpenFont(const char *filename)

    Opens a PCF font file. Supports both .pcf and .pcf.gz.

    Parameters:
        **filename** The file to open

    Returns:
        a PCF_Font opaque struct representing the font.
        The caller must call PCF_CloseFont when done using the font.

.. c:function:: void PCF_CloseFont(PCF_Font *self)

    Free resources taken up by a loaded font.
    Caller code must always call PCF_CloseFont on all fonts
    it allocates. Each PCF_OpenFont must be paired with a
    matching PCF_CloseFont.

    Parameters:
        **self** The font to free.

.. c:function:: bool PCF_FontWriteChar(PCF_Font *font, int c, Uint32 color, SDL_Surface *destination, SDL_Rect *location)

    Writes a character on screen, and advance the location by one char width.
    If the surface is too small to fit the char or if the glyph is partly out
    of the surface (start writing a 18 pixel wide char 2 pixels before the edge)
    only the pixels that can be written will be drawn, resulting in a partly
    drawn glyph and the function will return false.

    Parameters:
        | **c** The ASCII code of the char to write. You can of course use 'a' instead of 97.
        | **font** The font to use to write the char. Opened by PCF_OpenFont.
        | **color** The color of text. Must be in @param destination format (use SDL_MapRGB/SDL_MapRGBA to build a suitable value).
        | **destination** The surface to write to.
        | **location** Where to write on the surface. Can be NULL to write at 0,0. If not NULL, location will be advanced by the width.

    Returns:
        True on success(the whole char has been written), false on error/partial draw. Details of the failure can be retreived with SDL_GetError().

.. c:function:: bool PCF_FontWrite(PCF_Font *font, const char *str, Uint32 color, SDL_Surface *destination, SDL_Rect *location)

    Writes a string on screen. This function will try it's best to write
    as many chars as possible: If the surface is not wide enough to accomodate
    the whole string, it will stop at the very last pixel (and return false).
    This function doesn't wrap lines. Use PCF_FontGetSizeRequest to get needed
    space for a given string/font.

    Parameters:
       | **str** The string to write.
       | **font** The font to use. Opened by PCF_OpenFont.
       | **color** The color of text. Must be in @param destination format (use SDL_MapRGB/SDL_MapRGBA to build a suitable value).
       | **destination** The surface to write to.
       | **location** Where to write on the surface. Can be NULL to write at 0,0. If not NULL, location will be advanced by the width of the string.


    Returns:
        True on success(the whole string has been written), false on error/partial draw. Details of the failure can be retreived with SDL_GetError().

.. c:function:: bool PCF_FontRenderChar(PCF_Font *font, int c, SDL_Renderer *renderer, SDL_Rect *location)

    Writes a character on a SDL_Renderer, and advance the given location by one
    char width.
    If the renderer is too small to fit the char or if the glyph is partly out
    of the surface (start writing a 18 pixel wide char 2 pixels before the edge)
    only the pixels that can be written will be drawn, resulting in a partly
    drawn glyph and the function will return false.

    Note that there is no color parameter: This is controlled at the
    SDL_Renderer level with SDL_SetRenderDrawColor.

    Parameters:
        c The ASCII code of the char to write. You can of course use 'a'
        instead of 97.
        font The font to use to write the char. Opened by PCF_OpenFont.
        renderer The renderer that will be used to draw.
        location Location within the renderer. Can be NULL to write at
        0,0. If not NULL, location will be advanced by the width.

    Returns:
        True on success(the whole char has been written), false on error/partial
        draw. Details of the failure can be retreived with SDL_GetError().

.. c:function:: bool PCF_FontRender(PCF_Font *font, const char *str, SDL_Color *color, SDL_Renderer *renderer, SDL_Rect *location)

    Writes a string on renderer. This function will try it's best to write
    as many chars as possible: If the renderer is not wide enough to accomodate
    the whole string, it will stop at the very last pixel (and return false).
    This function doesn't wrap lines. Use PCF_FontGetSizeRequest to get needed
    space for a given string/font.

    Parameters:
        str The string to write.
        font The font to use. Opened by PCF_OpenFont.
        color The color of text. If not NULL, it will overrede the current
        renderer's color. If NULL, the current renderer's color will be used.
        renderer The rendering context to use.
        location Where to write on the renderer. Can be NULL to write at
        0,0. If not NULL, location will be advanced by the width of the string.

    Returns:
        True on success(the whole string has been written), false on error/partial
        draw. Details of the failure can be retreived with SDL_GetError().

.. c:function:: void PCF_FontGetSizeRequest(PCF_Font *font, const char *str, Uint32 *w, Uint32 *h)

    Computes space (pixels width*height) needed to draw a string using a given
    font. Both @param w and @param h can be NULL depending on which metric you
    are interested in. The function won't fail if both are NULL, it'll just be
    useless.

    Parameters:
        str The string whose size you need to know.
        font The font you want to use to write that string
        w Pointer to somewhere to place the resulting width. Can be NULL.
        h Pointer to somewhere to place the resulting height. Can be NULL.

.. c:function:: void PCF_FontGetSizeRequestRect(PCF_Font *font, const char *str, SDL_Rect *rect)

    Same PCF_FontGetSizeRequest as but fills an SDL_Rect. Rect x and y
    get initialized to 0.

    Parameters:
        str The string whose size you need to know.
        font The font you want to use to write that string
        rect Pointer to an existing SDL_Rect (cannot be NULL) to fill with
        the size request.

