#include <iostream>
#include <memory.h>

#include "misc.h"
#include "cpu/emulator.h"
#include "video/virtual_screen.h"
#include "dg640.h"

using namespace std;

extern unsigned char g_dg640Char_ROM[1024];

DG640::DG640(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : SingleColourMemoryMappedVideo(mainWindow, emulator, options, info)
{
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);
}

bool DG640::CreatePixelFont(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  // create inverted chars
  {
    uint8_t * src = &fontData[0];
    uint8_t * dst = &fontData[128 * DG640_FONT_HEIGHT];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i)
      *dst++ = *src++ ^ 0xff;
  }

  return true;
}

void DG640::WriteMemoryAtPos(int pos, uint8_t ch)
{
  if ((pos >= m_memory.size()) || (m_memory[pos] == ch)) {
    return;
  }

  m_memory[pos] = ch;

  if (pos >= 0x400) {
    if ((ch & 0xf)!= 0)
      cout << "dg640: video attribute set at " << HEXFORMAT0x4(pos) << " to " << HEXFORMAT0x2(ch) << endl;
  }

  RefreshCharAtPos(pos);
}
