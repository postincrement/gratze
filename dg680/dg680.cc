

#include <iostream>
#include <iomanip>

#include "dg680.h"
#include "video/dg640.h"

using namespace std;

extern unsigned char g_dgos680_1_4ROM[2048];

DG680_Emulator::DG680_Emulator()
{
  m_rom = g_dgos680_1_4ROM;
  m_romSize = sizeof(g_dgos680_1_4ROM);
}

std::string DG680_Emulator::GetTitle() const
{ 
  return "DG-680"; 
}

int DG680_Emulator::GetDefaultRAMSize_k() const
{ 
  return 48;
}

int DG680_Emulator::GetVideoMemSize_k()
{
  return 2;
}

uint16_t DG680_Emulator::GetStartAddress() const
{
  return DG680_ROM_START_ADDR;
}

void DG680_Emulator::GetScreenSizePixels(int & x, int & y) const
{
  x = DG640_SCREEN_WIDTH_PIXELS;
  y = DG640_SCREEN_HEIGHT_PIXELS;
}

void DG680_Emulator::GetScreenSizeChars(int & x, int & y) const
{
  x = DG640_SCREEN_WIDTH_CHARS;
  y = DG640_SCREEN_HEIGHT_CHARS;
}

double DG680_Emulator::GetTargetClockSpeed_Hz() const
{
  return 4000000;
}

bool DG680_Emulator::Open(const Options & options)
{
  return Z80Emulator::Open(options);
}

bool DG680_Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!Z80Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  InitializeDG640();

  return true;
}

void DG680_Emulator::WriteMemory(register uint16_t addr, register uint8_t val)
{
  if (addr < m_ramSize_bytes) {
    m_ram[addr & m_ramMask] = val;
    return;
  }

  if ((addr >= DG680_RAM_START_ADDR) && (addr <= DG680_RAM_END_ADDR)) {
    m_dgosRAM[addr - DG680_RAM_START_ADDR] = val;
    return;
  }

  if ((addr >= DG640_VIDEO_START_ADDR) && (addr <= DG640_VIDEO_END_ADDR)) {
    unsigned offset = addr - DG640_VIDEO_START_ADDR;  
    if (m_videoRAM[offset] != val) {
      m_videoRAM[offset] = val;
      WriteVideoChar(offset, val);
    }
    return;
  }

  if ((addr >= DG680_ROM_START_ADDR) && (addr <= DG680_ROM_END_ADDR)) {
    return;
  }

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t DG680_Emulator::ReadMemory(register uint16_t addr)
{
  if (addr < m_ramSize_bytes) {
    return m_ram[addr & m_ramMask];
  }

  if ((addr >= DG680_ROM_START_ADDR) && (addr <= DG680_ROM_END_ADDR)) {
    return m_rom[addr - DG680_ROM_START_ADDR];
  }

  if ((addr >= DG680_RAM_START_ADDR) && (addr <= DG680_RAM_END_ADDR)) {
    return m_dgosRAM[addr - DG680_RAM_START_ADDR];
  }

  if ((addr >= DG640_VIDEO_START_ADDR) && (addr <= DG640_VIDEO_END_ADDR)) {
    return m_videoRAM[addr - DG640_VIDEO_START_ADDR];
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}
