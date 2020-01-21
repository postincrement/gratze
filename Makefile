

LINK.o = $(LINK.cc)
OBJDIR = ./obj
DEPDIR = ./.deps

APP = gratze

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

MINGW_ENABLED=1

ifneq ($(MINGW_ENABLED),)

MING_ARCH=x86_64
MING_SDL_BUILD_TYPE=x86_64-w64-mingw32

SDL_VERSION=2.0.10
SDL_TTF_VERSION=2.0.15

SDL_BASE=SDL2-$(SDL_VERSION)
SDL_TGZ=SDL2-devel-$(SDL_VERSION)-mingw.tar.gz
SDL_URL=https://www.libsdl.org/release/$(SDL_TGZ)
SDL_TARGETS=SDL2.dll

SDL_DIR=$(SDL_BASE)/$(MING_ARCH)-w64-mingw32
SDL_TARGET_SRC=$(addprefix $(SDL_DIR)/bin/,$(SDL_TARGETS))


SDL_TTF_BASE=SDL2_ttf-$(SDL_TTF_VERSION)
SDL_TTF_TGZ=SDL2_ttf-devel-$(SDL_TTF_VERSION)-mingw.tar.gz
SDL_TTF_URL=https://www.libsdl.org/projects/SDL_ttf/release/$(SDL_TTF_TGZ)
SDL_TTF_TARGETS=SDL2_ttf.dll libfreetype-6.dll zlib1.dll

SDL_TTF_DIR=$(SDL_TTF_BASE)/$(MING_ARCH)-w64-mingw32
SDL_TTF_TARGET_SRC=$(addprefix $(SDL_TTF_DIR)/bin/,$(SDL_TTF_TARGETS))  

SDL_INC=-I$(SDL_DIR)/include/SDL2 -I$(SDL_TTF_DIR)/include/SDL2
SDL_LDFLAGS=-L$(SDL_DIR)/lib -L$(SDL_TTF_DIR)/lib

endif # MING_ENABLED

INC_DIRS = -I$(shell pwd)

NFD_BUILD_TYPE=release_x64
NFD_BUILD_ENV=CC=x86_64-w64-mingw32-g++ CXX=x86_64-w64-mingw32-g++
NFD_BUILD_DIR=gmake_windows
NFD_LIB_DIR=./nativefiledialog/build/lib/Release/x64
NFD_LIB=$(NFD_LIB_DIR)/nfd.lib

CC=x86_64-w64-mingw32-gcc
CXX=x86_64-w64-mingw32-g++

CXXFLAGS :=  -DEXECZ80 -DLSB_FIRST -I./nativefiledialog/src/include -std=c++17 -Wno-register -g $(INC_DIRS) $(SDL_INC)
CFLAGS   :=  -DEXECZ80 -DLSB_FIRST -g   $(INC_DIRS) $(SDL_INC) -DDEBUG  # DEBUG symbol needed for mf80 debug

LDFLAGS=$(SDL_LDFLAGS)  -L./nativefiledialog/build/lib/Release/x64 -g
LDLIBS=-l:nfd.lib -lole32 -luuid  -static-libgcc -static-libstdc++ -lmingw32 -lSDL2_ttf -lSDL2main -lSDL2

COMPILE.c  = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

OUTPUT_OPTION = -o $(OBJDIR)/$*.o

%.o : %.c 
$(OBJDIR)/%.o : %.c | $(DEPDIR) $(OBJDIR)
	@echo "(CC) $<" 
	@$(COMPILE.c) $(OUTPUT_OPTION) $<
				
%.o : %.cc
$(OBJDIR)/%.o : %.cc | $(DEPDIR) $(OBJDIR)
	@echo "(CXX) $<" 
	@$(COMPILE.cc) $(OUTPUT_OPTION) $<

all:	$(APP)

################################################################################################	

SRCS_CC = main.cc \
					mainwindow.cc \
	  			magmedia/fdc.cc \
					magmedia/cassette.cc \
					devices/keyscan.cc \
					video/virtual_screen.cc \
					video/font.cc \
					video/dg640.cc \
					video/chargen_2513.cc \
					video/chargen_mcm6574.cc \
					video/chargen_mcm6674.cc \
					cpu/emulator.cc \
					cpu/z80emulator.cc \
					cpu/2650emulator.cc \
					cpu/cpu_2650.cc \
					devices/z80pio.cc \
					trs80/trs80.cc \
					trs80/model1/model1.cc \
					trs80/model3/model3.cc \
					trs80/model3/chargen_model3.cc \
					trs80/model4/model4.cc \
					dg680/dg680.cc \
					super80/super80.cc \
					2650/binbug/binbug.cc \

SRCS_C  = trs80/model1/rom_level1.c \
          trs80/model1/rom_level2.c \
          trs80/model3/rom_model3.c \
					SDL_FontCache/SDL_FontCache.c \
  				cpu/mfz80/Z80.c \
					cpu/mfz80/Debug.c \
					dg680/rom_dgos1_4.c \
					super80/rom_super80.c \
					2650/binbug/rom_binbug_6_1ROM.c

FILENAMES	:= $(notdir $(basename $(SRCS_C) $(SRCS_CC)))
OBJS	    := $(addsuffix .o,$(addprefix $(OBJDIR)/,$(FILENAMES)))
DEPFILES  := $(addsuffix .d,$(addprefix $(DEPDIR)/,$(FILENAMES)))

vpath %.c  $(sort $(dir $(SRCS_C)))
vpath %.cc $(sort $(dir $(SRCS_CC)))


$(APP):	$(OBJS)

gratze:	nfd sdl ttf $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJDIR):
	@mkdir -p $@

$(DEPFILES): $(DEPDIR)

$(DEPDIR):
	@mkdir -p $@

include $(wildcard $(DEPFILES))

################################################################################################	
#
#   MingW libraries
#

ifneq ($(MINGW_ENABLED),)

#
# MingW SDL
#

# Original URL https://www.libsdl.org/release/SDL2-devel-2.0.10-mingw.tar.gz

.PHONY: sdl
sdl: $(SDL_TARGETS)

$(SDL_TARGETS): $(SDL_TARGET_SRC)
	cp $^ .

$(SDL_TARGET_SRC) : $(SDL_TGZ)
	@echo "Extracting SDL" && \
	tar -xmzf $(SDL_TGZ)

$(SDL_TGZ) :
	@echo "Downloading SDL - $(SDL_URL)" && \
	wget $(SDL_URL)

#
# MingW SDL_ttf
#

# Original URL https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.15-mingw.tar.gz

.PHONY: ttf
ttf: $(SDL_TTF_TARGETS)

$(SDL_TTF_TARGETS): $(SDL_TTF_TARGET_SRC)
	cp $^ .

$(SDL_TTF_TARGET_SRC) : $(SDL_TTF_TGZ)
	@echo "Extracting SDL_ttf" && \
	tar -xmzf $(SDL_TTF_TGZ)

$(SDL_TTF_TGZ) :
	@echo "Downloading SDL_ttf - $(SDL_TTF_URL)" && \
	wget $(SDL_TTF_URL)

#####

clean_sdl:
	rm -rf $(SDL_DIR)     $(SDL_TGZ)     $(SDL_TARGETS) \
	       $(SDL_TTF_DIR) $(SDL_TTF_TGZ) $(SDL_TTF_TARGETS)

################################################################################################	

else

sdl:		# configure uses packages on other platforms

ttf:		# configure uses packages on other platforms

endif # ($(MINGW_ENABLED),)

################################################################################################	
#
#  Native file dialogs
#

.PHONY : nfd

$(NFD_LIB): nfd

.PHONY : clean_nfd

clean_nfd:
	cd nativefiledialog/build/$(NFD_BUILD_DIR) && $(NFD_BUILD_ENV) sh -c "make config=$(NFD_BUILD_TYPE) clean"

nfd: 
	if test \! -d nativefiledialog ; then \
		git clone https://github.com/postincrement/nativefiledialog.git ; \
	  cd nativefiledialog/build/$(NFD_BUILD_DIR) && \
		$(NFD_BUILD_ENV) sh -c "make config=$(NFD_BUILD_TYPE) clean && make config=$(NFD_BUILD_TYPE) nfd" ; \
	else \
	  cd nativefiledialog/build/$(NFD_BUILD_DIR) && \
		$(NFD_BUILD_ENV) sh -c "make config=$(NFD_BUILD_TYPE) nfd" ; \
	fi

################################################################################################	
#
#  SDLFontCache
#

$(OBJDIR)/SDL_FontCache.o : ./SDL_FontCache/SDL_FontCache.c

./SDL_FontCache/SDL_FontCache.c : ./SDL_FontCache

./SDL_FontCache :
		git clone https://github.com/grimfang4/SDL_FontCache.git

################################################################################################	

.PHONY : clean

clean: clean_nfd
	rm -f $(APP) $(OBJS) $(DEPFILES)

