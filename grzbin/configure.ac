# This is David A. Wheeler's template for configure.ac, showing
# some suggested options and setups.

# Process this file with autoconf to produce a configure script.

##########################################################
#
# Initialize autoconf.
#
AC_INIT([grzbin], 
        [0.1], 
        [craig@postincrement.com],
        [gratze-tar.gz], 
        [http://www.postincrement.com])


# Force autoconf to be at least this version number:
AC_PREREQ([2.68])

# Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([grzbin.cc])

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
# Checks for programs, e.g., AC_PROG_CC
#
AC_PROG_CXX
AC_PROG_CC

##########################################################
#
# settings based on architecture
#

AC_SUBST(MING_ARCH)
AC_SUBST(MINGW_ENABLED)

case "$target" in
        *-darwin*)
		;;
		
        *-mingw32)
                MINGW_ENABLED=1
                MING_LDLIBS="-static-libgcc -static-libstdc++ -lmingw32" 
                ;;

        *-linux-*)
                ;;
esac

case "$target" in
        # generic linux systems
        i686-*)
                MING_ARCH=i686
                ;;
        x86_64-*)
                MING_ARCH=x86_64
                ;;
        # none of the above        
        *)
                ;;
esac

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
LDFLAGS="$MING_LDFLAGS"
LDLIBS="$MING_LDLIBS"

##########################################################
#
# Do final output.
#
AC_OUTPUT
