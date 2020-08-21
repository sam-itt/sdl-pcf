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

# Detect the canonical host and target build environment
AC_CANONICAL_HOST
#AC_CANONICAL_TARGET

# Check for compilers
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES([SDL2], [sdl2])

AC_CHECK_LIB([z], [gzopen], [], [AC_MSG_ERROR([Z library not found,
              please install zlib.], [1])])

AC_CONFIG_FILES([Makefile
                 SDL2_pcf.pc
                 src/Makefile
                 test/Makefile
                 ])
AC_OUTPUT
