#include <memory.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <time.h>
#include <functional>
#include <math.h>

#include "misc.h"
#include "config.h"
#include "cpu/emulator.h"
#include "magmedia/fdc.h"
#include "video/virtual_screen.h"
#include "mainwindow.h"

using namespace std;

#define TRACE_SYM   SDLK_F2
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

  CompileConfigBlocks();

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
          if (event.key.keysym.sym == TRACE_SYM)
            SetTrace(true);

          if (event.key.keysym.sym == TRACE_SYM + 1)
            SetTrace(false);

          if (event.key.keysym.sym == SDLK_F9) {
            ChangeVideoColour();
          }
          else if (event.key.keysym.sym == SDLK_F10) {
            MemoryDump();
          }
          else if (event.key.keysym.sym == REBOOT_SYM) {
            Reset();
          }
          else
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

bool CloseToInteger(double val)
{
  return fabs((val + .4) - trunc(val)) <= 0.1;
}

void Emulator::CreateScreen(MainWindow & mainWindow, const Options & options)
{
  const Config::Video * video = GetVideoInfo();

  const Config::Block * block = GetConfigBlock(Config::Type::eMonitor);
  const Config::Monitor * monitor = (block == nullptr) ? nullptr : &block->m_info.m_monitor;

  int monitorW, monitorH;       // monitor resolution in pixels
  int videoW, videoH;           // pixels generations by emulator
  int top = 0;
  int left = 0;                // border top and left
  int fieldMult = 1;
  std::string standardName;
  int overscanX;
  int overscanY;
  double hratio;
  double vratio;

  // get the scale
  if ((monitor == nullptr) || (monitor->m_std == Config::VideoStandard::eNone)) {
    cerr << "error: must have monitor declaration" << endl;
    exit(-1);
  }
  else {

    int totalLines;
    double frameRate;
    double activeHTime_us;

    switch (monitor->m_std) {
      case Config::VideoStandard::eNone:
        cerr << "info: monitor has no video standard definined" << endl;
        exit(-1);
      case Config::VideoStandard::ePAL:
        standardName   = "PAL";
        totalLines     = 625;
        monitorH       = 576;
        frameRate      = 25.0;
        activeHTime_us = 52.0;
        hratio         = 4.0;
        vratio         = 3.0;
        break;
      case Config::VideoStandard::eNTSC:
        standardName   = "NTSC";
        totalLines     = 525;
        monitorH       = 488;
        frameRate      = 30.0;
        activeHTime_us = 52.6;
        hratio         = 4.0;
        vratio         = 3.0;
        break;
    }

    // display monitor information
    monitorW = monitor->m_pixelFrequency_MHz * activeHTime_us;
    cout << "info: " << standardName 
         << " monitor at " << monitor->m_pixelFrequency_MHz << " MHz"
         << " is " << monitorW << "x" << monitorH << endl;

    // remove overscan
    overscanX = monitorW * 2 * monitor->m_hOverScan_percent / 100.0;
    overscanY = monitorH * 2 * monitor->m_vOverScan_percent / 100.0;
    cout << "info: overscan is " << overscanX << ", " << overscanY << endl;

    // display video output pixels
    videoW = video->m_screenWidth;
    videoH = video->m_screenHeight;
  }

  int vdup = 2;

  cout << "info: raw video output is " << videoW << "x" << videoH*vdup << endl;

  // get size of screen
  SDL_DisplayMode mode;
  SDL_GetDesktopDisplayMode(0, &mode);
  cout << "info: screen is " << mode.w << "x" << mode.h << endl;

  // calculate relative pixel sizes scaling X or Y
  int ratio = 1;
  int hscale;
  int vscale;
  bool found = false;
  while (((ratio * videoW) <= mode.w) && ((ratio * videoH*vdup) <= mode.h)) {

    // see if vertical scale can be integer
    {
      int w = 1.0 * ratio * (videoH * vdup) * hratio / vratio;
      int hs = trunc(w / videoW);
      if (hs >= 0) {
        cout << "info: trying vscale=" << ratio 
                                << " - " << hs << ":" << ratio 
                                << " gives " << hs * videoW << "x" << ratio * videoH 
                                << " compared to " << w << "x" << ratio * videoH << endl;
        if (CloseToInteger(w / videoW)) {
          hscale = hs;
          vscale = ratio;
          cout << "info: good" << endl;
          found = true;
        }
      }
    }

    // see if horizontal scale can be integer
    {
      int h = 1.0 * ratio * videoW * vratio / hratio;
      int vs = trunc(h / videoW);
      if (vs > 0) {
        cout << "info: trying hscale=" << ratio
                                << " - " << ratio << ":" << vs 
                                << " gives " << ratio * videoW << "x" << vs * videoH 
                                << " compared to " << ratio * videoH << "x" << h << endl;
        if (CloseToInteger(h / (vdup * videoH))) {
          hscale = ratio;
          vscale = vs;
          cout << "info: good" << endl;
          found = true;
        }
      }
    }

    ratio++;
  }

  if (found) {
    cout << "info: found approximate scale " << hscale << ":" << vscale << endl;
  }
  else {
    hscale = trunc(mode.w * 3 / 4 / videoW);
    vscale = trunc(mode.h * 3 / 4 / videoH);
    cout << "info: using pixel scale " << hscale << ":" << vscale << endl;
  }

/*
  monitorW -= overscanX;
  monitorH -= overscanY;
  cout << "info: " << standardName 
       << " monitor less overscan is " << monitorW << "x" << monitorH << endl;

  left = (monitorW - videoW) / 2;
  if (left < 0)
    left = 0;
  top  = (monitorH - videoH*vdup) / 2 / vdup;
  if (top < 0)
    top = 0;
  cout << "info: left = " << left << ", top = " << top << endl;

  cout << "scale is " << hscale << "x" << vscale << endl;
*/

  // calculate monitor scale
  int width  = videoW * hscale;
  int height = videoH * vscale;

  // create main window 
  mainWindow.Open(width, height);

  VirtualScreen * screen = VirtualScreen::Create(mainWindow, *this, options, *video);
  if (screen == nullptr) {
    cerr << "error: could not instantiate screen type" << endl;
    return; // false;
  }

  screen->SetScale(hscale, vscale);
  screen->SetOffset(left, top);

  m_video.reset(screen);
  m_video->Open();
}

bool Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
}

void Emulator::WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  if (addr > info.m_endAddr)
    cerr << "warning: bad video write" << endl;
  else  
    m_video->WriteMemoryAtPos(addr - info.m_startAddr, data);
}

uint8_t Emulator::ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  if (addr > info.m_endAddr) {
    cerr << "warning: bad video read" << endl;
    return 0x00;
  }
  else  
    return m_video->ReadMemoryAtPos(addr - info.m_startAddr);
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
  cerr << "warning: emulator got key down sym code" << HEXFORMAT0x2(keysym.sym) << endl;
}

void Emulator::OnKeyUp(const SDL_Keysym &keysym)
{
  cerr << "warning: emulator got key up sym code" << HEXFORMAT0x2(keysym.sym) << endl;
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
  return nullptr;
}

const Config::CPU * Emulator::GetCPUInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eCPU);
  return (info == nullptr) ? nullptr : &info->m_info.m_cpu;
}

const Config::Video * Emulator::GetVideoInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eVideo);
  if (info == nullptr)
    info = GetConfigBlock(Config::Type::eVideoExternal);
  return (info == nullptr) ? nullptr : &info->m_info.m_video;
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

  const Config::Block * block = m_info->m_blocks;
  for (int i = 0; i < MAX_INFO_BLOCKS; ++i) {

    if (block->m_type == Config::Type::eEnd)
      break;

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

    // add read block
    if ((block->m_type == Config::Type::eIOPortRead) || (block->m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block->m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: read IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      ReadIOPortBlockInfo readInfo;
      readInfo.m_type      = block->m_type;
      readInfo.m_startPort = info.m_startPort;
      readInfo.m_endPort   = info.m_endPort;
      readInfo.m_function  = &Emulator::ReadIOPort;
      readInfo.m_id        = info.m_id; 
      m_readIOPortBlocks.push_back(readInfo);
    }

    // add write block
    if ((block->m_type == Config::Type::eIOPortWrite) || (block->m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block->m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: write IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      WriteIOPortBlockInfo writeInfo;
      writeInfo.m_type      = block->m_type;
      writeInfo.m_startPort = info.m_startPort;
      writeInfo.m_endPort   = info.m_endPort;
      writeInfo.m_function   = &Emulator::WriteIOPort;
      writeInfo.m_id        = info.m_id; 
      m_writeIOPortBlocks.push_back(writeInfo);
    }

    ++block;
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
      return r.m_memory[addr - r.m_startAddr];
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

/////////////////////////////////////////////////////////////////////////////////////
