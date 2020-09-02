Building SDL_pcf
================================

| **SDL_pcf** depends on SDL2 and zlib. It has optional support for SDL_gpu_


.. _SDL_gpu: https://github.com/grimfang4/sdl-gpu

Unix (Autotools)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ ./configure --prefix=/usr
  $ make
  $ make check                 # build demos in ./test (optional)
  $ [sudo] make install        # install to system (optional)

**make** will build cglm to **src/.libs** sub folder in project folder.
If you don't want to install **cglm** to your system's folder you can get static and dynamic libs in this folder.
Headers (\*.h) will be found in the src folder.


Documentation (Sphinx):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**SDL_pcf** documentation is based on sphinx_.

To build html documentation do the following:

.. code-block:: bash
  :linenos:

  $ cd docs
  $ make html

.. _sphinx: http://terminus-font.sourceforge.net/
