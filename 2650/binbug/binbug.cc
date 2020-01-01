

#include <iostream>
#include <iomanip>

#include "binbug.h"
#include "video/dg640.h"

using namespace std;

extern unsigned char g_rom_binbug6_1ROM[2048];

#define   BINBUG_START_ADDR     0x0000
#define   BINBUG_END_ADDR       0x03ff

#define   BINBUG_RAM_START_ADDR 0x0400
#define   BINBUG_RAM_END_ADDR   0x77ff

#define   BINBUG_DG640_START_ADDR   0x7800
#define   BINBUG_DG640_END_ADDR     0x7fff


BINBUG_2650::BINBUG_2650()
{
  m_rom = g_rom_binbug6_1ROM;
  m_romSize = sizeof(g_rom_binbug6_1ROM);
}

std::string BINBUG_2650::GetTitle() const
{ 
  return "BINBUG 2650"; 
}

int BINBUG_2650::GetDefaultRAMSize_k() const
{ 
  return 32 - 1 - 2;
}

int BINBUG_2650::GetVideoMemSize_k()
{
  return 2;
}

void BINBUG_2650::GetScreenSizePixels(int & x, int & y) const
{
  x = DG6480_SCREEN_WIDTH_PIXELS;
  y = DG6480_SCREEN_HEIGHT_PIXELS;
}

int BINBUG_2650::GetScreenSizeChars(int & x, int & y) const
{
  x = DG6480_SCREEN_WIDTH_CHARS;
  y = DG6480_SCREEN_HEIGHT_CHARS;
}

double BINBUG_2650::GetTargetClockSpeed_Hz() const
{
  return 1000000;
}

bool BINBUG_2650::Open(const Options & options)
{
  return S2650Emulator::Open(options);
}

bool BINBUG_2650::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!S2650Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  InitializeDG640();
  return true;
}

void BINBUG_2650::WriteMemory(register uint16_t addr, register uint8_t val)
{
  if ((addr >= BINBUG_RAM_START_ADDR) && (addr <= BINBUG_RAM_END_ADDR)) {
    m_ram[addr & m_ramMask] = val;
    return;
  }

  if ((addr >= BINBUG_DG640_START_ADDR) && (addr <= BINBUG_DG640_END_ADDR)) {
    unsigned offset = addr - BINBUG_DG640_START_ADDR;  
    if (m_videoRAM[offset] != val) {
      m_videoRAM[offset] = val;
      WriteVideoChar(offset, val);
    }
    return;
  }

/*
  if ((addr >= DG680_ROM_START_ADDR) && (addr <= DG680_ROM_END_ADDR)) {
    return;
  }
*/

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t BINBUG_2650::ReadMemory(register uint16_t addr)
{
  if ((addr >= BINBUG_START_ADDR) && (addr <= BINBUG_END_ADDR)) {
    return m_rom[addr - BINBUG_START_ADDR];
  }

  if ((addr >= BINBUG_RAM_START_ADDR) && (addr <= BINBUG_RAM_END_ADDR)) {
    return m_ram[addr & m_ramMask];
  }

  if ((addr >= BINBUG_DG640_START_ADDR) && (addr <= BINBUG_DG640_END_ADDR)) {
    return m_videoRAM[addr - BINBUG_DG640_START_ADDR];
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}
