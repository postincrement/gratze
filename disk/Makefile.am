noinst_LIBRARIES = libgrzdisk.a

libgrzdisk_a_CXXFLAGS = -std=c++17 \
                   -I$(abs_top_builddir)/../cereal/include \
                   -I$(top_srcdir)/.. \
                   -I$(top_srcdir)

libgrzdisk_a_SOURCES = dmk.cc dsk.cc grzdisk.cc jv1.cc jv3.cc virtual_drive.cc

bin_PROGRAMS = grzdisk

grzdisk_CXXFLAGS = -std=c++17 \
                   -I$(abs_top_builddir)/../cereal/include \
                   -I$(top_srcdir)/.. \
                   -I$(top_srcdir)

grzdisk_DEPENDENCIES = $(abs_top_builddir)/../common/libgrzcommon.a libgrzdisk.a

grzdisk_LDADD = $(abs_top_builddir)/../common/libgrzcommon.a                   

grzdisk_LDFLAGS  = -L. -lgrzdisk 
grzdisk_SOURCES = grzdisk.cc cpmfs.cc