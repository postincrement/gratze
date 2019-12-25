#SDL_DIR =  `pwd`/SDL2-2.0.10/x86_64-w64-mingw32/
SDL_DIR =  `pwd`/SDL2-2.0.10/i686-w64-mingw32/

CXX =  i686-w64-mingw32-g++
CXXFLAGS += -I$(SDL_DIR)/include
LDFLAGS += -L$(SDL_DIR)/lib 
LDLIBS += -lmingw32 -lSDL2main -lSDL2 -static-libstdc++ -static-libgcc 
LINK.o = $(LINK.cc)

main:	main.o 

main.o:	main.cc Makefile

clean:
	rm main main.o
