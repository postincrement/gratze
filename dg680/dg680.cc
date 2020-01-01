

#include <iostream>
#include <iomanip>

#include "dg680.h"
#include "video/dg640.h"

using namespace std;

#define   DG680_ROM_START_ADDR    0xd000
#define   DG680_ROM_END_ADDR      0xd7ff

#define   DG680_RAM_START_ADDR    0xd800
#define   DG680_RAM_END_ADDR      0xdfff

#define   DG680_VIDEO_START_ADDR  0xf000
#define   DG680_VIDEO_END_ADDR    0xf7ff

extern unsigned char g_dgos680_1_4ROM[DG680_ROM_END_ADDR - DG680_ROM_START_ADDR + 1];

static EmulatorInfo g_emulatorInfo = 
{
  "dg680",                        // command line option
  "DG-680",                       // short name
  "DG-680 with DGOS",             // long name

  4,                              // nominal CPU clock speed
  DG680_ROM_START_ADDR,           // address to start when reset

  DG640_VIDEO_DRIVER(DG680_VIDEO_START_ADDR),

  DEFINE_ROM(DG680_ROM_START_ADDR, g_dgos680_1_4ROM),

  48,                             // default RAM size, in k
  8,                              // min RAM size, in k
  DG680_ROM_START_ADDR / 1024,    // max RAM size, in k
};


DG680_Emulator::DG680_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
}

bool DG680_Emulator::Open(const Options & options)
{
  return Z80Emulator::Open(options);
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

  if ((addr >= DG680_VIDEO_START_ADDR) && (addr <= DG680_VIDEO_END_ADDR)) {
    unsigned offset = addr - DG680_VIDEO_START_ADDR;  
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

  if ((addr >= DG680_VIDEO_START_ADDR) && (addr <= DG680_VIDEO_END_ADDR)) {
    return m_videoRAM[addr - DG680_VIDEO_START_ADDR];
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}
