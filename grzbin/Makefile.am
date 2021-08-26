
bin_PROGRAMS = grzbin

grzbin_DEPENDENCIES = $(abs_top_builddir)/../common/libgrzcommon.a
grzbin_LDADD = $(abs_top_builddir)/../common/libgrzcommon.a

grzbin_CXXFLAGS = -std=c++17 \
                   -I$(abs_top_builddir)/../cereal/include \
                   -I$(top_srcdir)/.. \
                   -I$(top_srcdir)


grzbin_SOURCES = grzbin.cc