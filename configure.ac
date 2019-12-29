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
AC_CONFIG_SRCDIR([emulator.cc])

# Put autotools auxiliary files in subdirectories to reduce clutter:
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

if test \! -d m4/aclocal.m4 ; then
  AC_MSG_ERROR([run "aclocal"])
fi

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
AC_CONFIG_HEADERS([config.h])

##########################################################
#
# Checks for programs, e.g., AC_PROG_CC
#
AC_PROG_CXX
AC_PROG_CC

#CXX = i686-w64-mingw32-g++
#CC  = i686-w64-mingw32-gcc
#CXXFLAGS += -I$(SDL_DIR)/include -I$(NFD_DIR)/src/include -std=c++17 -Wno-register -g -DEXECZ80 
#CFLAGS += -g -DLSB_FIRST -DDEBUG -DEXECZ80 

##########################################################
#
# set SDL  
#

#CXXFLAGS += -I$(SDL_DIR)/include -I$(NFD_DIR)/src/include 
#CFLAGS += -g -DLSB_FIRST -DDEBUG -DEXECZ80 


##########################################################
#
# handle nativefiledialog 
#

NFD_DIR=./nativefiledialog
NFD_INCLUDE_DIR="$NFD_DIR/src/include"
NFD_BUILD_ENV="CC=$CXX CXX=$CXX"
NFD_CXXFLAGS="-I$NFD_INCLUDE_DIR"

AC_CHECK_FILE([$NFD_INCLUDE_DIR/nfd.h], [], AC_MSG_ERROR([cannot find nfd.h]))

case "$target" in
        *-mingw32-*)
                NFD_BUILD_DIR=gmake_windows
                NFD_LIBS="-lole32 -luuid" 
                ;;
        *-linux-*)
                NFD_BUILD_DIR=gmake_linux
                PKG_CHECK_MODULES([GTK], [gtk+-3.0],
                        [AC_DEFINE([HAVE_GTK], [1], [Define if gtk+-3.0 library available])],
                        [AC_MSG_ERROR([nfd on linux requires gtk. Install libgtk-3-dev])])
                ;;
                # none of the above        
        *)
                AC_MSG_NOTICE([cannot determine NFD build type for $target])
                ;;
esac

case "$target" in
        # generic linux systems
        i686-*)
                NFD_BUILD_TYPE=release_x86
                NFD_LDFLAGS="-L$NFD_DIR/build/lib/Release/x86"
                ;;
        x86_64-*)
                NFD_BUILD_TYPE=release_x64
                NFD_LDFLAGS="-L$NFD_DIR/build/lib/Release/x64"
                ;;
        # none of the above        
        *)
                AC_MSG_NOTICE([cannot determine NFD build type for $target])
                ;;
esac

AC_SUBST(NFD_BUILD_ENV)
AC_SUBST(NFD_BUILD_DIR)
AC_SUBST(NFD_BUILD_TYPE)

AC_MSG_NOTICE([NFD build type is $NFD_BUILD_TYPE])

##########################################################
#
# set compile flags for mingw
#
case "$target" in
        # generic linux systems
        *-mingw32-*)
                MING_LIBS="-lmingw32 -static-libstdc++ -static-libgcc"
                ;;
        # none of the above        
        *)
                ;;
esac

##########################################################
#
#  set flags for MFZ
#
MFZ_CFLAGS="-DEXECZ80"

##########################################################
#
# Checks for libraries.
#

##########################################################
#
# Checks for header files.
#
#AC_SEARCH_LIBS(deflate, z, [], [AC_ERROR([A working zlib is required])])

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
CFLAGS="$MFZ_CFLAGS"
CXXFLAGS="$MFZ_CFLAGS $NFD_CXXFLAGS -std=c++17 -Wno-register"
LDFLAGS="$NFD_LDFLAGS"
LDLIBS="-lnfd $MING_LIBS"

##########################################################
#
# Do final output.
#
AC_OUTPUT