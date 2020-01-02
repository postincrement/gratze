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

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator(EmulatorInfo * info)
  : m_info(info)
{
}

const EmulatorInfo & Emulator::GetInfo() const
{
  return *m_info;
}

bool Emulator::Open(const Options & options)
{
  CompileMemoryBlocks();

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
  const CPUInfo * cpu = GetCPUInfo();
  if (cpu == NULL) {
    cerr << "error: cannot get CPU information" << endl;
    return false;
  }

  m_targetCPUClock_Hz = cpu->m_clockSpeed_MHz * 1000000.0;
  m_actualCPUClock_Hz = m_targetCPUClock_Hz;

  m_debugWriteMemory = options.m_writeDebug;
  m_debugReadMemory  = options.m_writeDebug;

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
  const VideoDriverInfo * video = GetVideoInfo();

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
  const VideoDriverInfo * video = GetVideoInfo();

  // create the virtual screen
  m_video.reset(new VirtualScreen(mainWindow, *this, options));

  // initialise the video driver, if we can
  switch (video->m_type) {
    case VideoDriver::eMemoryMapped:
      if (video->m_font.m_creator != NULL) {
        (*video->m_font.m_creator)(options, video->m_font, m_fontData);
        m_video->SetFont(new PixelFont(video->m_font.m_count, video->m_font.m_width, video->m_font.m_height, &m_fontData[0]));
      }
      m_videoOffset = 0;
      break;
    case VideoDriver::eNone:
      return false;
  }

  return true;
}

void Emulator::WriteToMemoryMappedVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  WriteVideoChar(addr - info.m_startAddr, data);
}

uint8_t Emulator::ReadFromMemoryMappedVideo(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  return m_video->Read(addr - info.m_startAddr);
}

void Emulator::WriteVideoChar(unsigned int offset, uint8_t ch)
{
  if (m_video)
    m_video->Write(offset, ch);
}

int Emulator::GetVideoOffset()
{
  return m_videoOffset;
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
  else {
    cerr << "warning: unknown keyboard sym code" << HEXFORMAT0x2(keysym.sym) << endl;
  }
}

void Emulator::OnKeyUp(const SDL_Keysym &keysym)
{
}

/////////////////////////////////////////////////////////////////////////////////////

const Emulator::InfoBlock * Emulator::GetInfoBlock(BlockType type) const
{
  const InfoBlock * block = m_info->m_blocks;
  for (int i = 0; i < MAX_INFO_BLOCKS; ++i) {
    if (block->m_type == BlockType::eEnd)
      break;
    if (block->m_type == type)
      return block;
    ++block;  
  }
  return NULL;
}

const Emulator::CPUInfo * Emulator::GetCPUInfo() const
{
  const InfoBlock * info = GetInfoBlock(BlockType::eCPU);
  return (info == NULL) ? NULL : &info->m_info.m_cpu;
}

const Emulator::VideoDriverInfo * Emulator::GetVideoInfo() const
{
  const InfoBlock * info = GetInfoBlock(BlockType::eVideo);
  return (info == NULL) ? NULL : &info->m_info.m_video;
}

const Emulator::RAMInfo * Emulator::GetMainRAMInfo() const
{
  const InfoBlock * info = GetInfoBlock(BlockType::eMainRAM);
  return (info == NULL) ? NULL : &info->m_info.m_ram;
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CompileMemoryBlocks()
{
  m_readMemoryBlocks.clear();
  m_writeMemoryBlocks.clear();

  const InfoBlock * block = m_info->m_blocks;
  for (int i = 0; i < MAX_INFO_BLOCKS; ++i) {

    // add RAM
    if ((block->m_type == BlockType::eRAM) || (block->m_type == BlockType::eMainRAM)) {
      const RAMInfo & info = block->m_info.m_ram;
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
    else if (block->m_type == BlockType::eROM) {
      const ROMInfo & info = block->m_info.m_rom;
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
    else if (block->m_type == BlockType::eMemIORead) {
      const MemIOInfo & info = block->m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO read block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type      = block->m_type;
      readInfo.m_startAddr = info.m_startAddr;
      readInfo.m_endAddr   = info.m_endAddr;
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
    else if (block->m_type == BlockType::eMemIOWrite) {
      const MemIOInfo & info = block->m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO write block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      WriteMemoryBlockInfo writeInfo;
      writeInfo.m_type      = block->m_type;
      writeInfo.m_startAddr = info.m_startAddr;
      writeInfo.m_endAddr   = info.m_endAddr;
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
    else if ((block->m_type == BlockType::eVideo) && (block->m_info.m_video.m_type == VideoDriver::eMemoryMapped)) {
      const VideoDriverInfo & info = block->m_info.m_video;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: video block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }

      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block->m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        writeInfo.m_function  = &Emulator::WriteToMemoryMappedVideo;
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block->m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_function  = &Emulator::ReadFromMemoryMappedVideo;
        m_readMemoryBlocks.push_back(readInfo);
      }
    }
    ++block;
  }

  for (auto & r : m_writeMemoryBlocks) {
    cout << "WRITE " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl;
  }
  for (auto & r : m_readMemoryBlocks) {
    cout << "READ  " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl;
  }
}

uint8_t Emulator::ReadMemory(uint16_t addr)
{
  ReadMemoryBlockInfo * ptr = &m_readMemoryBlocks[0];
  for (int i = 0; i < m_readMemoryBlocks.size(); ++i) {
    if (addr >= ptr->m_startAddr && addr <= ptr->m_endAddr) {
      if (ptr->m_function != NULL)
        return std::invoke(ptr->m_function, *this, *ptr, addr);
      return ptr->m_memory[addr - ptr->m_startAddr];
    }
    ++ptr;
  }
  return ReadLog(addr);
}

void Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  if (m_debugWriteMemory) {
    cout << "debug: writing " << HEXFORMAT0x4(addr) << endl;
  }
  WriteMemoryBlockInfo * ptr = &m_writeMemoryBlocks[0];
  for (int i = 0; i < m_writeMemoryBlocks.size(); ++i) {
    if (addr >= ptr->m_startAddr && addr <= ptr->m_endAddr) {
      if (ptr->m_function != NULL) {
        std::invoke(ptr->m_function, *this, *ptr, addr, data);
        return;
      }
      ptr->m_memory[addr - ptr->m_startAddr] = data;
      return;
    }
    ++ptr;
  }
  WriteLog(addr, data);
}

uint8_t Emulator::DebugReadMemory(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  cout << "debug: reading memory " << HEXFORMAT0x4(addr) << endl;
  uint8_t val = info.m_memory[addr - info.m_startAddr];
  return val;
}

void Emulator::DebugWriteMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  cout << "debug: writing memory " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(data) << endl;
  info.m_memory[addr - info.m_startAddr] = data;
}

uint8_t Emulator::DebugReadIOMemory(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  cout << "debug: reading IO memory " << HEXFORMAT0x4(addr) << endl;
  uint8_t val = ReadIOMemoryInternal(info, addr);
  return val;
}

void Emulator::DebugWriteIOMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  cout << "debug: writing IO memory " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(data) << endl;
  WriteIOMemoryInternal(info, addr, data);
}

uint8_t Emulator::ReadIOMemoryInternal(const ReadMemoryBlockInfo & info, uint16_t addr)
{
  //cout << "READ IO MEM " << HEXFORMAT0x4(addr) << endl;
  return ReadIOMemory(info.m_id, addr);
}

void Emulator::WriteIOMemoryInternal(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  //cout << "WRITE IO MEM " << HEXFORMAT0x4(addr) << endl;
  WriteIOMemory(info.m_id, addr, data);
}

uint8_t Emulator::ReadIOMemory(int id, uint16_t)
{
  return 0xff;
}

void Emulator::WriteIOMemory(int id, uint16_t, uint8_t data)
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
