AM_CPPFLAGS = -I$(top_srcdir)/src

if HAVE_SDL2
LDADD = ../src/libSDL2_pcf.la $(SDL2_LIBS)
AM_CPPFLAGS += $(SDL2_CFLAGS)
else
LDADD = ../src/libSDL_pcf.la $(SDL_LIBS)
AM_CPPFLAGS += $(SDL_CFLAGS)
endif

check_PROGRAMS = ayba ayba-sf
AM_DEFAULT_SOURCE_EXT = .c
