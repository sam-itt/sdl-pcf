.. default-domain:: C

Static fonts
================================================================================


Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1. :c:macro:`PCF_TEXTURE_TYPE`
#. :c:macro:`PCF_TEXTURE_SDL2`
#. :c:macro:`PCF_TEXTURE_GPU`
#. :c:macro:`PCF_LOWER_CASE`
#. :c:macro:`PCF_UPPER_CASE`
#. :c:macro:`PCF_ALPHA`
#. :c:macro:`PCF_DIGITS`

Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1. :c:func:`PCF_FontCreateStaticFont`
#. :c:func:`PCF_FontCreateStaticFontVA`
#. :c:func:`PCF_FreeStaticFont`
#. :c:func:`PCF_StaticFontGetCharRect`
#. :c:func:`PCF_StaticFontGetSizeRequest`
#. :c:func:`PCF_StaticFontGetSizeRequestRect`
#. :c:func:`PCF_StaticFontCanWrite`
#. :c:func:`PCF_StaticFontCreateTexture`

Structure documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:type:: PCF_StaticFont

   The structure has the following public members:

.. code-block:: c

   typedef struct{
       SDL_Surface *raster;
       xCharInfo   metrics;
       SDL_Texture|GPU_Image  *texture;
   }PCF_StaticFont;


.. c:member:: PCF_StaticFont raster

   The pre-rendered characters for that font, in a raster. Usable for any software
   blitting operation.

.. c:member:: PCF_StaticFont texture

   The pre-rendered characters for that font in a GPU-friendly texture. Be sure to call
   :c:func:`PCF_StaticFontCreateTexture` before using it.


.. c:member:: PCF_StaticFont metrics

   See somewhere else

Macros documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:macro:: PCF_TEXTURE_TYPE

   Defined at build time to either :c:macro:`PCF_TEXTURE_SDL2` (default) or
   :c:macro:`PCF_TEXTURE_GPU`

.. c:macro:: PCF_TEXTURE_SDL2

   SDL_pcf is built to support SDL2 textures. :c:type:`PCF_StaticFont` **texture**
   member is of SDL_Texture* type

.. c:macro:: PCF_TEXTURE_GPU

   SDL_pcf is built to support SDL_gpu textures. :c:type:`PCF_StaticFont`
   **texture** member is of GPU_Image* type

.. c:macro:: PCF_LOWER_CASE

   Pre-defined character set for use with :c:func:`PCF_FontCreateStaticFont`
.. code-block:: c

   #define PCF_LOWER_CASE "abcdefghijklmnopqrstuvwxyz"

.. c:macro:: PCF_UPPER_CASE

   Pre-defined character set for use with :c:func:`PCF_FontCreateStaticFont`
.. code-block:: c

   #define PCF_UPPER_CASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

.. c:macro:: PCF_ALPHA

   Pre-defined character set for use with :c:func:`PCF_FontCreateStaticFont`
.. code-block:: c

   #define PCF_ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

.. c:macro:: PCF_DIGITS

   Pre-defined character set for use with :c:func:`PCF_FontCreateStaticFont`
.. code-block:: c

   #define PCF_DIGITS "0123456789"


Functions documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: PCF_StaticFont *PCF_FontCreateStaticFont(PCF_Font *font, SDL_Color *color, int nsets, ...)

    Creates and return a pre-drawn set of characters.
    The font can be closed afterwards. The return value must be freed by the
    caller using :c:func:`PCF_FreeStaticFont`.

    Once drawn, static fonts are immutable: You can't add characters on the fly,
    or change colors. You'll need to create a new static font to do that. The
    purpose of PCF_StaticFont is to integrate with rendering systems based on
    fixed bitmap data + coordinates, like SDL_Renderer or OpenGL.

    Parameters:
        | **font**  The font to draw with
        | **color** The color of the pre-rendered glyphs
        | **nsets** The number of glyph sets that follows
        | **...**   Sets of glyphs to include in the cache, as const char*. You can
          use pre-defined sets such as :c:macro:`PCF_ALPHA`, :c:macro:`PCF_DIGITS`, etc. The function will
          filter out duplicated characters.

    Returns:
        a newly allocated PCF_StaticFont or NULL on error. The error will be
        available with SDL_GetError()

.. c:function:: PCF_StaticFont *PCF_FontCreateStaticFontVA(PCF_Font *font, SDL_Color *color, int nsets, size_t tlen, va_list ap)

    va_list version of PCF_FontCreateStaticFont. The only difference is that
    this function needs to be provided with the total(cumulative) length of
    all the strings that it gets through ap. This is due to the fact that
    va_list can't be rewinded when passed as an argument to a non-variadic
    function

    Parameters:
        | **font** See :c:func:`PCF_FontCreateStaticFont` font
        | **color** See :c:func:`PCF_FontCreateStaticFont` color
        | **nsets** See :c:func:`PCF_FontCreateStaticFont` nsets
        | **tlen** Total (cumulative) len of the strings passed in.
        | **ap** List of  nsets char*

    Returns:
        See :c:func:`PCF_FontCreateStaticFont`.

.. c:function:: void PCF_FreeStaticFont(PCF_StaticFont *self)

    Frees memory used by a static font. Each static font created using
    PCF_FontCreateStaticFont should be released using this function.

    Parameters:
        self The PCF_StaticFont to free.

.. c:function:: int PCF_StaticFontGetCharRect(PCF_StaticFont *font, int c, SDL_Rect *glyph)

    Find the area in self->raster holding a glyph for c. The area is
    suitable for a SDL_BlitSurface or a SDL_RenderCopy operation using
    self->raster as a source

    Parameters:
        | **font** The static font to search in.
        | **c**    The char to search for.
        | **glyph** Location where to put the coordinates, when found.

    Returns:
        0 for whitespace (**glpyh** untouched), non-zero if **font**
        has something printable for **c**: 1 if the char as been found,
        -1 otherwise. When returning -1, **glpyh** has been set to the default glyph.

.. c:function:: void PCF_StaticFontGetSizeRequest(PCF_StaticFont *font, const char *str, Uint32 *w, Uint32 *h)

    Computes space (pixels width*height) needed to draw a string using a given
    font. Both **w** and **h** can be NULL depending on which metric you
    are interested in. The function won't fail if both are NULL, it'll just be
    useless.

    Parameters:
        | **str** The string whose size you need to know.
        | **font** The font you want to use to write that string
        | **w** Pointer to somewhere to place the resulting width. Can be NULL.
        | **h** Pointer to somewhere to place the resulting height. Can be NULL.

.. c:function:: void PCF_StaticFontGetSizeRequestRect(PCF_StaticFont *font, const char *str, SDL_Rect *rect)

    Same PCF_StaticFontGetSizeRequest as but fills an SDL_Rect. Rect x and y
    get initialized to 0.

    Parameters:
        | **str** The string whose size you need to know.
        | **font** The font you want to use to write that string
        | **rect** Pointer to an existing SDL_Rect (cannot be NULL) to fill with
          the size request.

.. c:function:: bool PCF_StaticFontCanWrite(PCF_StaticFont *font, SDL_Color *color, const char *sequence)

    Check whether **font** can be used to write all chars given in
    **sequence** in color **color**.

    Parameters:
        | **color** The color you want to write in
        | **sequence** All the chars you may want to use


    Returns:
        true if all chars of **sequence** can be written in
        **color**, false otherwise.

.. c:function:: void PCF_StaticFontCreateTexture()

    Creates a hardware-friendly texture into **font**. Parameters depends on which support
    (SDL2_Renderer or SDL_gpu) was compiled in.

    Parameters:
        | **font** The font to act on
        | **renderer** When using SDL2_Renderer, the renderer which the texture will belong
          to
