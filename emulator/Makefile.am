
bin_PROGRAMS = gratze

noinst_LIBRARIES = libnfd.a

gratze_CFLAGS   = $(CFLAGS) -DEXECZ80

gratze_CXXFLAGS =  -std=c++17  -Wno-register \
									 $(gratze_CFLAGS) \
                   -I$(abs_top_builddir)/../cereal/include \
                   -I$(abs_top_builddir)/../nativefiledialog/src/include/ \
                   -I$(top_srcdir)/.. \
                   -I$(top_srcdir) \
									 -I$(top_srcdir)/nativefiledialog/src/include

gratze_LDADD = $(abs_top_builddir)/../disk/libgrzdisk.a \
               $(abs_top_builddir)/../common/libgrzcommon.a \
							 libnfd.a 


gratze_LDFLAGS = -framework Foundation -framework AppKit


gratze_SOURCES = \
    src/main.cc \
		src/mainwindow.cc \
		src/emulator.cc \
		../common/cmdargs.cc \
		../common/misc.cc \
		../common/binfile.cc \
		devices/fdc.cc \
		devices/keyscan.cc \
		devices/keypar.cc \
		devices/z80pio.cc \
		devices/intel8255.cc \
		devices/synertek6545.cc \
		video/virtual_screen.cc \
		video/font.cc \
		video/dg640.cc \
		video/chargen_2513.cc \
		video/chargen_mcm6574.cc \
		video/chargen_mcm6674.cc \
		terminal/terminal.cc \
		SDL_FontCache/SDL_FontCache.c \
		z80/z80emulator.cc \
		z80/trs80/trs80.cc \
		z80/trs80/model1/model1.cc \
		z80/trs80/model3/model3.cc \
		z80/trs80/model3/chargen_model3.cc \
		z80/trs80/model4/model4.cc \
		z80/microbee/microbee.cc \
		z80/microbee/mbee_font.cc \
		z80/sorcerer/sorcerer.cc \
		z80/sorcerer/chargen_sorcerer.cc \
		z80/dg680/dg680.cc \
		z80/super80/super80.cc \
		z80/cpm80/cpm80.cc \
		2650/2650emulator.cc \
		2650/cpu_2650.cc \
		2650/eti685/eti685.cc \
		2650/78up5/78up5.cc \
		z80/trs80/model1/rom_level1.c \
		z80/trs80/model1/rom_level2.c \
		z80/trs80/model3/rom_model3.c \
		z80/dg680/rom_dgos1_4.c \
		z80/super80/rom_super80.c \
		z80/microbee/rom_basic5_22e.c \
		z80/microbee/rom_disk56.c \
		z80/microbee/rom_disk128.c \
		z80/microbee/rom_bn5443.c \
		z80/microbee/starnet/starnet.cc \
		z80/sorcerer/rom_sorcerer.c \
		z80/mfz80/Z80.c \
		z80/mfz80/Debug.c \
		z80/mfz80/dasm.c \
		z80/cpm80/newbdos.cc \
		z80/cpm80/newbdos_rom.c \
		2650/eti685/rom_binbug_6_1.c \
		2650/78up5/rom_pipbug.c 


libnfd_a_OBJCFLAGS = -I$(top_srcdir)/nativefiledialog/src/include

libnfd_a_SOURCES = nativefiledialog/src/nfd_common.c \
									nativefiledialog/src/nfd_cocoa.m

libnfd_a_CFLAGS = -I$(top_srcdir)/nativefiledialog/src/include
