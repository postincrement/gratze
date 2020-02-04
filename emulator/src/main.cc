#include <SDL.h>

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#include "src/config.h"
#include "common/misc.h"
#include "src/mainwindow.h"
#include "common/cmdargs.h"

#include "z80/trs80/model1/model1.h"
#include "z80/trs80/model3/model3.h"
#include "z80/trs80/model4/model4.h"
#include "z80/dg680/dg680.h"
#include "z80/super80/super80.h"
#include "z80/microbee/microbee.h"
#include "2650/eti685/eti685.h"
#include "2650/78up5/78up5.h"

#include "common/factory.h"
#include "src/cereal.h"


using namespace std;

using EmulatorFactory = Factory<Emulator, std::string>;
static EmulatorFactory g_emulatorFactory;

/////////////////////////////////////////////////////


template <class Type>
void AddEmulator()
{
  std::unique_ptr<Emulator> emulator(new Type());
  const EmulatorInfo & info = emulator->GetInfo();
  std::string key = info.m_option;
  g_emulatorFactory.AddConcreteClass<Type>(key);
}

void Init()
{
  AddEmulator<Model1Level1_Emulator>();
  AddEmulator<Model1Level2_Emulator>();
  AddEmulator<Model3_Emulator>();
  AddEmulator<Model4_Emulator>();
  AddEmulator<DG680_Emulator>();
  AddEmulator<ETI685>();
  AddEmulator<Super80_Emulator>();
  AddEmulator<Microbee_Emulator>();
  AddEmulator<EA78UP5_PIPBUG_110>();
  AddEmulator<EA78UP5_PIPBUG_300>();

  std::vector<std::string> keys;
  g_emulatorFactory.GetKeys(keys);

  for (auto & r : keys) {
    std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
    emulator->Instantiate();
  }
}

static CommandLineArgs::Option g_commandLineOptions[] = {
  { 'h', "help",        ' ', "display this help message" },
  { 'r', "rom",         's', "name of ROM"    },
  { ' ', "ram",         'u', "RAM size in k" },
  { ' ', "drive*",      's', "name of file for virtual disk drive" },
  { 'b', "breakpoint",  'x', "breakpoint address"  },
  { 'f', "font",        's', "use TTF font"},
  { 'F', "fontSize",    'u', "TTF font size" },
  { 's', "scale",       'u', "Screen scale factor" },
  { ' ', "diskette",    's', "test virtual drive file"},
  { ' ', "cassette",    's', "test virtual cassette file"},
  { 't', "type",        's', "look for model" },
  { ' ', "readdebug",   'b', "turn read debugging on or off" },
  { ' ', "writedebug",  'b', "turn write debugging on or off" },
  { ' ', "ei",          'b', "enable/disable Model 1 Expansion Interface" },
  { 'v', "verbose",     '+', "enable verbose logging" },
  { ' ', "videotest",   ' ', "display video test before starting" },
  { ' ', "displaySpeed",  ' ', "display CPU speed on console"},
  { ' ', "keyboardDebug", ' ', "display keyboard debug on console" },
  { ' ', "turbo",         ' ', "do not throttle CPU speed"},
  { ' ', "list",          ' ', "list all emulations"},
  { 0, 0, 0, 0}
};

extern "C"
int main(int argc, char *argv[])
{
  Init();

  Options options;

  if (!options.m_args.Parse(g_commandLineOptions, argc, argv)) {
    return -1;
  }

  cout << options.m_args.DumpValues();

  if (options.m_args.HasArg("--list")) {
    cout << "Available emulations\n";
    ColumnFormatter::Columns<2> columns;
    std::vector<std::string> keys;
    g_emulatorFactory.GetKeys(keys);

    for (auto & r : keys) {
      columns[0].push_back(r);
      std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
      const EmulatorInfo & info = emulator->GetInfo();
      columns[1].push_back(info.m_title);
    }

    cout << ColumnFormatter::Print<2>(columns, { "   ", "   " });
    exit(0);
  }

  if (options.m_args.HasArg("-h")) {
    cout << "usage: gratze [options] args...\n"
         << "where options are:\n"
        << options.m_args.Usage();
    return 0;
  }

  std::string fn;
  if (options.m_args.GetValue("--diskette", fn)) {
    VirtualDriveFile file;
    if (!file.Open(fn, true)) {
      cerr << "error: could not open diskette file '" << fn << "'" << endl;
      return -1;
    }
    cerr << "file '" << fn << "' opened" << endl;
    return 0;
  }

  if (options.m_args.GetValue("--cassette", fn)) {
    VirtualCassetteFile file;
    if (!file.ReadOpen(fn)) {
      cerr << "error: could not open cassette file '" << fn << "'" << endl;
      return -1;
    }
    cout << "info: file '" << fn << "' opened, internal filename is " << file.GetFilename() << endl;
    return 0;
  }

  // set default type
  if (!options.m_args.GetValue("-t", options.m_typeName))
    options.m_typeName = "m1";

  options.m_args.GetValue("-r",           options.m_romFn);
  options.m_args.GetValue("--ram",        options.m_ramSize_k);
  options.m_args.GetValue("-s",           options.m_videoScale);
  options.m_args.GetValue("-v",           options.m_verbose);
  options.m_args.GetValue("--readdebug",  options.m_readDebug);
  options.m_args.GetValue("--writedebug", options.m_writeDebug);
  options.m_args.GetValue("--keyboardDebug", options.m_keyboardDebug);
  options.m_args.GetValue("--turbo",         options.m_turbo);

  options.m_args.GetValue("-f", options.m_font);
  options.m_args.GetValue("-F", options.m_fontSize);

  cout << "info: using type '" << options.m_typeName << "'" << endl;

  // attempt to instantiate emulator
  std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(options.m_typeName));
  if (!emulator) {
    cerr << "error: system type '" << options.m_typeName << "' not found" << endl;
    return -1;
  }

  cout << "info: running " << emulator->GetInfo().m_name << endl;

  std::string error;
  if (!options.m_args.GetValues("--drive*", options.m_driveFns, error)) {
    cerr << "error: could not parse drive filename list - " << error << endl;
    return -1;
  }
  else if (options.m_driveFns.size() > 0) {
    cout << options.m_driveFns.size() << " drives specified" << endl;
    options.m_withEI = true;
  }

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

  return emulator->Run(options);
}

