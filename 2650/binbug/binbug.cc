

#include <iostream>
#include <iomanip>

#include "binbug.h"
#include "video/dg640.h"

using namespace std;

#define   BINBUG_START_ADDR     0x0000
#define   BINBUG_END_ADDR       0x03ff

#define   BINBUG_RAM_START_ADDR 0x0400
#define   BINBUG_RAM_END_ADDR   0x77ff

#define   BINBUG_VIDEO_START_ADDR   0x7800
#define   BINBUG_VIDEO_END_ADDR     0x7fff

extern unsigned char g_rom_binbug6_1ROM[2048];

static EmulatorInfo g_emulatorInfo =
{
  "binbug",                       // command line option
  "BINBUG",                       // short name
  "Signetics 2650 with BINBUG",   // long name

  1,                              // nominal CPU clock speed
  0x0000,                         // address to start when reset

  DG640_VIDEO_DRIVER(BINBUG_VIDEO_START_ADDR),
  
  DEFINE_ROM(0x0000, g_rom_binbug6_1ROM),

  4,                                // default RAM size, in k
  1,                                // min RAM size, in k
  32 - 1 - DG640_VIDEO_RAM_SIZE_K,  // max RAM size, in k
};

BINBUG_2650::BINBUG_2650()
  : S2650Emulator(&g_emulatorInfo)
{
}

bool BINBUG_2650::Open(const Options & options)
{
  return S2650Emulator::Open(options);
}

void BINBUG_2650::WriteMemory(register uint16_t addr, register uint8_t val)
{
  if ((addr >= BINBUG_RAM_START_ADDR) && (addr <= BINBUG_RAM_END_ADDR)) {
    m_ram[addr & m_ramMask] = val;
    return;
  }

  if ((addr >= BINBUG_VIDEO_START_ADDR) && (addr <= BINBUG_VIDEO_END_ADDR)) {
    unsigned offset = addr - BINBUG_VIDEO_START_ADDR;  
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

  if ((addr >= BINBUG_VIDEO_START_ADDR) && (addr <= BINBUG_VIDEO_END_ADDR)) {
    return m_videoRAM[addr - BINBUG_VIDEO_START_ADDR];
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}
