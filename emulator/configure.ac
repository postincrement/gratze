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
# add Makefile
#
AC_CONFIG_FILES([Makefile])
#AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
#AC_SUBST(LDLIBS)
#AC_SUBST(LDFLAGS)

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
