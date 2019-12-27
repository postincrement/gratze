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
  Options options;

  // parse command line arguments
  int optIndex = 1;
  while (optIndex < argc) {
    std::string arg(argv[optIndex]);
    size_t len = arg.length();

    // non-option argument terminates options
    if (arg[0] != '-')
      break;

    // solitary "-"" terminates options
    if (len == 1) {
      ++optIndex;
      break;
    }

    std::string opt(arg.substr(1, 1));
    if (arg[0] == '-') {
      // solitary "--" terminates options
      if (len == 2) {
        optIndex++;
        break;
      }
      opt = arg.substr(2);
    }

    // select ROM
    if ((opt == "rom") || (opt == "r")) {
      if (++optIndex >= argc) {
        cerr << "error: --rom option requires filename argument" << endl;
        return -1;
      }
      options.m_romFn = argv[optIndex++];
    }

    // select drives
    else if ((opt.substr(0, 5) == "drive")) {
      std::string driveNumStr(opt.substr(5));
      int driveNum = atoi(driveNumStr.c_str());
      if (++optIndex >= argc) {
        cerr << "error: --drivex option requires filename argument" << endl;
        return -1;
      }
      options.m_driveFns[driveNum] = std::string(argv[optIndex++]);
    }

    // breakpoint
    else if ((opt == "b") || (opt == "breakpoint")) {
      if (++optIndex >= argc) {
        cerr << "error: --breakpoint option requires address argument" << endl;
        return -1;
      }
      std::string arg(argv[optIndex]);
      int addr = strtoul(arg.c_str(), NULL, 16);
      options.m_breakpoint = addr;
    }

    else {
      cerr << "error: unknown option '" << arg << "'" << endl;
      return -1;
    }
  }

  // returns zero on success else non-zero 
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
    printf("error initializing SDL: %s\n", SDL_GetError()); 
    return -1;
  }

  // instantiate and open and start the emulator
  std::unique_ptr<Emulator> emulator(new TRS80Emulator());
  if (!emulator->Open(options)) {
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

  emulator->Execute();  
  
  cout << "finished" << endl;
}
