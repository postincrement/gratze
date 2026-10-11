#include <SDL.h>

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>

#include "common/config.h"
#include "common/misc.h"
#include "common/cmdargs.h"
#include "common/factory.h"
#include "common/cereal.h"

#include "src/mainwindow.h"
#include "src/env_config.h"
#include "src/options.h"

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
  AddEmulator<Microbee128_BN_Emulator>();
  AddEmulator<Microbee128_StarnetClient_Emulator>();
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
  { ' ', "drive*",          's', "disk image for drive a-p (e.g. --drivea file.dsk; legacy --drive0 = A)" },
  { ' ', "cpmdrive",        's', "CP/M drive, A=dir:path or A=image:file,dpb=name" },
  { 'b', "breakpoint",      'x', "breakpoint address"  },
  { ' ', "sdl",             ' ', "use an SDL window for a terminal emulation"},
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
  { ' ', "station",         'u', "Starnet workstation number 0-15"},
  { ' ', "list",            ' ', "list all emulations and environments"},
  { ' ', "gamekb",          ' ', "set keyboard game mode" },

  { 0, 0, 0, 0}
};

static void ApplyCliOverrides(Options & options)
{
  // Only overwrite fields that were explicitly present on the command line.
  options.m_args.GetValue("-t",              options.m_typeName);
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
  {
    unsigned station = 0;
    if (options.m_args.GetValue("--station", station))
      options.m_starnetStation = (int)station;
  }
  options.m_args.GetValue("--displaySpeed",  options.m_displayCPUSpeed);
  options.m_args.GetValue("-f",         options.m_font);
  options.m_args.GetValue("-F",         options.m_fontSize);
  options.m_args.GetValue("--traceLen", options.m_traceLength);
  options.m_args.GetValue("--logpc",    options.m_logPC);
  options.m_args.GetValue("--trace",    options.m_trace);
  options.m_args.GetValue("--sdl",    options.m_useSDL);
  options.m_args.GetValue("--gamekb", options.m_gameKb);

  bool ei = false;
  if (options.m_args.GetValue("--ei", ei))
    options.m_withEI = ei;
}

extern "C"
int main(int argc, char *argv[])
{
  Init();

  Options options;

  int optIndex = options.m_args.Parse(g_commandLineOptions, argc, argv);
  if (optIndex < 0) {
    exit(-1);
  }

  // Verbose may come from CLI before config load.
  options.m_args.GetValue("-v", options.m_verbose);
  if (options.m_verbose)
    cout << options.m_args.DumpValues();

  if (options.m_args.HasArg("--list")) {
    cout << "Available emulations\n";
    ColumnFormatter::Columns columns(3);
    std::vector<std::string> keys;
    g_emulatorFactory.GetKeys(keys);

    for (auto & r : keys) {
      columns[0].push_back(r);
      std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(r));
      const EmulatorInfo & info = emulator->GetInfo();
      columns[1].push_back(info.m_title);
      columns[2].push_back(info.m_sdl ? "SDL" : "console or SDL");
    }

    cout << ColumnFormatter::Print(columns, { "   ", "   ", "   " });

    std::vector<EnvConfigInfo> envs;
    ListEnvConfigs(envs);
    if (!envs.empty()) {
      cout << "\nAvailable environments\n";
      ColumnFormatter::Columns envCols(3);
      for (const auto & env : envs) {
        envCols[0].push_back(env.m_name);
        envCols[1].push_back(env.m_type);
        envCols[2].push_back(env.m_path);
      }
      cout << ColumnFormatter::Print(envCols, { "   ", "   ", "   " });
    }
    exit(0);
  }

  if (options.m_args.HasArg("-h")) {
    cout << "usage: gratze [options] [environment] [args...]\n"
         << "  environment  name of a JSON config in emulator/configs/\n"
         << "where options are:\n"
         << options.m_args.Usage();
    return 0;
  }

  std::string fn;
  if (options.m_args.GetValue("--diskette", fn)) {
    VirtualFileIdentifier fileId;
    std::shared_ptr<VirtualDrive> file = fileId.Open(fn, true);
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

  {
    int opt = optIndex;
    while (opt < argc) {
      options.m_arg.push_back(argv[opt++]);
    }
  }

  // Named environment: first positional arg, unless -t was given and the
  // name does not resolve (then keep it as a program argument).
  bool loadedEnv = false;
  if (!options.m_arg.empty()) {
    std::string envName = options.m_arg.front();
    std::string envError;
    std::string envPath = FindEnvConfigPath(envName, envError);
    if (!envPath.empty()) {
      if (!LoadEnvConfig(envName, options, envError)) {
        cerr << "error: " << envError << endl;
        return -1;
      }
      options.m_arg.erase(options.m_arg.begin());
      loadedEnv = true;
      if (options.m_verbose)
        cout << "info: loaded environment '" << envName << "' from " << options.m_envConfigPath << endl;
    }
    else if (!options.m_args.HasArg("-t") && !options.m_args.HasArg("--type")) {
      // Bare name that is not a config and no -t: report the lookup error
      // only when it looks like an environment name (no path/extension).
      bool looksLikeFile = envName.find('/') != std::string::npos
                        || envName.find('\\') != std::string::npos
                        || envName.find('.') != std::string::npos;
      if (!looksLikeFile) {
        cerr << "error: " << envError << endl;
        return -1;
      }
    }
  }

  // CLI flags override the environment (and supply defaults when none loaded).
  if (!loadedEnv && !options.m_args.HasArg("-t") && !options.m_args.HasArg("--type"))
    options.m_typeName = "m1";
  ApplyCliOverrides(options);

  std::string error;
  std::map<unsigned, std::string> cliDriveFns;
  if (!options.m_args.GetValues("--drive*", cliDriveFns, error)) {
    cerr << "error: could not parse drive list - " << error << endl;
    return -1;
  }
  std::vector<std::string> cliCpmDrives;
  if (!options.m_args.GetValues("--cpmdrive", cliCpmDrives)) {
    cerr << "error: could not parse --cpmdrive" << endl;
    return -1;
  }

  if (!MaterializeDrives(options, error)) {
    cerr << "error: " << error << endl;
    return -1;
  }

  // Explicit CLI drives override / extend materialized config drives.
  for (const auto & r : cliDriveFns)
    options.m_driveFns[r.first] = r.second;
  if (!cliCpmDrives.empty())
    options.m_cpmDrives = cliCpmDrives;

  if (options.m_driveFns.size() > 0) {
    cout << options.m_driveFns.size() << " drives specified" << endl;
    options.m_withEI = true;
  }

  if (options.m_verbose) {
    cout << "info: backtrace queue is " << options.m_traceLength << endl;
    cout << "info: using type '" << options.m_typeName << "'" << endl;
    cout << "info: video scale is " << options.m_videoScale << endl;
  }

  std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(options.m_typeName));
  if (!emulator) {
    cerr << "error: system type '" << options.m_typeName << "' not found" << endl;
    return -1;
  }

  if (options.m_verbose) {
    cout << "info: running " << emulator->GetInfo().m_name << endl;
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

    if (options.m_verbose)
      cout << "info: RAM size set to " << dec << ramSize_k << "k" << endl;
  }

  return emulator->Run(options);
}
