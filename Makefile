#SDL_DIR =  `pwd`/SDL2-2.0.10/x86_64-w64-mingw32/
SDL_DIR =  ./SDL2-2.0.10/i686-w64-mingw32

CXX = i686-w64-mingw32-g++
CC  = i686-w64-mingw32-gcc

NFD_DIR=./nativefiledialog

NFD_LIB=$(NFD_DIR)/build/lib/Release/x86/nfd.lib

#SDL_LIBS = $(shell $(SDL_DIR)/bin/sdl2-config --libs)
SDL_LIBS=-lmingw32 -lSDL2main -lSDL2

CXXFLAGS += -I$(SDL_DIR)/include -I$(NFD_DIR)/src/include -std=c++17 -Wno-register -g -DEXECZ80 
CFLAGS += -g -DLSB_FIRST -DDEBUG -DEXECZ80 

LDFLAGS += -L$(SDL_DIR)/lib 
LDLIBS += $(NFD_LIB) -lole32 -luuid $(SDL_LIBS) -static-libstdc++ -static-libgcc  
LINK.o = $(LINK.cc)


vpath %.c mfz80

gratze:	main.o cassette.o Z80.o sdl_video.o emulator.o trs80.o Debug.o fdc.o $(NFD_LIB)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

main.o:	main.cc Makefile trs80.h emulator.h fdc.h options.h cassette.h

Z80.o: mfz80/Z80.c Makefile

debug.o: mfz80/debug.c

sdl_video.o: sdl_video.cc

emulator.o: emulator.cc emulator.h fdc.h options.h cassette.h

trs80.o: trs80.cc trs80.h fdc.h emulator.h options.h cassette.h

trs80.cc: $(NFD_LIB)

fdc.o: fdc.cc fdc.h options.h

cassette.o: cassette.cc cassette.h

clean:
	rm -f gratze *.o


$(NFD_LIB): nativefiledialog
	cd nativefiledialog/build/gmake_windows && CC=$(CXX) CXX=$(CXX) sh -c "make config=release_x86 clean ; make config=release_x86"

nfd: 
	cd nativefiledialog/build/gmake_windows && CC=$(CXX) CXX=$(CXX) sh -c "make config=release_x86 clean ; make config=release_x86"

nativefiledialog:
	git clone https://github.com/postincrement/nativefiledialog.git
	#git clone https://github.com/mlabbe/nativefiledialog.git
