#include <memory.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <time.h>
#include <functional>
#include <math.h>

#include "common/misc.h"
#include "src/config.h"
#include "src/emulator.h"
#include "devices/fdc.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"
#include "terminal/terminal.h"
#include "src/grz.h"

extern "C" {
#include "nfd.h"
};

using namespace std;

#define TRACE_SYM   SDLK_F2
#define COLOUR_SYM  SDLK_F9
#define DUMP_SYM    SDLK_F10
#define GRZ_SYM     SDLK_F11
#define REBOOT_SYM  SDLK_F12


/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator(const EmulatorInfo * info)
  : m_info(info)
{
}

Emulator::~Emulator()
{}

void Emulator::Instantiate()
{
}

const EmulatorInfo & Emulator::GetInfo() const
{
  return *m_info;
}

bool Emulator::Open(const Options & options)
{
  return true;
}

void Emulator::Reset(int addr)
{
  m_pollers.m_list.clear();

  using namespace std::placeholders;
  AddRealTimePollDef(1.0,   std::bind(&Emulator::CalcCPUSpeed,  this, _1, _2));
  AddRealTimePollDef(0.1,   std::bind(&Emulator::CheckKeyboard, this));
  AddCPUTimePollDef(100000, std::bind(&Emulator::UpdateScreen,  this));
}

bool Emulator::SetRAMSize_k(int len)
{
  //m_ram.resize(len * 1024);
  m_ramSize_bytes = len * 1024; //m_ram.size();
  m_ramMask = (m_ramSize_bytes - 1);

  cout << "info: RAM size " << len << " k, " << m_ramSize_bytes << " bytes, " << HEXFORMAT0x4(m_ramMask) << endl;
  return true;
}

int Emulator::GetRAMSize_k() const
{
  return m_ramSize_bytes / 1024;
}

double Emulator::GetActualCPUSpeed_Hz() const
{
  return m_actualCPUClock_Hz;
}

void Emulator::UpdateScreen()
{
  if (m_screen)
    m_screen->Update(false);
}

void Emulator::CheckKeyboard()
{
  SDL_Event event;
  if (SDL_PollEvent(&event)) {
    switch (event.type) {

      case SDL_QUIT:
        return;
        break;

      case SDL_KEYDOWN:
        if (event.key.repeat == 0) {
          if (event.key.keysym.sym == TRACE_SYM)
            SetTrace(true);

          if (event.key.keysym.sym == TRACE_SYM + 1)
            SetTrace(false);

          if (event.key.keysym.sym == COLOUR_SYM) {
            ChangeVideoColour();
          }
          else if (event.key.keysym.sym == DUMP_SYM) {
            MemoryDump();
          }
          else if (event.key.keysym.sym == GRZ_SYM) {
            LoadGRZ();
          }
          else if (event.key.keysym.sym == REBOOT_SYM) {
            Reset();
          }
          else if (m_keyboard != nullptr) {
            if (m_keyboardDebug) {
              cerr << "debug: key down " << HEXFORMAT0x8(event.key.keysym.sym) << endl;
            }
            m_keyboard->OnKeyDown(event.key.keysym);
          }
          else {
            OnKeyDown(event.key.keysym);
          }
        }
        break;

      case SDL_KEYUP:
        if (event.key.repeat == 0) {
          if (m_keyboard != nullptr) {
            if (m_keyboardDebug) {
              cerr << "debug: key up " << HEXFORMAT0x8(event.key.keysym.sym) << endl;
            }
            m_keyboard->OnKeyUp(event.key.keysym);
          }
          else {
            OnKeyUp(event.key.keysym);
          }
        }
        break;

      default:
        break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::SetKeyboard(VirtualKeyboard * kb)
{
  m_keyboard.reset(kb);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CreateScreen(MainWindow & mainWindow, const Options & options)
{
  int top = 10;
  int left = 10;                // border top and left

  int vdup = 1;                 // duplicate lines for fields

  int pixelCols;
  int pixelRows;

  int width;
  int height;

  // get size of screen
  SDL_DisplayMode mode;
  SDL_GetDesktopDisplayMode(0, &mode);
  cout << "info: screen is " << mode.w << "x" << mode.h << endl;

  // check for memory mapped screens
  const Config::MemoryMappedScreen * mmapScreenInfo = GetMemoryMappedInfo();
  if (mmapScreenInfo != nullptr) {

    cout << "info: emulated screen is memory mapped" << endl;

    const Config::Block * block = GetConfigBlock(Config::Type::eMonitor);
    const Config::Monitor * monitor = (block == nullptr) ? nullptr : &block->m_info.m_monitor;

    // display video output pixels
    pixelCols = mmapScreenInfo->m_screenWidth;
    pixelRows = mmapScreenInfo->m_screenHeight;

    width  = pixelCols * options.m_videoScale;
    height = pixelRows * options.m_videoScale;

    // create main window
    mainWindow.Open(width, height);

    m_memMapScreen.reset(MemoryMappedScreen::Create(mainWindow, options, *mmapScreenInfo));
    m_screen = m_memMapScreen;

    if (!options.m_font.empty()) {
      m_memMapScreen->SetFont(new TTFFont(mmapScreenInfo->m_font, 128, options.m_font, options.m_fontSize));
    }
    else {
      m_memMapScreen->SetFont(new PixelFont(mmapScreenInfo->m_font));
    }
  }

  // check for terminals
  else {
    const Config::Block * block = GetConfigBlock(Config::Type::eTerminal);
    if (block == nullptr) {
      cerr << "error: no screen or terminal defined" << endl;
      exit(-1);
    }

    // create main window with a guess at the size
    mainWindow.Open(800 * options.m_videoScale, 600 * options.m_videoScale);

    const Config::Terminal & termInfo = block->m_info.m_terminal;

    m_terminal.reset(new Terminal(mainWindow, options, termInfo.m_cols, termInfo.m_rows));
    m_screen   = m_terminal->m_screen;
    m_keyboard = m_terminal->m_keyboard;

    std::string fontName = options.m_font;
    if (fontName.empty())
      fontName = DEFAULT_TTF_FONT;

    int fontSize = options.m_fontSize;
    if (fontSize <= 0)
      fontSize = 15;

    m_screen->SetFont(new TTFFont(fontName, fontSize));
  }

  if (!m_screen) {
    cerr << "error: could not instantiate screen" << endl;
    return; // false;
  }

  m_screen->SetScale(options.m_videoScale, options.m_videoScale);
  m_screen->Open();
}

void Emulator::WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  if (addr > info.m_endAddr)
    cerr << "warning: bad video write" << endl;
  else
    m_memMapScreen->WriteMemoryAtAddress(addr - info.m_startAddr, data);
}

uint8_t Emulator::ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  if (addr > info.m_endAddr) {
    cerr << "warning: bad video read" << endl;
    return 0x00;
  }
  else
    return m_memMapScreen->ReadMemoryAtAddress(addr - info.m_startAddr);
}

void Emulator::ChangeVideoColour()
{
  SDL_Color newFg;
  SDL_Color newBg;

  m_memMapScreen->GetFontColour(newFg, newBg);

  newFg.r ^= 0xff;
  newFg.b ^= 0xff;

  m_memMapScreen->SetFontColour(newFg, newBg);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::OnKeyDown(const SDL_Keysym &keysym)
{
  cerr << "warning: emulator got key down sym code " << HEXFORMAT0x2(keysym.sym) << endl;
}

void Emulator::OnKeyUp(const SDL_Keysym &keysym)
{
  cerr << "warning: emulator got key up sym code " << HEXFORMAT0x2(keysym.sym) << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

const Config::Block * Emulator::GetConfigBlock(Config::Type type) const
{
  for (auto & block : m_info->m_blocks) {
    if (block.m_type == Config::Type::eEnd)
      break;
    if (block.m_type == type)
      return &block;
  }
  return nullptr;
}

const Config::CPU * Emulator::GetCPUInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eCPU);
  return (info == nullptr) ? nullptr : &info->m_info.m_cpu;
}

const Config::MemoryMappedScreen * Emulator::GetMemoryMappedInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eMemoryMappedScreen);
  return (info == nullptr) ? nullptr : &info->m_info.m_memoryMappedScreen;
}

const Config::RAM * Emulator::GetMainRAMInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eMainRAM);
  return (info == nullptr) ? nullptr : &info->m_info.m_ram;
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CompileConfigBlocks()
{
  m_readMemoryBlocks.clear();
  m_writeMemoryBlocks.clear();
  m_readIOPortBlocks.clear();
  m_writeIOPortBlocks.clear();

  if (m_verbose) {
    cout << "debug: compiling config blocks" << endl;
  }

  for (const auto & block : m_info->m_blocks) {

    if (block.m_type == Config::Type::eEnd)
      break;

    // add RAM
    if ((block.m_type == Config::Type::eRAM) || (block.m_type == Config::Type::eMainRAM)) {
      const Config::RAM & info = block.m_info.m_ram;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: RAM block has end address" << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      if (m_verbose) {
        cout << "debug: RAM block" << endl;
      }
      uint8_t * memory;
      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block.m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        memory = (uint8_t *)malloc(writeInfo.m_endAddr - writeInfo.m_startAddr + 1);
        writeInfo.m_memory = memory;
        if (m_debugWriteMemory) {
          writeInfo.m_function  = &Emulator::DebugWriteMemory;
        }
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block.m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_memory    = memory;
        if (m_debugReadMemory) {
          readInfo.m_function  = &Emulator::DebugReadMemory;
        }
        m_readMemoryBlocks.push_back(readInfo);
      }
    }

    // add ROM
    else if (block.m_type == Config::Type::eROM) {
      if (m_verbose) {
        cout << "debug: ROM block" << endl;
      }
      const Config::ROM & info = block.m_info.m_rom;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: ROM block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type      = block.m_type;
      readInfo.m_startAddr = info.m_startAddr;
      readInfo.m_endAddr   = info.m_endAddr;
      readInfo.m_memory    = info.m_data;
      if (m_debugReadMemory) {
        readInfo.m_function  = &Emulator::DebugReadMemory;
      }
      m_readMemoryBlocks.push_back(readInfo);
    }

    // add read memIO
    else if (block.m_type == Config::Type::eMemIORead) {
      const Config::MemIO & info = block.m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO read block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type         = block.m_type;
      readInfo.m_startAddr    = info.m_startAddr;
      readInfo.m_endAddr      = info.m_endAddr;
      readInfo.m_realFunction = &Emulator::ReadIOMemoryInternal;
      if (m_debugReadMemory) {
        readInfo.m_function  = &Emulator::DebugReadIOMemory;
      }
      else {
        readInfo.m_function  = &Emulator::ReadIOMemoryInternal;
      }
      readInfo.m_id        = info.m_id;
      m_readMemoryBlocks.push_back(readInfo);
    }

    // add write memIO
    else if (block.m_type == Config::Type::eMemIOWrite) {
      const Config::MemIO & info = block.m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO write block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      WriteMemoryBlockInfo writeInfo;
      writeInfo.m_type         = block.m_type;
      writeInfo.m_startAddr    = info.m_startAddr;
      writeInfo.m_endAddr      = info.m_endAddr;
      writeInfo.m_realFunction = &Emulator::WriteIOMemoryInternal;
      if (m_debugWriteMemory) {
        writeInfo.m_function  = &Emulator::DebugWriteIOMemory;
      }
      else {
        writeInfo.m_function  = &Emulator::WriteIOMemoryInternal;
      }
      writeInfo.m_id        = info.m_id;
      m_writeMemoryBlocks.push_back(writeInfo);
    }

    // add video
    else if (block.m_type == Config::Type::eMemoryMappedScreen) {
      const Config::MemoryMappedScreen & info = block.m_info.m_memoryMappedScreen;
      if (info.m_variable)
        continue;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: video block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }

      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block.m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        writeInfo.m_realFunction = &Emulator::WriteToVideo;
        writeInfo.m_function     = &Emulator::WriteToVideo;
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block.m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_realFunction = &Emulator::ReadFromVideo;
        readInfo.m_function     = &Emulator::ReadFromVideo;
        m_readMemoryBlocks.push_back(readInfo);
      }
    }

    // add read block
    if ((block.m_type == Config::Type::eIOPortRead) || (block.m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block.m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: read IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      ReadIOPortBlockInfo readInfo;
      readInfo.m_type      = block.m_type;
      readInfo.m_startPort = info.m_startPort;
      readInfo.m_endPort   = info.m_endPort;
      readInfo.m_function  = &Emulator::ReadIOPort;
      readInfo.m_id        = info.m_id;
      m_readIOPortBlocks.push_back(readInfo);
    }

    // add write block
    if ((block.m_type == Config::Type::eIOPortWrite) || (block.m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block.m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: write IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      WriteIOPortBlockInfo writeInfo;
      writeInfo.m_type      = block.m_type;
      writeInfo.m_startPort = info.m_startPort;
      writeInfo.m_endPort   = info.m_endPort;
      writeInfo.m_function   = &Emulator::WriteIOPort;
      writeInfo.m_id        = info.m_id;
      m_writeIOPortBlocks.push_back(writeInfo);
    }
  }

  for (auto & r : m_writeMemoryBlocks) {
    cout << "WRITE " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl; // << " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
  }
  for (auto & r : m_readMemoryBlocks) {
    cout << "READ  " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl; //  <<  " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
  }

  for (auto & r : m_writeIOPortBlocks) {
    cout << "WRITE IO " << HEXFORMAT0x2(r.m_startPort) << " - " << HEXFORMAT0x2(r.m_endPort) << " - " << (int)r.m_type << endl; //  << " " << (void *)r.m_function << endl;
  }
  for (auto & r : m_readIOPortBlocks) {
    cout << "READ IO " << HEXFORMAT0x2(r.m_startPort) << " - " << HEXFORMAT0x2(r.m_endPort) << " - " << (int)r.m_type << endl; //  <<  " " << (void *)r.m_function << endl;
  }
}

uint8_t Emulator::ReadMemory(uint16_t addr)
{
  for (auto & r : m_readMemoryBlocks) {
    if ((addr >= r.m_startAddr) && (addr <= r.m_endAddr)) {
      if (r.m_function != nullptr)
        return std::invoke(r.m_function, *this, r, addr);
      uint8_t data = r.m_memory[addr - r.m_startAddr];
      // cout << "info: read " << HEXFORMAT0x2(data) << " from " << HEXFORMAT0x4(addr - r.m_startAddr) << endl;
      return data;
    }
  }
  return ReadLog(addr);
}

void Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  for (auto & r : m_writeMemoryBlocks) {
    if ((addr >= r.m_startAddr) && (addr <= r.m_endAddr)) {
      if (r.m_function != nullptr) {
        std::invoke(r.m_function, *this, r, addr, data);
        return;
      }
      r.m_memory[addr - r.m_startAddr] = data;
      return;
    }
  }
  WriteLog(addr, data);
}

uint8_t Emulator::DebugReadMemory(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  if (info.m_realFunction != nullptr) {
    cout << "debug: reading via fn from " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
    return std::invoke(info.m_realFunction, *this, info, addr);
  }
  if (info.m_memory == nullptr) {
    cout << "debug: reading from " << HEXFORMAT0x4(addr) << " failed with no memory" << endl;
    return 0x00;
  }
  uint8_t v = info.m_memory[addr - info.m_startAddr];
  cout << "debug: read " << HEXFORMAT0x2(v) << " from " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
  return v;
}

void Emulator::DebugWriteMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  if (info.m_realFunction != nullptr) {
    cout << "debug: writing via fn to " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
    std::invoke(info.m_realFunction, *this, info, addr, data);
    return;
  }
  if (info.m_memory == nullptr) {
    cout << "debug: writing to " << HEXFORMAT0x4(addr) << " failed with no memory" << endl;
    return;
  }
  cout << "debug: writing to " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
  info.m_memory[addr - info.m_startAddr] = data;
}

uint8_t Emulator::DebugReadIOMemory(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  cout << "debug: reading IO memory " << HEXFORMAT0x4(addr) << endl;
  if (info.m_realFunction != nullptr) {
    return std::invoke(info.m_realFunction, *this, info, addr);
  }
  return 0x00;
}

void Emulator::DebugWriteIOMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  cout << "debug: writing IO memory " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(data) << endl;
  //if (info.m_realFunction != nullptr) {
  //  std::invoke(info.m_realFunction, *this, info, addr, data);
  //}
}

uint8_t Emulator::ReadIOMemoryInternal(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  return ReadIOMemory(info.m_id, addr);
}

void Emulator::WriteIOMemoryInternal(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  WriteIOMemory(info.m_id, addr, data);
}

uint8_t Emulator::ReadIOMemory(int, uint16_t)
{
  return 0xff;
}

void Emulator::WriteIOMemory(int, uint16_t, uint8_t data)
{}

/////////////////////////////////////////////////////////////////////////////////////

uint8_t Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t)
{
  return 0x00;
}

void Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
}

void Emulator::WritePort(register uint16_t port, register uint8_t data)
{
  uint8_t shortPort = port & 0xff;
  for (auto & r : m_writeIOPortBlocks) {
    if (shortPort < r.m_startPort)
      continue;
    if (shortPort <= r.m_endPort) {
      if (r.m_function != nullptr) {
        std::invoke(r.m_function, *this, r, shortPort, data);
     }
      return;
    }
  }
  WriteIoPortLog(port, data);
}

uint8_t Emulator::ReadPort(register uint16_t port)
{
  uint8_t shortPort = port & 0xff;
  for (auto & r : m_readIOPortBlocks) {
    if (shortPort < r.m_startPort)
      continue;
    if (shortPort <= r.m_endPort) {
      if (r.m_function != nullptr) {
        return std::invoke(r.m_function, *this, r, shortPort);
      }
    }
  }
  ReadIOPortLog(port);
  return 0x00;
}

uint8_t Emulator::ReadNull(uint16_t)
{
  return 0x00;
}

void Emulator::WriteNull(uint16_t, uint8_t)
{
}

uint8_t Emulator::ReadLog(uint16_t addr)
{
  cerr << "READ " << HEXFORMAT0x4(addr) << endl;
  return 0x00;
}

void Emulator::WriteLog(uint16_t addr, uint8_t val)
{
  cerr << "WRITE " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(val) << endl;
}

uint8_t Emulator::ReadIOPortLog(uint16_t addr)
{
  cerr << "READ IO PORT " << HEXFORMAT0x2(addr) << endl;
  return 0x00;
}

void Emulator::WriteIoPortLog(uint16_t addr, uint8_t val)
{
  cerr << "WRITE IO PORT " << HEXFORMAT0x2(addr) << " " << HEXFORMAT0x2(val) << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::ReadROMFromFile(const std::string &filename, unsigned char *ptr, int len)
{
  ifstream file(filename.c_str(), ifstream::in | ifstream::binary);
  if (!file.is_open())
  {
    cerr << "error: cannot read ROM file '" << filename << "'" << endl;
    return false;
  }

  if (len <= 0)
  {
    file.seekg(0, ios::end);
    len = file.tellg();
    file.seekg(0, ios::beg);
  }

  file.read((char *)ptr, len);

  return !file.fail();
}

void Emulator::DumpStack(const std::vector<uint16_t> & stack)
{
  DumpStackInternal(stack);
}

void Emulator::DumpStack(int count)
{
  std::vector<uint16_t> stack;
  stack.resize(count);
  GetStack(stack);
  DumpStack(stack);
}

bool Emulator::MountDrive(int driveNum, VirtualDrive * drive, bool readOnly)
{
  return false;
}

std::string Emulator::DumpRegs() const
{
  return "";
}

void Emulator::MemoryDump() const
{
  uint16_t addr;
  std::vector<uint8_t> dump;
  dump.resize(GetMemorySize());
  VECTOR_ZERO(dump);

  for (auto & r : m_readMemoryBlocks) {
    if (r.m_memory != nullptr) {
      memcpy(&dump[r.m_startAddr], r.m_memory, r.m_endAddr - r.m_startAddr + 1);
    }
    else {
    }
  }

  std::string basename(GetName() + "_dump");

  {
    std::string filename(basename + ".txt");
    ofstream file(filename);
    if (!file.is_open()) {
      cerr << "error: could not open " << filename << endl;
    }
    else {
      file << DumpRegs();

      int cols = 32;
      int p = 0;
      while (p < dump.size()) {
        file << HEXFORMAT0x4(p) << "  ";
        int len = std::min((int)dump.size(), cols);
        int i;
        for (i = 0; i < len; ++i)
          file << " " << HEXFORMAT2(dump[p + i]);
        while (i < cols)
          file << "   ";
        file << "   ";
        for (i = 0; i < len; ++i)
          file << (isgraph(dump[p + i]) ? (char)dump[p + i] : '.');
        file << endl;
        p += len;
      }
    }
    cout << "memory dumped to " << filename << endl;
  }

  {
    std::string filename(basename + ".bin");
    ofstream file(filename, ios::out | ios::trunc | ios::binary);
    if (!file.is_open()) {
      cerr << "error: could not open " << filename << endl;
    }
    else {
      file.write((char *)&dump[0], dump.size());
    }
    cout << "memory dumped to " << filename << endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////

int Emulator::Run(const Options & options)
{
  m_verbose = options.m_verbose;
  m_debugWriteMemory = options.m_writeDebug;
  m_debugReadMemory  = options.m_readDebug;
  m_keyboardDebug    = options.m_keyboardDebug;
  m_turbo            = options.m_turbo;

  // set target CPU speed
  const Config::CPU * cpu = GetCPUInfo();
  if (cpu == NULL) {
    cerr << "error: cannot get CPU information" << endl;
    return false;
  }

  m_targetCPUClock_Hz = cpu->m_clockSpeed_MHz * 1000000.0;
  m_actualCPUClock_Hz = m_targetCPUClock_Hz;

  if (m_verbose) {
    cout << "debug: target CPU speed is " << m_targetCPUClock_Hz << endl;
  }

  // allow any descendant classes to do whatever they need
  if (!Open(options)) {
    cerr << "error: cannot open emulator" << endl;
    return -1;
  }

  // get the drives
  for (auto &r : options.m_driveFns) {
    std::string fn(r.second);
    VirtualDriveFile *drive = new VirtualDriveFile();
    if (!drive->Open(fn, true))
      return false;
    if (!MountDrive(r.first, drive, true)) {
      cerr << "error: cannot mount drive " << r.first << " with " << r.second << endl;
      return -1;
    }
    cerr << "info: mounted '" << fn << " as drive " << r.first << endl;
  }

  CompileConfigBlocks();

  // initlialize SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("error initializing SDL: %s\n", SDL_GetError());
    return -1;
  }

  MainWindow mainWindow;

  cerr << "Creating screen" << endl;
  cerr << "font size is " << (int)options.m_fontSize << endl;

  CreateScreen(mainWindow, options);

  if (!Start()) {
    cerr << "error: cannot start emulator" << endl;
    return -1;
  }

  bool videoTest = false;
  options.m_args.GetValue("--videotest",    videoTest);

  if (videoTest) {
    if (m_memMapScreen) {
      const Config::MemoryMappedScreen * mmapInfo = GetMemoryMappedInfo();
      for (int i = 0; i < mmapInfo->m_screenCols * mmapInfo->m_screenRows; ++i) {
        m_memMapScreen->WriteMemoryAtAddress(i, i);
      }
      m_memMapScreen->Update(true);
    }
    else if (m_terminal) {
      for (int c = 0x20; c <= 0x3f; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n");
      for (int c = 0x40; c <= 0x5f; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n");
      for (int c = 0x60; c <= 0x7e; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n\n");
      m_terminal->WriteString(m_info->m_title);
      m_terminal->WriteString("\r\n\n");
      m_terminal->Update(true);
    }

    if (m_screen) {
      auto now = std::chrono::system_clock::now();
      auto finish = std::chrono::system_clock::now() + std::chrono::seconds(4);
      while (std::chrono::system_clock::now() < finish) {
        usleep(1000);
        m_screen->Update(false);
      }
    }
  }

#define GET_NOW_AS_DOUBLE() \
  std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

  m_cycleCounter = 0;
  double now = GET_NOW_AS_DOUBLE();

  cout << "now = " << FIXEDFORMAT(3, now) << endl;

  // initialise real time pollers
  for (auto & r : m_pollers.m_list) {
    PollDef & def = r.second;
    def.m_lastTime  = now;
    def.m_lastClock = m_cycleCounter;
    if (def.m_pollIsTime) {
      def.m_nextTime = now + def.m_timeInterval;
    }
    else {
      def.m_nextClock = m_cycleCounter + def.m_clockInterval;
    }
  }

  bool displayCPUSpeed = false;
  options.m_args.GetValue("--displaySpeed", displayCPUSpeed);

  // run emulator
  auto lastPoll  = std::chrono::system_clock::now();
  auto lastSpeed = std::chrono::system_clock::now();

  for (;;) {
    now                            = GET_NOW_AS_DOUBLE();
    double  earliestNextRealTime_s = now + 1.0;
    int64_t earliestNextClockTime  = m_cycleCounter + 1e+6;

    // run pollers
    for (auto & r : m_pollers.m_list) {
      PollDef & def = r.second;
      if (def.m_pollIsTime) {
        if (now >= def.m_nextTime) {
//          cout << "info: clocks = " << m_cycleCounter << ", clock interval = " << r.m_clockInterval << endl;
          def.Execute(now - def.m_lastTime, m_cycleCounter - def.m_lastClock);
          def.m_lastTime  = now;
          def.m_lastClock = m_cycleCounter;
          def.m_nextTime  = def.m_nextTime + def.m_timeInterval;
        }
//        cout << "real time interval = " << r.m_timeInterval << endl;
        earliestNextRealTime_s = std::min<double>(earliestNextRealTime_s, def.m_nextTime);
      }
      else if (!def.m_pollIsTime) {
        if (m_cycleCounter >= def.m_nextClock) {
          def.Execute(now - def.m_lastTime, m_cycleCounter - def.m_lastClock);
          def.m_lastTime  = now;
          def.m_lastClock = m_cycleCounter;
          def.m_nextClock = def.m_nextClock + def.m_clockInterval;
        }
        earliestNextClockTime = std::min<uint64_t>(earliestNextClockTime, def.m_nextClock);
      }
    }

    // run the CPU
    uint64_t cyclesToDo = earliestNextClockTime - m_cycleCounter;
    double   timeToDo_s = earliestNextRealTime_s - now;

    uint64_t cyclesForTime = timeToDo_s * m_targetCPUClock_Hz;
    if (cyclesForTime < cyclesToDo)
      cyclesToDo = cyclesForTime;

    int remaining = Exec(cyclesToDo);
    int cyclesDone = cyclesToDo - remaining;
    //cout << "time to do " << timeToDo_s << ", cycles to do - " << cyclesToDo << ", cycles done = " << cyclesDone << endl;
    m_cycleCounter += cyclesDone;
  }
}

int Emulator::AddRealTimePollDef(double seconds, PollHandler handler)
{
  return m_pollers.Add(seconds, handler);
}

int Emulator::AddCPUTimePollDef(uint64_t cycles, PollHandler handler)
{
  return m_pollers.Add(cycles, handler);
}

void Emulator::CalcCPUSpeed(double secs, uint64_t clocks)
{
  if (secs > 0) {
    m_actualCPUClock_Hz = 1.0 * clocks / secs;
    //cout << "secs " << secs << ", clocks " << clocks << endl;
    cout << std::fixed << std::setprecision(3) << (m_actualCPUClock_Hz / 1e+6) << " MHz" << endl;
  }
}

void Emulator::LoadGRZ()
{
  /*
  nfd_OpenDialogExt extInfo;
  memset(&extInfo, 0, sizeof(extInfo));
  extInfo.filterList      = "grz";
  extInfo.title           = "Open GRZ file";

  nfdchar_t * path = NULL;
  if (NFD_OpenDialogExt(&extInfo, &path) == NFD_OKAY) {
    std::string json;
    {
      std::ifstream file(path);
      if (!file.is_open()) {
        cout << "cannot open " << path << endl;
        return;
      }
      std::string line;
      while ( getline(file,line)) {
        json += line;
      }
      file.close();
    }
    cout << "json: " << json << endl;
    {
      GRZ grz;
      std::stringstream strm(json);
      try {
        cereal::JSONInputArchive ar(strm);
        grz.load(ar);
      }
      catch(const std::exception& e) {
        cerr << "load failed - " << e.what() << endl;
        return;
      }

      std::string dir(path);

      size_t pos = dir.rfind(DIR_SEPERATOR);
      if (pos != std::string::npos)
        dir = dir.substr(0, pos+1);

      std::string fn = dir + grz.m_filename;

      int fd = ::open(fn.c_str(), O_RDONLY);
      if (fd < 0) {
        cerr << "error: cannot open '" << fn << "' - " << strerror(errno) << endl;
        return;
      }

      // read file
      off_t len = lseek(fd, 0, SEEK_END);
      if (len < 0) {
        cerr << "error: cannot get length of '" << fn << "'" << endl;
        return;
      }

      // check offset
      unsigned offset = grz.m_hasOffs ? grz.m_offs : 0;
      if (grz.m_hasOffs && (len < offset)) {
        cerr << "error: file too short for offset" << endl;
        return;
      }

      // check length
      signed length = (grz.m_hasLength ? grz.m_length : len) - offset;
      if ((length < 0) || ((offset + length) > len)) {
        cerr << "error: file too short for offset + length" << endl;
        return;
      }

      // allocate and read data
      std::vector<int8_t> m_data;
      m_data.resize(length);
      lseek(fd, offset, SEEK_SET);
      ::read(fd, &m_data[0], length);
      ::close(fd);

      // copy to memory
      unsigned addr = grz.m_addr;
      for (unsigned i = 0; i < length; ++i) {
        WriteMemory(addr++, m_data[i]);
      }

      cout << "info: loaded " << HEXFORMAT0x4(length) << " bytes to " << HEXFORMAT0x4(grz.m_addr) << endl;
    }
  }
  */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0

  double interval = std::chrono::duration<double>(std::chrono::system_clock::now() - m_cpuDelayTimer).count();
      if (interval >= 0.001) {
        m_actualCPUClock_Hz = m_speedCycleCounter / interval;
        m_speedCycleCounter = 0;
        m_cpuDelayTimer = std::chrono::system_clock::now();
      }
    }
    }
    else {
      for (;;) {
        Exec(poll_cycles);

    auto now = std::chrono::system_clock::now();

    double interval = std::chrono::duration<double>(now - lastPoll).count();
    if (interval >= 50e-3) {
      if (!Poll())
        break;
      lastPoll = now;
    }

    if (displayCPUSpeed) {
      interval = std::chrono::duration<double>(now - lastSpeed).count();
      if (interval >= 1) {
        cout << std::fixed << std::setprecision(3) << (GetActualCPUSpeed_Hz() / 1e+6) << " MHz" << endl;
        lastSpeed = now;
      }
    }
  }


  // full speed
  if (m_turbo) {
    int cyclesDone = cycles - ExecZ80(&m_cpu, cycles);
    m_cycleCounter      += cyclesDone;
    m_speedCycleCounter += cyclesDone;
    if (m_speedCycleCounter > 100) {
      double interval = std::chrono::duration<double>(std::chrono::system_clock::now() - m_cpuDelayTimer).count();
      if (interval >= 0.001) {
        m_actualCPUClock_Hz = m_speedCycleCounter / interval;
        m_speedCycleCounter = 0;
        m_cpuDelayTimer = std::chrono::system_clock::now();
      }
    }
  }
  else {
#define INC  4
    while (cycles > 0) {
      int cyclesDone = INC - ExecZ80(&m_cpu, INC);
      cycles              -= cyclesDone;
      m_cycleCounter      += cyclesDone;
      m_speedCycleCounter += cyclesDone;

      double interval = std::chrono::duration<double>(std::chrono::system_clock::now() - m_cpuDelayTimer).count();

      if ((m_speedCycleCounter > 100) && (interval >= 0.001)) {
        m_actualCPUClock_Hz = m_speedCycleCounter / interval;
        m_speedCycleCounter = 0;
        m_cpuDelayTimer = std::chrono::system_clock::now();

        m_cpuDelayRepeat = m_cpuDelayRepeat * m_actualCPUClock_Hz / m_targetCPUClock_Hz;
        if (m_cpuDelayRepeat < 1)
          m_cpuDelayRepeat = 1;
        else if (m_cpuDelayRepeat > 600)
          m_cpuDelayRepeat = 600;
      }

      for (int i = 0; i < m_cpuDelayRepeat; ++i)
        memset(m_delayBuffer, 0, sizeof(m_delayBuffer));
    }
  }
  // exiting
}

#endif

/////////////////////////////////////////////////////////////////////////////////////
