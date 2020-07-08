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
#include "z80/sorcerer/sorcerer.h"
#include "z80/cpm80/cpm80.h"
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
  AddEmulator<Microbee32_Emulator>();
  AddEmulator<Microbee56_Emulator>();
  AddEmulator<Microbee128_Emulator>();
  AddEmulator<Sorcerer_Emulator>();
  AddEmulator<EA78UP5_PIPBUG_110>();
  AddEmulator<EA78UP5_PIPBUG_300>();
  AddEmulator<CPM80_Emulator>();

  std::vector<std::string> keys;
  g_emulatorFactory.GetKeys(keys);

  for (auto & r : keys) {
    std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
    emulator->Instantiate();
  }

  VirtualDrive::Init();
}

static CommandLineArgs::Option g_commandLineOptions[] = {
  { 'h', "help",            ' ', "display this help message" },
  { 'r', "rom",             's', "name of ROM"    },
  { ' ', "ram",             'u', "RAM size in k" },
  { ' ', "drive*",          's', "name of file for virtual disk drive" },
  { 'b', "breakpoint",      'x', "breakpoint address"  },
  { 'f', "font",            's', "use TTF font"},
  { 'F', "fontSize",        'u', "TTF font size" },
  { 's', "scale",           'u', "Screen scale factor" },
  { ' ', "diskette",        's', "test virtual drive file"},
  { ' ', "cassette",        's', "test virtual cassette file"},
  { 't', "type",            's', "look for model" },
  { ' ', "readmemdebug",    ' ', "turn on memory read debugging" },
  { ' ', "writememdebug",   ' ', "turn on memory write debugging" },
  { ' ', "readvideodebug",  ' ', "turn on video read debugging" },
  { ' ', "writevideodebug", ' ', "turn on memory write debugging" },
  { ' ', "readiodebug",     ' ', "turn on I/O read debugging" },
  { ' ', "writeiodebug",    ' ', "turn on I/O write debugging" },
  { ' ', "trace",           ' ', "turn on tracing" },
  { ' ', "logpc",           ' ', "turn on logging of PC" },
  { ' ', "traceLen",        'u', "set length of backtrace queue" },
  { ' ', "ei",              'b', "enable/disable Model 1 Expansion Interface" },
  { 'v', "verbose",         '+', "enable verbose logging" },
  { ' ', "videotest",       ' ', "display video test before starting" },
  { ' ', "displaySpeed",    ' ', "display CPU speed on console"},
  { ' ', "keyboardDebug",   ' ', "display keyboard debug on console" },
  { ' ', "fdcDebug",        ' ', "display FDC debug on console" },
  { ' ', "turbo",           ' ', "do not throttle CPU speed"},
  { ' ', "list",            ' ', "list all emulations"},

  { ' ', "delay",           'u', "manual CPU delay"},
  { ' ', "kp",              'f', "Kp for CPU delay control"},
  { ' ', "ki",              'f', "Ki for CPU delay control"},

  { 0, 0, 0, 0}
};

extern "C"
int main(int argc, char *argv[])
{
  Init();

  Options options;

  int optIndex = options.m_args.Parse(g_commandLineOptions, argc, argv);
  if (optIndex < 0) {
    exit(-1);
  }

  cout << options.m_args.DumpValues();

  if (options.m_args.HasArg("--list")) {
    cout << "Available emulations\n";
    ColumnFormatter::Columns columns(2);
    std::vector<std::string> keys;
    g_emulatorFactory.GetKeys(keys);

    for (auto & r : keys) {
      columns[0].push_back(r);
      std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
      const EmulatorInfo & info = emulator->GetInfo();
      columns[1].push_back(info.m_title);
    }

    cout << ColumnFormatter::Print(columns, { "   ", "   " });
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
    VirtualFileIdentifier fileId;
    VirtualDrive * file = fileId.Open(fn, true);
    if (file == nullptr) {
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

  options.m_args.GetValue("-r",              options.m_romFn);
  options.m_args.GetValue("--ram",           options.m_ramSize_k);
  options.m_args.GetValue("-s",              options.m_videoScale);
  options.m_args.GetValue("-v",              options.m_verbose);
  options.m_args.GetValue("--readiodebug",   options.m_readIO);
  options.m_args.GetValue("--writeiodebug",  options.m_writeIO);
  options.m_args.GetValue("--readmemdebug",  options.m_readMemory);
  options.m_args.GetValue("--writememdebug", options.m_writeMemory);
  options.m_args.GetValue("--readvideodebug",     options.m_readVideo);
  options.m_args.GetValue("--writevideodebug",    options.m_writeVideo);
  options.m_args.GetValue("--keyboardDebug", options.m_keyboardDebug);
  options.m_args.GetValue("--fdcDebug",      options.m_fdcDebug);
  options.m_args.GetValue("--turbo",         options.m_turbo);
  options.m_args.GetValue("--displaySpeed",  options.m_displayCPUSpeed);

  options.m_args.GetValue("-f",         options.m_font);
  options.m_args.GetValue("-F",         options.m_fontSize);
  options.m_args.GetValue("--traceLen", options.m_traceLength);
  options.m_args.GetValue("--logpc",    options.m_logPC);
  options.m_args.GetValue("--trace",    options.m_trace);

  options.m_args.GetValue("--delay", options.m_delay);
  options.m_args.GetValue("--kp",    options.m_Kp);
  options.m_args.GetValue("--ki",    options.m_Ki);

  {
    int opt = optIndex;
    while (opt < argc) {
      options.m_arg.push_back(argv[opt++]);
    }
  }

  cout << "info: backtrace queue is " << options.m_traceLength << endl;

  cout << "info: using type '" << options.m_typeName << "'" << endl;

  cout << "info: video scale is " << options.m_videoScale << endl;
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

