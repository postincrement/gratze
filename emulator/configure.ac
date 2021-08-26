# This is David A. Wheeler's template for configure.ac, showing
# some suggested options and setups.

# Process this file with autoconf to produce a configure script.

##########################################################
#
# Initialize autoconf.
#
AC_INIT([gratze], [0.1])


# Force autoconf to be at least this version number:
AC_PREREQ([2.68])

# Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([src/main.cc])

# Put autotools auxiliary files in subdirectories to reduce clutter:
AC_CONFIG_AUX_DIR([build-aux])
#AC_CONFIG_MACRO_DIR([m4])

# Enable "automake" to simplify creating makefiles:
AM_INIT_AUTOMAKE([-Wall -Werror foreign  subdir-objects])
#AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror])

##########################################################
#
# SDL
#
SDL_VERSION=2.0.0
AM_PATH_SDL($SDL_VERSION,
        :,
        AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"
AC_SEARCH_LIBS([TTF_Quit], [SDL2_ttf], [],[
  AC_MSG_ERROR([SDL2_ttf not installed])
],[-lSDL2])


##########################################################
#
# add Makefile
#
CFLAGS="$CFLAGS"
AC_SUBST(CFLAGS)

CXXFLAGS="$CXXFLAGS"
AC_SUBST(CXXFLAGS)

LIBS="$LIBS"
AC_SUBST(LIBS)

#AC_SUBST(LDLIBS)
#AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([Makefile])

##########################################################
#
# Checks for programs, e.g., AC_PROG_CC
#
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

##########################################################
#
# Checks for libraries.
#

##########################################################
#
# Checks for header files.
#

##########################################################
#
# Checks for typedefs, structures, and compiler characteristics.
#

##########################################################
#
# Checks for library functions.
#

##########################################################
#
#  construct final flags
#
LDFLAGS=
LDLIBS=

##########################################################
#
# Do final output.
#
AC_OUTPUT
