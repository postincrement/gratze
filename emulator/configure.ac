# This is David A. Wheeler's template for configure.ac, showing
# some suggested options and setups.

# Process this file with autoconf to produce a configure script.

##########################################################
#
# Initialize autoconf.
#
AC_INIT([gratze], 
        [0.1], 
        [craig@postincrement.com],
        [gratze-tar.gz], 
        [http://www.postincrement.com])


# Force autoconf to be at least this version number:
AC_PREREQ([2.68])

# Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([src/main.cc])

# Put autotools auxiliary files in subdirectories to reduce clutter:
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

# get system type
AC_CANONICAL_SYSTEM

# Enable "automake" to simplify creating makefiles:
#AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror])

##########################################################
#
# add Makefile
#
AC_CONFIG_FILES([Makefile])
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDLIBS)
AC_SUBST(LDFLAGS)

##########################################################
#
# add header files
#
AC_CONFIG_HEADERS([src/config.h])

##########################################################
#
# Checks for programs, e.g., AC_PROG_CC
#
AC_PROG_CXX
AC_PROG_CC

##########################################################
#
# settings based on architecture
#

NFD_DIR=./nativefiledialog
NFD_INCLUDE_DIR="$NFD_DIR/src/include"
NFD_BUILD_ENV="CC=$CXX CXX=$CXX"
NFD_CXXFLAGS="-I$NFD_INCLUDE_DIR"

AC_SUBST(MING_ENABLED)
AC_SUBST(MING_ARCH)
AC_SUBST(MING_SDL_BUILD_TYPE)

AC_SUBST(NFD_BUILD_ENV)
AC_SUBST(NFD_BUILD_DIR)
AC_SUBST(NFD_BUILD_TYPE)
AC_SUBST(NFD_LIB_DIR)
AC_SUBST(NFD_LIB_NAME)
AC_SUBST(MINGW_ENABLED)

case "$target" in
        *-darwin*)
                NFD_BUILD_DIR=gmake_macosx
                NFD_LIBS="-lnfd" 
                NFD_LIB_NAME=libnfd.a

		SDL_INC=-I/opt/local/include/SDL2
                SDL_LIBS="-lSDL2_ttf -lSDL2main -lSDL2"
		SDL_LDFLAGS=-L/opt/local/lib
		;;
		
        *-mingw32)
                MINGW_ENABLED=1
                NFD_BUILD_DIR=gmake_windows
                NFD_LIBS="-l:nfd.lib -lole32 -luuid" 
                NFD_LIB_NAME=nfd.lib

                SDL_LIBS="-lSDL2_ttf -lSDL2main -lSDL2"
                MING_LDLIBS="-static-libgcc -static-libstdc++ -lmingw32" 
                ;;

        *-linux-*)
                NFD_BUILD_DIR=gmake_linux
                NFD_LIBS="-lnfd" 
                NFD_LIB_NAME=libnfd.a

                PKG_CHECK_MODULES([SDL], [sdl2],
                        [AC_DEFINE([HAVE_SDL], [1], [Define if SDL library available])],
                        [AC_MSG_ERROR([SDL2 required. Install libsdl2-dev])])
                PKG_CHECK_MODULES([SDL_ttf], [SDL2_ttf],
                        [AC_DEFINE([HAVE_SDL_TTF], [1], [Define if SDL-ttf library available])],
                        [AC_MSG_ERROR([SDL2 TTF required. Install libsdl2-ttf-dev])])
                PKG_CHECK_MODULES([FREETYPE], [freetype2],
                        [AC_DEFINE([HAVE_FREETYPE], [1], [Define if freetype library available])],
                        [AC_MSG_ERROR([FreeType required. Install libfreetype6-dev])])
                PKG_CHECK_MODULES([GTK2], [gtk+-3.0],
                        [AC_DEFINE([HAVE_GTK], [1], [Define if GTK library available])],
                        [AC_MSG_ERROR([GTK+ 3.0 required. Install libgtk-3-0-dev])])
                SDL_INC=`pkg-config --cflags-only-I sdl2 gtk+-3.0 SDL2_ttf freetype2 sdl2`
                SDL_LIBS=`pkg-config --libs gtk+-3.0 SDL2_ttf freetype2 sdl2`
                
                PKG_CHECK_MODULES([MHASH], [mhash],
                        [AC_DEFINE([HAVE_MHASH], [1], [Define if libhash library available])],
                        [AC_MSG_ERROR([libmhash required. Install libmhash-dev])])
                ;;
esac

case "$target" in
        # generic linux systems
        i686-*)
                NFD_BUILD_TYPE=release_x86
                NFD_LIB_DIR="$NFD_DIR/build/lib/Release/x86"

                MING_ARCH=i686
                MING_SDL_BUILD_TYPE=i686-w64-mingw32
                ;;
        x86_64-*)
                NFD_BUILD_TYPE=release_x64
                NFD_LIB_DIR="$NFD_DIR/build/lib/Release/x64"

                MING_ARCH=x86_64
                MING_SDL_BUILD_TYPE=x86_64-w64-mingw32
                ;;
        # none of the above        
        *)
                AC_MSG_NOTICE([cannot determine NFD build type for $target])
                ;;
esac

NFD_LDFLAGS="-L$NFD_LIB_DIR"

AC_MSG_NOTICE([NFD build type is $NFD_BUILD_TYPE])

##########################################################
#
#  set flags for MFZ
#
MFZ_CFLAGS="-DEXECZ80 -DLSB_FIRST"

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
CFLAGS="$SDL_INC $MFZ_CFLAGS -g"
CXXFLAGS="$SDL_INC $MFZ_CFLAGS $NFD_CXXFLAGS -std=c++17 -Wno-register -g"
LDFLAGS="$SDL_LDFLAGS $MING_LDFLAGS $NFD_LDFLAGS"
LDLIBS="$NFD_LIBS $GTK_LIBS $MING_LDLIBS $SDL_LIBS"

##########################################################
#
# Do final output.
#
AC_OUTPUT
