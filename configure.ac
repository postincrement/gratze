AC_INIT([gratze], 
        [0.1], 
        [craig@postincrement.com],
        [gratze-tar.gz], 
        [http://www.postincrement.com])


# Force autoconf to be at least this version number:
AC_PREREQ([2.68])

# Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([emulator/src/emulator.cc])

# Put autotools auxiliary files in subdirectories to reduce clutter:
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([common/config.h])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([disk emulator])
AC_OUTPUT
