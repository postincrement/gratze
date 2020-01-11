#include <SDL.h> 

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#include "config.h"
#include "misc.h"
#include "mainwindow.h"

#include "trs80/model1/model1.h"
#include "trs80/model3/model3.h"
#include "trs80/model4/model4.h"
#include "dg680/dg680.h"
#include "super80/super80.h"
#include "2650/binbug/binbug.h"

#include "factory.h"


using namespace std;

using EmulatorFactory = Factory<Emulator, std::string>;
static EmulatorFactory g_emulatorFactory;

/////////////////////////////////////////////////////

bool ParseOptions(int argc, char *argv[], Options & options)
{
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

    std::string option(arg.substr(1, 1));
    if (arg[0] == '-') {
      // solitary "--" terminates options
      if (len == 2) {
        optIndex++;
        break;
      }
      option = arg.substr(2);
    }

    cout << "matching " << option << endl;

    // select ROM file
    if ((option == "rom") || (option == "r")) {
      if (++optIndex >= argc) {
        cerr << "error: --rom option requires filename argument" << endl;
        return false;
      }
      options.m_romFn = argv[optIndex++];
    }

    // set RAM size
    if (option == "ram") {
      if (++optIndex >= argc) {
        cerr << "error: --ram option requires size in k" << endl;
        return false;
      }
      options.m_ramSize_k = atoi(argv[optIndex++]);
    }

    // select drives
    else if ((option.substr(0, 5) == "drive")) {
      std::string driveNumStr(option.substr(5));
      int driveNum = atoi(driveNumStr.c_str());
      if (++optIndex >= argc) {
        cerr << "error: --drivex option requires filename argument" << endl;
        return false;
      }
      options.m_driveFns[driveNum] = std::string(argv[optIndex++]);
      cerr << "file '" << arg << "' opened for drive " << driveNum << endl;
      options.m_withEI = true;
    }

    // breakpoint
    else if ((option == "b") || (option == "breakpoint")) {
      if (++optIndex >= argc) {
        cerr << "error: --breakpoint option requires address argument" << endl;
        return false;
      }
      std::string arg(argv[optIndex++]);
      int addr = strtoul(arg.c_str(), NULL, 16);
      options.m_breakpoint = addr;
    }

    // font
    else if ((option == "f") || (option == "font")) {
      if (++optIndex >= argc) {
        cerr << "error: --font option requires address argument" << endl;
        return false;
      }
      std::string arg(argv[optIndex++]);
      options.m_font = arg;
    }

    // breakpoint
    else if ((option == "s") || (option == "fontSize")) {
      if (++optIndex >= argc) {
        cerr << "error: --fontSize option requires address argument" << endl;
        return false;
      }
      std::string arg(argv[optIndex++]);
      int num = strtoul(arg.c_str(), NULL, 10);
      options.m_fontSize = num;
    }

    // diskette
    else if (option == "diskette") {
      if (++optIndex >= argc) {
        cerr << "error: --diskette option requires filename argument" << endl;
        return false;
      }
      std::string arg(argv[optIndex++]);

      VirtualDriveFile file;
      if (!file.Open(arg, true)) {
        cerr << "error: could not open diskette file '" << arg << "'" << endl;
        return false;
      }
      cerr << "file '" << arg << "' opened" << endl;
      return false;
    }

    // cassette
    else if (option == "cassette") {
      if (++optIndex >= argc) {
        cerr << "error: --cassette option requires filename argument" << endl;
        return -1;
      }
      std::string arg(argv[optIndex++]);

      VirtualCassetteFile file;
      if (!file.ReadOpen(arg)) {
        cerr << "error: could not open cassette file '" << arg << "'" << endl;
        return -1;
      }
      cout << "info: file '" << arg << "' opened, internal filename is " << file.GetFilename() << endl;

      return false;
    }

    else {
      // look for model option
      std::vector<std::string> keys;
      size_t count = g_emulatorFactory.GetKeys(keys);
      for (auto & r : keys) {
        if (option == r) {
          options.m_typeName = r;
          optIndex++;
          break;
        }
      }

      // if mode option not found, display it
      if (!options.m_typeName.empty()) {
        cout << "info: selected type " << option << endl;
      }

      // look for enable/disable options
      else {
        std:string enableOpt;
        bool on = false;
        if ((option.length() > 7) && option.substr(0, 7) == "enable-") {
          enableOpt = option.substr(7);
          on = true;
        }
        else if ((option.length() > 8) && option.substr(0, 8) == "disable-") {
          enableOpt = option.substr(8);
          on = false;
        }
        if (!enableOpt.empty()) {
          for (auto & r : enableOpt) r = tolower(r); 

          // enable/disable EI
          if (enableOpt == "ei") {
            options.m_withEI = on;
            optIndex++;
          }

          // enable/disable EI
          else if (enableOpt == "readdebug") {
            options.m_readDebug = on;
            optIndex++;
          }

          // enable/disable EI
          else if (enableOpt == "writedebug") {
            options.m_writeDebug = on;
            optIndex++;
          }

          // unknown option
          else {
            enableOpt.clear();
          }
        }
        if (enableOpt.empty()) {
          cerr << "error: unknown option '" << option << "'" << endl;
          return false;
        }
      }
    }
  }

  return true;
}

/////////////////////////////////////////////////////

void Init()
{
  g_emulatorFactory.AddConcreteClass<Model1Level2_Emulator>("m1");
  g_emulatorFactory.AddConcreteClass<Model1Level1_Emulator>("m11");
  g_emulatorFactory.AddConcreteClass<Model1Level2_Emulator>("m12");
  g_emulatorFactory.AddConcreteClass<Model3_Emulator>("m3");
  g_emulatorFactory.AddConcreteClass<Model4_Emulator>("m4");
  g_emulatorFactory.AddConcreteClass<DG680_Emulator>("dg680");
  g_emulatorFactory.AddConcreteClass<BINBUG_2650>("binbug");
  g_emulatorFactory.AddConcreteClass<Super80_Emulator>("super80");

  std::vector<std::string> keys;
  g_emulatorFactory.GetKeys(keys);

  for (auto & r : keys) {
    std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
    emulator->Init();
  }
}


extern "C"
int main(int argc, char *argv[]) 
{
  Init();

  Options options;

  if (!ParseOptions(argc, argv, options)) {
    return -1;
  }

  // set default type
  if (options.m_typeName.empty())
    options.m_typeName  = "m1";

  // make sure font size is set
  if (options.m_font.empty() && (options.m_fontSize < 0))
    options.m_fontSize = 20; 

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
      emulator->m_video->WriteMemoryAtAddress(i, i & 0xff);
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
