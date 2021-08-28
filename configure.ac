AC_INIT([gratze], 
        [0.1], 
        [craig@postincrement.com],
        [gratze-tar.gz], 
        [http://www.postincrement.com])

AM_INIT_AUTOMAKE

# Force autoconf to be at least this version number:
AC_PREREQ([2.68])

AC_PROG_CXX

CXXFLAGS=std=gnu++17

AC_CONFIG_COMMANDS([co-cereal], 
                   [[if test \! -d cereal ; then \
                       git clone -b v1.3.0 https://github.com/USCiLab/cereal.git ; \
                     fi]])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS(common disk grzbin emulator)
AC_OUTPUT
