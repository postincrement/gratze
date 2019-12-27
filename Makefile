#SDL_DIR =  `pwd`/SDL2-2.0.10/x86_64-w64-mingw32/
SDL_DIR =  ./SDL2-2.0.10/i686-w64-mingw32

CXX = i686-w64-mingw32-g++
CC  = i686-w64-mingw32-gcc

CXXFLAGS += -I$(SDL_DIR)/include -std=c++17 -Wno-register -g -DEXECZ80  
CFLAGS += -g -DLSB_FIRST -DDEBUG -DEXECZ80

LDFLAGS += -L$(SDL_DIR)/lib 
LDLIBS += -lmingw32 -lSDL2main -lSDL2 -static-libstdc++ -static-libgcc  
LINK.o = $(LINK.cc)

vpath %.c mfz80

gratze:	main.o Z80.o sdl_video.o emulator.o trs80.o debug.o fdc.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

main.o:	main.cc Makefile trs80.h emulator.h fdc.h options.h

Z80.o: mfz80/Z80.c Makefile

debug.o: mfz80/debug.c

sdl_video.o: sdl_video.cc

emulator.o: emulator.cc emulator.h fdc.h options.h

trs80.o: trs80.cc trs80.h fdc.h emulator.h options.h

fdc.o: fdc.cc fdc.h options.h

clean:
	rm -f gratze *.o
