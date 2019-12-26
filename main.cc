#include <SDL2/SDL.h> 

#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "trs80.h"


using namespace std;

/////////////////////////////////////////////////////

#if 0
void Zed80::SigHandler(int sig)
{
  if (sig == SIGINT)
    g_instance->m_sigInt = true;
}
#endif

int main(int argc, char *argv[]) 
{   
  // returns zero on success else non-zero 
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
    printf("error initializing SDL: %s\n", SDL_GetError()); 
    return -1;
  }

  // instantiate and open and start the emulator
  std::unique_ptr<Emulator> emulator(new TRS80Emulator());
  if (!emulator->Open(argc, argv)) {
    cerr << "error: cannot open emulator" << endl;
    return -1;
  }
  if (!emulator->Start()) {
    cerr << "error: cannot start emulator" << endl;
    return -1;
  }

  // capture signal handler
  //m_sigInt = false;
  //signal(SIGINT, &Zed80::SigHandler);

  // put something in the video memory
  for (int i = 0; i < 64*16; ++i)
    emulator->WriteVideoChar(i, i & 0xff);

  // run emulator
  int count = 0;
  for (;;) {
    // give CPU some time
    emulator->Run();

    // look for events
    if (count++ > 20) {
      count = 0;
      SDL_Event event;
      if (SDL_PollEvent(&event)) {
        switch (event.type) { 
          case SDL_KEYDOWN:
            if (event.key.repeat == 0)
              emulator->OnKeyDown(event.key.keysym);
            break;

          case SDL_KEYUP:
            if (event.key.repeat == 0)
              emulator->OnKeyUp(event.key.keysym);
            break;

          default:
            break;
        }
      }
    }
  }

  cout << "finished" << endl;
}
