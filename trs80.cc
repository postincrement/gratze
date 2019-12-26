#include "trs80.h"

#include <functional>
#include <iostream>

extern "C" {
#include "trs_chars.c"
};

using namespace std;

/////////////////////////////////////////////////////////////

TRS80Emulator::TRS80Emulator()
{}

bool TRS80Emulator::Open(int argc, char *argv[])
{
  const char * romFn = "roms/trs80_level2.rom";
  if (argc > 1) 
    romFn = argv[1];

  // load ROM
  if (!ReadROMFromFile(romFn, m_rom))
    return false;

  return Emulator::Open(argc, argv);
}

bool TRS80Emulator::Start(uint16_t addr)
{
  m_font.reset(new MemoryMappedVideo::Font(6, 12, &trs_char_data[0][0][0]));
  if (!OpenVideo(16, 64, m_font.get())) {
    return -1;
  }

  return Emulator::Start(addr);
}

/////////////////////////////////////////////////////////////

typedef byte (TRS80Emulator::* ReadMemoryFn)(uint16_t);
typedef void (TRS80Emulator::* WriteMemoryFn)(uint16_t, uint8_t);

uint8_t TRS80Emulator::ReadROM(uint16_t addr)
{
  return m_rom[addr];
}

uint8_t TRS80Emulator::ReadRAM(uint16_t addr)
{
  return m_ram[addr - 0x4000];
}

void TRS80Emulator::WriteRAM(uint16_t addr, uint8_t val)
{
  m_ram[addr - 0x4000] = val;
}

uint8_t TRS80Emulator::ReadVideo(uint16_t addr)
{
  return m_videoRAM[addr - 0x3c00];
}

void TRS80Emulator::WriteVideo(uint16_t addr, uint8_t val)
{
  uint16_t offset = addr - 0x3c00;

  if (val < 0x20)
    val += 0x40;

  m_videoRAM[offset] = val;
  WriteVideoChar(offset, val);
}

static WriteMemoryFn g_trs80WriteIO[16] = {
  &TRS80Emulator::WriteNull,     // 0x3000 to 0x30ff
  &TRS80Emulator::WriteNull,     // 0x3100 to 0x31ff
  &TRS80Emulator::WriteNull,     // 0x3200 to 0x32ff
  &TRS80Emulator::WriteNull,     // 0x3300 to 0x33ff
  &TRS80Emulator::WriteRAM,      // 0x3400 to 0x34ff
  &TRS80Emulator::WriteRAM,      // 0x3500 to 0x35ff
  &TRS80Emulator::WriteRAM,      // 0x3600 to 0x36ff
  &TRS80Emulator::WriteRAM,      // 0x3700 to 0x37ff
  &TRS80Emulator::WriteRAM,      // 0x3800 to 0x38ff
  &TRS80Emulator::WriteRAM,      // 0x3900 to 0x39ff
  &TRS80Emulator::WriteRAM,      // 0x3a00 to 0x3aff
  &TRS80Emulator::WriteRAM,      // 0x3b00 to 0x3bff
  &TRS80Emulator::WriteVideo,    // 0x3c00 to 0x3cff
  &TRS80Emulator::WriteVideo,    // 0x3d00 to 0x3dff
  &TRS80Emulator::WriteVideo,    // 0x3e00 to 0x3eff
  &TRS80Emulator::WriteVideo,    // 0x3f00 to 0x3fff
};

static ReadMemoryFn g_trs80ReadIO[16] = {
  &TRS80Emulator::ReadNull,     // 0x3000 to 0x30ff
  &TRS80Emulator::ReadNull,     // 0x3100 to 0x31ff
  &TRS80Emulator::ReadNull,     // 0x3200 to 0x32ff
  &TRS80Emulator::ReadNull,     // 0x3300 to 0x33ff
  &TRS80Emulator::ReadNull,     // 0x3400 to 0x34ff
  &TRS80Emulator::ReadNull,     // 0x3500 to 0x35ff
  &TRS80Emulator::ReadNull,     // 0x3600 to 0x36ff
  &TRS80Emulator::ReadNull,     // 0x3700 to 0x37ff
  &TRS80Emulator::ReadNull,     // 0x3800 to 0x38ff
  &TRS80Emulator::ReadNull,     // 0x3900 to 0x39ff
  &TRS80Emulator::ReadNull,     // 0x3a00 to 0x3aff
  &TRS80Emulator::ReadNull,     // 0x3b00 to 0x3bff
  &TRS80Emulator::ReadVideo,    // 0x3c00 to 0x3cff
  &TRS80Emulator::ReadVideo,    // 0x3d00 to 0x3dff
  &TRS80Emulator::ReadVideo,    // 0x3e00 to 0x3eff
  &TRS80Emulator::ReadVideo,    // 0x3f00 to 0x3fff
};

void TRS80Emulator::WriteIO(uint16_t addr, uint8_t value)
{
  return std::invoke(g_trs80WriteIO[(addr & 0x0f00) >> 8], *this, addr, value);
}

uint8_t TRS80Emulator::ReadIO(uint16_t addr)
{
  return std::invoke(g_trs80ReadIO[(addr & 0x0f00) >> 8], *this, addr);
}

static ReadMemoryFn g_trs80ReadMemory[16] = {
  &TRS80Emulator::ReadROM,   // 0x0000 to 0x0fff
  &TRS80Emulator::ReadROM,   // 0x1000 to 0x1fff
  &TRS80Emulator::ReadROM,   // 0x2000 to 0x2fff
  &TRS80Emulator::ReadIO,    // 0x3000 to 0x3fff
  &TRS80Emulator::ReadRAM,   // 0x4000 to 0x4fff
  &TRS80Emulator::ReadRAM,   // 0x5000 to 0x5fff
  &TRS80Emulator::ReadRAM,   // 0x6000 to 0x6fff
  &TRS80Emulator::ReadRAM,   // 0x7000 to 0x7fff
  &TRS80Emulator::ReadRAM,   // 0x8000 to 0x8fff
  &TRS80Emulator::ReadRAM,   // 0x9000 to 0x9fff
  &TRS80Emulator::ReadRAM,   // 0xa000 to 0xafff
  &TRS80Emulator::ReadRAM,   // 0xb000 to 0xbfff
  &TRS80Emulator::ReadRAM,   // 0xc000 to 0xcfff
  &TRS80Emulator::ReadRAM,   // 0xd000 to 0xdfff
  &TRS80Emulator::ReadRAM,   // 0xe000 to 0xefff
  &TRS80Emulator::ReadRAM,   // 0xf000 to 0xffff
};

static WriteMemoryFn g_trs80WriteMemory[16] = {
  &TRS80Emulator::WriteNull,   // 0x0000 to 0x0fff
  &TRS80Emulator::WriteNull,   // 0x1000 to 0x1fff
  &TRS80Emulator::WriteNull,   // 0x2000 to 0x2fff
  &TRS80Emulator::WriteIO,     // 0x3000 to 0x3fff
  &TRS80Emulator::WriteRAM,    // 0x4000 to 0x4fff
  &TRS80Emulator::WriteRAM,    // 0x5000 to 0x5fff
  &TRS80Emulator::WriteRAM,    // 0x6000 to 0x6fff
  &TRS80Emulator::WriteRAM,    // 0x7000 to 0x7fff
  &TRS80Emulator::WriteRAM,    // 0x8000 to 0x8fff
  &TRS80Emulator::WriteRAM,    // 0x9000 to 0x9fff
  &TRS80Emulator::WriteRAM,    // 0xa000 to 0xafff
  &TRS80Emulator::WriteRAM,    // 0xb000 to 0xbfff
  &TRS80Emulator::WriteRAM,    // 0xc000 to 0xcfff
  &TRS80Emulator::WriteRAM,    // 0xd000 to 0xdfff
  &TRS80Emulator::WriteRAM,    // 0xe000 to 0xefff
  &TRS80Emulator::WriteRAM,    // 0xf000 to 0xffff
};

/////////////////////////////////////////////////////////////

void TRS80Emulator::WrZ80(register uint16_t addr,register uint8_t value)
{
  return std::invoke(g_trs80WriteMemory[(addr & 0xf000) >> 12], *this, addr, value);
}

uint8_t TRS80Emulator::RdZ80(register uint16_t addr)
{
  return std::invoke(g_trs80ReadMemory[(addr & 0xf000) >> 12], *this, addr);
}

void TRS80Emulator::OutZ80(register uint16_t port, register uint8_t value)
{
}

uint8_t TRS80Emulator::InZ80(register uint16_t port)
{
  return 0;
}
