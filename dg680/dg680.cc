
#include "dg680.h"

#include <iostream>
#include <iomanip>

using namespace std;

extern unsigned char g_dgos680_1_4ROM[2048];
extern unsigned char g_dg640Char_ROM[1024];


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

int DG680_Emulator::GetScreenWidth() const
{
  return DG6480_SCREEN_WIDTH;
}

int DG680_Emulator::GetScreenHeight() const
{
  return DG6480_SCREEN_HEIGHT;
}

double DG680_Emulator::GetTargetClockSpeed_Hz() const
{
  return 4000000;
}

bool DG680_Emulator::Open(const Options & options)
{
  return Z80Emulator::Open(options);
}

static unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

bool DG680_Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!Z80Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  // set the base font
  m_fontData.resize(DG640_FONT_HEIGHT * 256);
  memcpy(&m_fontData[0], g_dg640Char_ROM, 128 * DG640_FONT_HEIGHT);

  // reverse bits
  {
    uint8_t * ptr = &m_fontData[0];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i) {
      *ptr = reverse(*ptr);
      ++ptr;
    }
  }

  // create inverted chars
  {
    uint8_t * src = &m_fontData[0];
    uint8_t * dst = &m_fontData[128 * DG640_FONT_HEIGHT];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i)
      *dst++ = *src++ ^ 0xff;
  }

  m_video->SetFont(new PixelFont(256, DG640_FONT_WIDTH, DG640_FONT_HEIGHT, &m_fontData[0]));

  return true;
}

void DG680_Emulator::WrZ80(register uint16_t addr, register uint8_t val)
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

uint8_t DG680_Emulator::RdZ80(register uint16_t addr)
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
