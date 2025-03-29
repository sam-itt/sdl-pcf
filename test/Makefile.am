AM_CPPFLAGS = -I$(top_srcdir)/src

LDADD = ../src/libSDL2_pcf.la $(SDL2_LIBS)
AM_CPPFLAGS += $(SDL2_CFLAGS)

check_PROGRAMS = ayba ayba-sf
if USE_SGPU_TEXTURE
check_PROGRAMS += ayba-sf-gpu
endif
check_PROGRAMS += placement-test
check_PROGRAMS += number-test
check_PROGRAMS += glyph-dump
AM_DEFAULT_SOURCE_EXT = .c

EXTRA_DIST = simple-test.c simple-test-sf.c ter-x24n.pcf.gz
