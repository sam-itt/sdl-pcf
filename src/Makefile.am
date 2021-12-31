ACLOCAL_AMFLAGS = "-Im4"


lib_LTLIBRARIES = libSDL2_pcf.la

libSDL2_pcfincludedir = $(includedir)/SDL2

libSDL2_pcfinclude_HEADERS = pcfread.h \
							 SDL_pcf.h

libSDL2_pcf_la_CPPFLAGS = -I$(top_srcdir)/src $(SDL2_CFLAGS)
libSDL2_pcf_la_SOURCES = pcfread.c \
						 utilbitmap.c \
						 SDL_GzRW.c \
						 SDL_pcf.c

libSDL2_pcf_la_LDFLAGS = \
	-no-undefined -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(LIBS)

EXTRA_DIST = pcf.h \
			 SDL_GzRW.h \
			 utilbitmap.h
