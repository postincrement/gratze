#include <iostream>
#include <memory.h>

#include "src/misc.h"
#include "src/emulator.h"
#include "video/virtual_screen.h"
#include "video/dg640.h"

using namespace std;

extern unsigned char g_dg640Char_ROM[1024];

DG640::DG640(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : SingleColourMemoryMappedVideo(mainWindow, emulator, options, info)
{
  if (m_memory.size() != DG640_VIDEO_RAM_SIZE_K * 1024) {
    cerr << "error: DG640 memory is wrong size - " << m_memory.size() << " instead of " << DG640_VIDEO_RAM_SIZE_K * 1024 << endl;
    exit(-1);
  }
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);
}

bool DG640::CreatePixelFont(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  if (fontData.size() != (DG640_FONT_HEIGHT*256)) {
    cerr << "error: DG640 font data is wrong size - " << fontData.size() << " instead of " << DG640_FONT_HEIGHT*256 << endl;
    exit(1);
  }

  // create inverted chars
  {
    uint8_t * src = &fontData[0];
    uint8_t * dst = &fontData[128 * DG640_FONT_HEIGHT];
    for (int i = 0; i < 128*DG640_FONT_HEIGHT; ++i)
      *dst++ = *src++ ^ 0xff;
  }

  return true;
}

void DG640::WriteMemoryAtAddress(int addr, uint8_t ch)
{
  if ((addr >= m_memory.size()) || (m_memory[addr] == ch)) {
    return;
  }

  m_memory[addr] = ch;

  if (addr >= 0x400) {
    if ((ch & 0xf)!= 0) {
      //cout << "dg640: video attribute set at " << HEXFORMAT0x4(addr) << " to " << HEXFORMAT0x2(ch) << endl;
    }
  }
  else {
    RenderCharAtAddress(addr);
  }
}
