ACLOCAL_AMFLAGS = "-Im4"

SUBDIRS = src test .

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA =
if HAVE_SDL2
pkgconfig_DATA += SDL2_pcf.pc
endif
if HAVE_SDL1
pkgconfig_DATA += SDL_pcf.pc
endif

CLEANFILES = SDL_pcf.pc SDL2_pcf.pc
