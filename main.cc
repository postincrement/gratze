#include <SDL2/SDL.h> 

#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "config.h"

#include "model1.h"
#include "model3.h"
#include "model4.h"


using namespace std;

template<class Abstract, class KeyType = std::string>
class Factory
{
  public:
    Factory()
    { }

    struct Worker 
    {
      virtual Abstract * CreateInstance() = 0;
    };

    typedef std::map<KeyType, Worker *> WorkerMap;

    template <class Concrete>
    struct ConcreteWorker : public Worker
    {
      Abstract * CreateInstance() override
      { return new Concrete(); }
    };

    template <class Concrete>
    void AddWorker(const KeyType & key)
    { m_workers[key] = new ConcreteWorker<Concrete>(); }

    Abstract * CreateInstance(const KeyType & key)
    { 
      typename WorkerMap::iterator r = m_workers.find(key);
      if (r == m_workers.end())
        return NULL;
      return r->second->CreateInstance();
    }

    size_t GetKeys(std::vector<KeyType> & keys)
    {
      keys.clear();
      for (auto & r : m_workers)
        keys.push_back(r.first);
      return keys.size();  
    }

  protected:  
    WorkerMap m_workers;
};

using EmulatorFactory = Factory<Emulator, std::string>;
static EmulatorFactory g_emulatorFactory;

/////////////////////////////////////////////////////

extern "C"
int main(int argc, char *argv[]) 
{
  g_emulatorFactory.AddWorker<Model1Level2_Emulator>("m1");
  g_emulatorFactory.AddWorker<Model1Level1_Emulator>("m11");
  g_emulatorFactory.AddWorker<Model1Level2_Emulator>("m12");
  g_emulatorFactory.AddWorker<Model3_Emulator>("m3");
  g_emulatorFactory.AddWorker<Model4_Emulator>("m4");

  Options options;
  options.m_ramSize_k = -1;

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

    // select ROM file
    if ((option == "rom") || (option == "r")) {
      if (++optIndex >= argc) {
        cerr << "error: --rom option requires filename argument" << endl;
        return -1;
      }
      options.m_romFn = argv[optIndex++];
    }

    // set RAM size
    if (option == "ram") {
      if (++optIndex >= argc) {
        cerr << "error: --ram option requires size in k" << endl;
        return -1;
      }
      options.m_ramSize_k = atoi(argv[optIndex++]);
    }

    // select drives
    else if ((option.substr(0, 5) == "drive")) {
      std::string driveNumStr(option.substr(5));
      int driveNum = atoi(driveNumStr.c_str());
      if (++optIndex >= argc) {
        cerr << "error: --drivex option requires filename argument" << endl;
        return -1;
      }
      options.m_driveFns[driveNum] = std::string(argv[optIndex++]);
    }

    // breakpoint
    else if ((option == "b") || (option == "breakpoint")) {
      if (++optIndex >= argc) {
        cerr << "error: --breakpoint option requires address argument" << endl;
        return -1;
      }
      std::string arg(argv[optIndex++]);
      int addr = strtoul(arg.c_str(), NULL, 16);
      options.m_breakpoint = addr;
    }

    // diskette
    else if (option == "diskette") {
      if (++optIndex >= argc) {
        cerr << "error: --diskette option requires filename argument" << endl;
        return -1;
      }
      std::string arg(argv[optIndex++]);

      VirtualDriveFile file;
      if (!file.Open(arg, true)) {
        cerr << "error: could not open diskette file '" << arg << "'" << endl;
        return -1;
      }
      cerr << "file '" << arg << "' opened" << endl;
      return 0;
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

      return 0;
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

      // if option not found, show error
      if (!options.m_typeName.empty()) {
        cout << "info: selected type " << option << endl;
      }
      else {
        cerr << "error: unknown option '" << option << "'" << endl;
        return -1;
      }
    }
  }

  if (options.m_typeName.empty())
    options.m_typeName  = "m1";

  // attempt to instantiate emulator
  std::unique_ptr<Emulator> emulator(g_emulatorFactory.CreateInstance(options.m_typeName));
  if (!emulator) {
    cerr << "error: system type '" << options.m_typeName << "' not found" << endl;
    return -1;
  }

  cout << "info: running " << emulator->GetTitle() << endl;

  if (options.m_ramSize_k >= 0) {
    emulator->SetRAMSize_k(options.m_ramSize_k);
  }
  else {
    emulator->SetRAMSize_k(emulator->GetDefaultRAMSize_k());
  }

  cout << "info: RAM size set to " << emulator->GetRAMSize_k() << "k" << endl;

  // load ROM
  //if (!ReadROMFromFile(options.m_romFn, m_rom))
  //  return false;

  // returns zero on success else non-zero 
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
    printf("error initializing SDL: %s\n", SDL_GetError()); 
    return -1;
  }

  // open and start the emulator
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
