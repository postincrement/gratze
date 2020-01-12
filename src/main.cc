#include <SDL.h> 

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#include "src/config.h"
#include "src/misc.h"
#include "src/mainwindow.h"
#include "src/cmdargs.h"

#include "z80/trs80/model1/model1.h"
#include "z80/trs80/model3/model3.h"
#include "z80/trs80/model4/model4.h"
#include "z80/dg680/dg680.h"
#include "z80/super80/super80.h"
#include "z80/microbee/microbee.h"
#include "2650/eti685/eti685.h"

#include "src/factory.h"


using namespace std;

using EmulatorFactory = Factory<Emulator, std::string>;
static EmulatorFactory g_emulatorFactory;

/////////////////////////////////////////////////////

void Init()
{
  g_emulatorFactory.AddConcreteClass<Model1Level2_Emulator>("m1");
  g_emulatorFactory.AddConcreteClass<Model1Level1_Emulator>("m11");
  g_emulatorFactory.AddConcreteClass<Model1Level2_Emulator>("m12");
  g_emulatorFactory.AddConcreteClass<Model3_Emulator>("m3");
  g_emulatorFactory.AddConcreteClass<Model4_Emulator>("m4");
  g_emulatorFactory.AddConcreteClass<DG680_Emulator>("dg680");
  g_emulatorFactory.AddConcreteClass<ETI685>("eti685");
  g_emulatorFactory.AddConcreteClass<Super80_Emulator>("super80");
  g_emulatorFactory.AddConcreteClass<Microbee_Emulator>("microbee");

  std::vector<std::string> keys;
  g_emulatorFactory.GetKeys(keys);

  for (auto & r : keys) {
    std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
    emulator->Init();
  }
}

static CommandLineArgs::Option g_commandLineOptions[] = {
  { 'h', "help",        ' ', "display this help message" },
  { 'r', "rom",         's', "name of ROM"    },
  { ' ', "ram",         'u', "RAM size in k" },
  { ' ', "drive*",      's', "name of file for virtual disk drive" },
  { 'b', "breakpoint",  'x', "breakpoint address"  },
  { 'f', "font",        's', "use TTF font"},
  { 's', "fontSize",    'u', "TTF font size" },
  { ' ', "diskette",    's', "test virtual drive file"},
  { ' ', "cassette",    's', "test virtual cassette file"},
  { 't', "type",        's', "look for model" },
  { ' ', "readdebug",   'b', "turn read debugging on or off" },
  { ' ', "writedebug",  'b', "turn write debugging on or off" },
  { ' ', "ei",          'b', "enable/disable Model 1 Expansion Interface" }
};

extern "C"
int main(int argc, char *argv[]) 
{
  Init();

  CommandLineArgs args(g_commandLineOptions);
  if (!args.Parse(argc, argv)) {
    return -1;
  }

  cout << args.DumpValues();

  if (args.HasArg("-h")) {
    cout << "usage: gratze [options] args...\n"
         << "where options are:\n"
        << args.Usage();
    return 0;
  }

  std::string fn;
  if (args.GetValue("--diskette", fn)) {
    VirtualDriveFile file;
    if (!file.Open(fn, true)) {
      cerr << "error: could not open diskette file '" << fn << "'" << endl;
      return false;
    }
    cerr << "file '" << fn << "' opened" << endl;
    return 0;
  }

  if (args.GetValue("--cassette", fn)) {
    VirtualCassetteFile file;
    if (!file.ReadOpen(fn)) {
      cerr << "error: could not open cassette file '" << fn << "'" << endl;
      return -1;
    }
    cout << "info: file '" << fn << "' opened, internal filename is " << file.GetFilename() << endl;
    return 0;
  }

  Options options;

  // set default type
  if (!args.GetValue("-t", options.m_typeName))
    options.m_typeName = "m1";

  args.GetValue("-r",    options.m_romFn);
  args.GetValue("--ram", options.m_ramSize_k);

  if (args.GetValue("-f", options.m_font)) {
    if (!args.GetValue("-s", options.m_fontSize)) {
      options.m_fontSize = 20; 
    }
  }

  std::string error;
  if (!args.GetValues("--drive*", options.m_driveFns, error)) {
    cerr << "error: could not parse drive filename list - " << error << endl;
    return -1;
  }
  else if (options.m_driveFns.size() > 0) {
    cout << options.m_driveFns.size() << " drives specified" << endl;
    options.m_withEI = true;
  }

#if 0
  { ' ', "readdebug",   'e', "turn read debugging on or off" /* options.m_readDebug */},
  { ' ', "writedebug",  'e', "turn write debugging on or off" /* options.m_writeDebug */ },
  { ' ', "ei",          'e', "enable/disable Model 1 Expansion Interface" /* options.m_withEI */ }
#endif

  cout << "info: using type '" << options.m_typeName << "'" << endl;  

  // attempt to instantiate emulator
  std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(options.m_typeName));
  if (!emulator) {
    cerr << "error: system type '" << options.m_typeName << "' not found" << endl;
    return -1;
  }

  cout << "info: running " << emulator->GetInfo().m_name << endl;

  const Config::RAM * ram = emulator->GetMainRAMInfo();
  if (ram == nullptr) {
    cerr << "warning: emulator has no RAM defined" << endl;
  }
  else {
    int ramSize_k = ((ram->m_endAddr - ram->m_startAddr) + 1) / 1024; 
    if (options.m_ramSize_k >= 0) {
      emulator->SetRAMSize_k(options.m_ramSize_k);
    }
    else {
      emulator->SetRAMSize_k(ramSize_k);
   }
    cout << "info: RAM size set to " << dec << ramSize_k << "k" << endl;
  }

  // load ROM
  //if (!ReadROMFromFile(options.m_romFn, m_rom))
  //  return false;

  // open and start the emulator
  if (!emulator->Open(options)) {
    cerr << "error: cannot open emulator" << endl;
    return -1;
  }

  MainWindow mainWindow;
  bool hasMemoryMappedVideo = false;
  const Config::Video * videoInfo = emulator->GetVideoInfo();
  if (videoInfo == nullptr) {
    cerr << "error: emulator has no video device defined" << endl;
    return -1;
  }

  // initlialize SDL 
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
    printf("error initializing SDL: %s\n", SDL_GetError()); 
    return -1;
  }

  // get ASCII codes in the keysyms
  //SDL_EnableUNICODE(1);

  cerr << "Creating screen" << endl;

  emulator->CreateScreen(mainWindow, options);

  if (!emulator->Start()) {
    cerr << "error: cannot start emulator" << endl;
    return -1;
  }

  int videoTest = 1;

  if (videoTest) {
    for (int i = 0; i < videoInfo->m_screenCols * videoInfo->m_screenRows; ++i) {
      emulator->m_video->WriteMemoryAtAddress(i, i);
    }
    emulator->m_video->Update(true);
    auto now = std::chrono::system_clock::now();
    auto finish = std::chrono::system_clock::now() + std::chrono::seconds(4);
    while (std::chrono::system_clock::now() < finish) {
      usleep(1000);
      emulator->m_video->Update(false);
    }
  }

  // run emulator
  auto lastPoll  = std::chrono::system_clock::now();
  auto lastSpeed = std::chrono::system_clock::now();

  for (;;) {
    //if (emulator->m_cpu.PC.W == options.m_breakpoint)
    //  emulator->SetTrace(true);

    // give CPU some time
    emulator->Run(500);

    auto now = std::chrono::system_clock::now();

    double interval = std::chrono::duration<double>(now - lastPoll).count();
    if (interval >= 50e-3) {
      if (!emulator->Poll())
        break;
      lastPoll = now;
    }

    interval = std::chrono::duration<double>(now - lastSpeed).count();
    if (interval >= 1) {
      //cout << std::fixed << std::setprecision(3) << (emulator->GetActualCPUSpeed_Hz() / 1e+6) << " MHz" << endl;
      lastSpeed = now;
    }
  }

  // exiting
}
