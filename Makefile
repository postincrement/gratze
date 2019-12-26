#SDL_DIR =  `pwd`/SDL2-2.0.10/x86_64-w64-mingw32/
SDL_DIR =  ./SDL2-2.0.10/i686-w64-mingw32

CXX = i686-w64-mingw32-g++
CC  = i686-w64-mingw32-gcc

CXXFLAGS += -I$(SDL_DIR)/include
LDFLAGS += -L$(SDL_DIR)/lib 
LDLIBS += -lmingw32 -lSDL2main -lSDL2 -static-libstdc++ -static-libgcc 
LINK.o = $(LINK.cc)

vpath %.c mfz80

gratze:	main.o Z80.o sdl_video.o emulator.o trs80.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

main.o:	main.cc Makefile

Z80.o: mfz80/Z80.c

sdl_video.o: sdl_video.cc

emulator.o: emulator.cc

trs80.o: trs80.cc

clean:
	rm -f gratze *.o
