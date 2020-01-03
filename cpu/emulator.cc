#include <memory.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <time.h>
#include <functional>

#include "misc.h"
#include "config.h"
#include "cpu/emulator.h"
#include "magmedia/fdc.h"
#include "video/virtual_screen.h"
#include "mainwindow.h"

using namespace std;

#define REBOOT_SYM  SDLK_F12


/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator(EmulatorInfo * info)
  : m_info(info)
{
  m_debugWriteMemory = true;
  m_debugReadMemory = true;
}

void Emulator::Init()
{  
}

const EmulatorInfo & Emulator::GetInfo() const
{
  return *m_info;
}

bool Emulator::Open(const Options & options)
{
  // get the drives
  for (auto &r : options.m_driveFns) {
    std::string fn(r.second);
    VirtualDriveFile *drive = new VirtualDriveFile();
    if (!drive->Open(fn, true))
      return false;
    if (!MountDrive(r.first, drive, true))
      return false;
    cerr << "info: mounted '" << fn << " as drive " << r.first << endl;
  }

  // set target CPU speed
  const Config::CPU * cpu = GetCPUInfo();
  if (cpu == NULL) {
    cerr << "error: cannot get CPU information" << endl;
    return false;
  }

  m_targetCPUClock_Hz = cpu->m_clockSpeed_MHz * 1000000.0;
  m_actualCPUClock_Hz = m_targetCPUClock_Hz;

  m_debugWriteMemory = false; //options.m_writeDebug;
  m_debugReadMemory  = false; //options.m_writeDebug;

  CompileMemoryBlocks();

  return true;
}

bool Emulator::SetRAMSize_k(int len)
{
  m_ram.resize(len * 1024);
  m_ramSize_bytes = m_ram.size();
  m_ramMask = (m_ramSize_bytes - 1);

  cout << "info: RAM size " << len << " k, " << m_ramSize_bytes << " bytes, " << HEXFORMAT0x4(m_ramMask) << endl;
} 

int Emulator::GetRAMSize_k() const
{
  return m_ram.size() / 1024;
}

double Emulator::GetActualCPUSpeed_Hz() const
{
  return m_actualCPUClock_Hz;
}

bool Emulator::Poll()
{
  if (m_video)
    m_video->Update(false);

  SDL_Event event;
  if (SDL_PollEvent(&event)) {
    switch (event.type) {

      case SDL_QUIT:
        return false;
        break;

      case SDL_KEYDOWN:
        if (event.key.repeat == 0) {
          OnKeyDown(event.key.keysym);
        }
        break;

      case SDL_KEYUP:
        if (event.key.repeat == 0)
          OnKeyUp(event.key.keysym);
        break;

      default:
        break;
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CreateScreen(MainWindow & mainWindow, const Options & options)
{
  const Config::Video * video = GetVideoInfo();

  // create main window with out best guess at the size
  mainWindow.Open(2, video->m_screenWidth, video->m_screenHeight);

  // get the emulator to create the font, and any other video options
  OpenVideo(mainWindow, options);

  // calculate the correct size given the final font and the required rows and columns
  int finalWidth  = video->m_screenCols * video->m_font.m_width;
  int finalHeight = video->m_screenRows * video->m_font.m_height;

  double hScale = 1.0;
  double vScale = 1.0;

  bool changed = false;

  if (finalWidth != video->m_screenWidth) {
    hScale = finalWidth * 1.0 / video->m_screenWidth;
    changed = true;
  }
  if (finalHeight != video->m_screenHeight) {
    vScale = finalHeight * 1.0 / video->m_screenHeight;
    changed = true;
  }

  if (changed) {
    cout << "info: screen resize from " << dec << video->m_screenWidth << "x" << video->m_screenHeight << " to " <<  finalWidth << "x" << finalHeight << endl; 
    //mainWindow.SetFinalSizePixels(finalWidth, finalHeight)
    mainWindow.Open(2, finalWidth, finalHeight);

    // get the emulator to create the font, and any other video options
    OpenVideo(mainWindow, options);
/*
    SDL_RenderSetScale(SDL_Renderer* renderer,
                       float         scaleX,
                       float         scaleY)
*/
  }
}

bool Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  const Config::Video & info = *GetVideoInfo();
  m_video.reset(VirtualScreen::Create(mainWindow, *this, options, info));
  return m_video.get() != nullptr;
}

void Emulator::WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  m_video->WriteMemory(addr - info.m_startAddr, data);
}

uint8_t Emulator::ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  return m_video->ReadMemory(addr - info.m_startAddr);
}

void Emulator::ChangeVideoColour()
{
  SDL_Color newFg;
  SDL_Color newBg;

  m_video->GetFontColour(newFg, newBg);

  newFg.r ^= 0xff;
  newFg.b ^= 0xff;

  m_video->SetFontColour(newFg, newBg);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::OnKeyDown(const SDL_Keysym &keysym)
{
  if (keysym.sym == SDLK_F9) {
    ChangeVideoColour();
  }
  else if (keysym.sym == SDLK_F10) {
    MemoryDump();
  }
  else if (keysym.sym == REBOOT_SYM) {
    Reset();
  }
  else {
    cerr << "warning: unknown keyboard sym code" << HEXFORMAT0x2(keysym.sym) << endl;
  }
}

void Emulator::OnKeyUp(const SDL_Keysym &keysym)
{
}

/////////////////////////////////////////////////////////////////////////////////////

const Config::Block * Emulator::GetConfigBlock(Config::Type type) const
{
  const Config::Block * block = m_info->m_blocks;
  for (int i = 0; i < MAX_INFO_BLOCKS; ++i) {
    if (block->m_type == Config::Type::eEnd)
      break;
    if (block->m_type == type)
      return block;
    ++block;  
  }
  return NULL;
}

const Config::CPU * Emulator::GetCPUInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eCPU);
  return (info == NULL) ? NULL : &info->m_info.m_cpu;
}

const Config::Video * Emulator::GetVideoInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eVideo);
  return (info == NULL) ? NULL : &info->m_info.m_video;
}

const Config::RAM * Emulator::GetMainRAMInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eMainRAM);
  return (info == NULL) ? NULL : &info->m_info.m_ram;
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CompileMemoryBlocks()
{
  m_readMemoryBlocks.clear();
  m_writeMemoryBlocks.clear();

  cout << "memory read debug start " << m_debugReadMemory << endl;

  const Config::Block * block = m_info->m_blocks;
  for (int i = 0; i < MAX_INFO_BLOCKS; ++i) {

    if (block->m_type == Config::Type::eEnd)
      break;

    cout << "memory read debug " << m_debugReadMemory << endl;

    // add RAM
    if ((block->m_type == Config::Type::eRAM) || (block->m_type == Config::Type::eMainRAM)) {
      const Config::RAM & info = block->m_info.m_ram;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: RAM block has end address" << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      WriteMemoryBlockInfo writeInfo;
      writeInfo.m_type      = block->m_type;
      writeInfo.m_startAddr = info.m_startAddr;
      writeInfo.m_endAddr   = info.m_endAddr;
      writeInfo.m_storage.resize(writeInfo.m_endAddr - writeInfo.m_startAddr + 1);
      writeInfo.m_memory    = &writeInfo.m_storage[0];
      if (m_debugWriteMemory) {
        writeInfo.m_function  = &Emulator::DebugWriteMemory;
      }
      m_writeMemoryBlocks.push_back(writeInfo);
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block->m_type;
        readInfo.m_startAddr = writeInfo.m_startAddr;
        readInfo.m_endAddr   = writeInfo.m_endAddr;
        readInfo.m_memory    = writeInfo.m_memory;
        if (m_debugReadMemory) {
          readInfo.m_function  = &Emulator::DebugReadMemory;
        }
        m_readMemoryBlocks.push_back(readInfo);
      }
    }

    // add ROM
    else if (block->m_type == Config::Type::eROM) {
      const Config::ROM & info = block->m_info.m_rom;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: ROM block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type      = block->m_type;
      readInfo.m_startAddr = info.m_startAddr;
      readInfo.m_endAddr   = info.m_endAddr;
      readInfo.m_memory    = info.m_data;
      if (m_debugReadMemory) {
        readInfo.m_function  = &Emulator::DebugReadMemory;
      }
      m_readMemoryBlocks.push_back(readInfo);
    }

    // add read memIO
    else if (block->m_type == Config::Type::eMemIORead) {
      const Config::MemIO & info = block->m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO read block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type         = block->m_type;
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
    else if (block->m_type == Config::Type::eMemIOWrite) {
      const Config::MemIO & info = block->m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO write block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      WriteMemoryBlockInfo writeInfo;
      writeInfo.m_type         = block->m_type;
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
    else if (block->m_type == Config::Type::eVideo) {
      const Config::Video & info = block->m_info.m_video;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: video block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }

      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block->m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        writeInfo.m_realFunction = &Emulator::WriteToVideo;
        writeInfo.m_function     = &Emulator::WriteToVideo;
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block->m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_realFunction = &Emulator::ReadFromVideo;
        readInfo.m_function     = &Emulator::ReadFromVideo;
        m_readMemoryBlocks.push_back(readInfo);
      }
    }
    ++block;
  }

  for (auto & r : m_writeMemoryBlocks) {
    cout << "WRITE " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
  }
  for (auto & r : m_readMemoryBlocks) {
    cout << "READ  " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type <<  " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
  }
}

uint8_t Emulator::ReadMemory(uint16_t addr)
{
  for (auto & r : m_readMemoryBlocks) {
    if ((addr < r.m_startAddr) || (addr > r.m_endAddr))
      continue;
    if (r.m_function != nullptr)
      return std::invoke(r.m_function, *this, r, addr);
    return r.m_memory[addr - r.m_startAddr];
  }
  return ReadLog(addr);
}

void Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  for (auto & r : m_writeMemoryBlocks) {
    if ((addr < r.m_startAddr) || (addr > r.m_endAddr))
      continue;
    if (r.m_function != nullptr) {
      std::invoke(r.m_function, *this, r, addr, data);
      return;
    }
    r.m_memory[addr - r.m_startAddr] = data;
    return;
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
  cout << "debug: reading from " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
  return info.m_memory[addr - info.m_startAddr];
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

void Emulator::CompileIOPortBlocks()
{
  m_readIOPortBlocks.clear();
  m_writeIOPortBlocks.clear();
}


void Emulator::WritePort(register uint16_t port, register uint8_t value)
{}

uint8_t Emulator::ReadPort(register uint16_t port)
{
  return 0xff;  
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

/////////////////////////////////////////////////////////////////////////////////////
