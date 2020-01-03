#include <iostream>
#include <memory.h>

#include "misc.h"
#include "cpu/emulator.h"
#include "video/virtual_screen.h"
#include "dg640.h"

using namespace std;

extern unsigned char g_dg640Char_ROM[1024];

static unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

DG640::DG640(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : SingleColourMemoryMappedVideo(mainWindow, emulator, options, info)
{
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);
}

void DG640::CreatePixelFont(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  // set the base font
  fontData.resize(DG640_FONT_HEIGHT * 256);
  memcpy(&fontData[0], g_dg640Char_ROM, 128 * DG640_FONT_HEIGHT);

  // reverse bits
  {
    uint8_t * ptr = &fontData[0];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i) {
      *ptr = reverse(*ptr);
      ++ptr;
    }
  }

  // create inverted chars
  {
    uint8_t * src = &fontData[0];
    uint8_t * dst = &fontData[128 * DG640_FONT_HEIGHT];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i)
      *dst++ = *src++ ^ 0xff;
  }
}

void DG640::WriteMemory(int addr, uint8_t ch)
{
  if ((addr >= m_memory.size()) || (m_memory[addr] == ch)) {
    return;
  }

  m_memory[addr] = ch;

  if (addr >= 0x400) {
    if ((ch & 0xf)!= 0)
      cout << "dg640: video attribute set at " << HEXFORMAT0x4(addr) << " to " << HEXFORMAT0x2(ch) << endl;
  }

  RefreshChar(addr);
}
