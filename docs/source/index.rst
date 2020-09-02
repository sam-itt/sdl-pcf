.. SDL_pcf documentation master file, created by
   sphinx-quickstart on Fri Aug 28 19:24:38 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to SDL_pcf's documentation!
===================================
With **SDL_pcf** you can know easily use pixel-perfect bitmap fonts with SDL2:

- Use thousands of existing X11 bitmap fonts, including terminus_
- Direct-to-surface writing (no temp surface needed)
- Hardware-accelerated rendering supported with SDL2 Renderer API or SDL_gpu_

.. _SDL_gpu: https://github.com/grimfang4/sdl-gpu
.. _terminus: http://terminus-font.sourceforge.net/

Using PCF fonts is as simple as:

.. code-block:: c

   PCF_Font *font;
   font = PCF_OpenFont("ter-x24n.pcf.gz");
   PCF_FontWrite(font, "Hello world !", white, screenSurface, &location);

.. toctree::
   :maxdepth: 2
   :caption: Getting Started:

   build
   getting_started

.. toctree::
   :maxdepth: 2
   :caption: API:

   api


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
