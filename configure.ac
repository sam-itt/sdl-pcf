AC_INIT([SDL_pcf], [0.0.1])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/SDL_pcf.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
LT_INIT

# Setup for automake - Taken from SDL_gfx
# Making releases:
#   MICRO_VERSION += 1;
#   INTERFACE_AGE += 1;
#   BINARY_AGE += 1;
# if any functions have been added, set INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set BINARY_AGE and INTERFACE_AGE to 0.

MAJOR_VERSION="0"
MINOR_VERSION="0"
MICRO_VERSION="1"
INTERFACE_AGE="0"
BINARY_AGE="0"
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(INTERFACE_AGE)
AC_SUBST(BINARY_AGE)
AC_SUBST(VERSION)

# libtool versioning - Taken from SDL_gfx
LT_RELEASE=$MAJOR_VERSION.$MINOR_VERSION
LT_CURRENT=`expr $MICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`

AC_SUBST([LT_RELEASE])
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

#Arguments
AC_ARG_WITH([sdl1], AS_HELP_STRING([--without-sdl1], [Disable building for SDL1 eve, if detected]))
AC_ARG_WITH([sdl2], AS_HELP_STRING([--without-sdl2], [Disable building for SDL2 even if detected]))

# Detect the canonical host and target build environment
AC_CANONICAL_HOST
#AC_CANONICAL_TARGET

# Check for compilers
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL

AS_IF([test "x$with_sdl1" != "xno"],
    [PKG_CHECK_MODULES([SDL], [sdl], [HAVE_SDL1=1], [HAVE_SDL1=0])],
    [HAVE_SDL1=0]
)
AM_CONDITIONAL(HAVE_SDL1, test "x$HAVE_SDL1" != "x0")

AS_IF([test "x$with_sdl2" != "xno"],
    [PKG_CHECK_MODULES([SDL2], [sdl2], [HAVE_SDL2=1], [HAVE_SDL2=0])],
    [HAVE_SDL2=0]
)
AM_CONDITIONAL(HAVE_SDL2, test "x$HAVE_SDL2" != "x0")

AS_IF([test "x$HAVE_SDL1" == "x0" && test "x$HAVE_SDL2" == "x0"],
    [AC_MSG_ERROR("SDL_pcf needs either SDL1 or SDL2. None found")]
)

AC_CHECK_LIB([z], [gzopen], [], [AC_MSG_ERROR([Z library not found,
              please install zlib.], [1])])

#SDL_gpu doesn't have a pkg-config file yet
AC_CHECK_LIB(SDL_gpu, GPU_Init, [HAVE_SDL_GPU=1])
AC_CHECK_LIB(SDL2_gpu, GPU_Init, [HAVE_SDL2_GPU=1])

AM_CONDITIONAL(HAVE_SDL_GPU, test "x$HAVE_SDL_GPU" == "x1")
AM_CONDITIONAL(HAVE_SDL2_GPU, test "x$HAVE_SDL2_GPU" == "x1")

# Export to config.h
AS_IF([test "x$HAVE_SDL1" == "x1"], AC_DEFINE(HAVE_SDL1,[1],[Build for SDL1]))
AS_IF([test "x$HAVE_SDL2" == "x1"], AC_DEFINE(HAVE_SDL2,[1],[Build for SDL2]))
AS_IF([test "x$HAVE_SDL_GPU" == "x1"], AC_DEFINE(HAVE_SDL_GPU,[1],[Enable support for SDL_gpu]))
AS_IF([test "x$HAVE_SDL2_GPU" == "x1"], AC_DEFINE(HAVE_SDL2_GPU,[1],[Enable support for SDL2_gpu]))

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 ])

AM_COND_IF([HAVE_SDL1], [AC_CONFIG_FILES([SDL_pcf.pc])])
AM_COND_IF([HAVE_SDL2], [AC_CONFIG_FILES([SDL2_pcf.pc])])

AC_OUTPUT
