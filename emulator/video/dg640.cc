#include <iostream>
#include <memory.h>

#include "common/misc.h"
#include "src/emulator.h"
#include "video/virtual_screen.h"
#include "video/dg640.h"

using namespace std;

// Extended memory:
//
// bit 0 = flashing
// bit 1 = graphics
// bit 2 = PCG
//

// font
//   0x000 - 0x07f   character ROM
//   0x080 - 0x0ff   inverted character ROM
//   0x100 - 0x1ff   graphics

extern unsigned char g_dg640Char_ROM[1024];

DG640::DG640(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : SingleColourMemoryMappedScreen(mainWindow, options, info)
{
  if (m_memory.size() != DG640_VIDEO_RAM_SIZE_K * 1024) {
    cerr << "error: DG640 memory is wrong size - " << m_memory.size() << " instead of " << DG640_VIDEO_RAM_SIZE_K * 1024 << endl;
    exit(-1);
  }
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);
  InitPCG();
}


// CPU access
void DG640::WriteMemoryAtAddress(int addr, uint8_t data)
{
  if (addr >= m_memory.size()) {
    return;
  }

  m_memory[addr] = data;

  int loc = addr;
  if (loc >= 0x400)
    loc -= 0x400;

  RefreshCharAtLoc(loc);
}

// CPU access
uint8_t DG640::ReadMemoryAtAddress(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0xff;
  }

  if (addr < 0x400)
    return m_memory[addr];

  return m_memory[addr] | 0xf8;
}


FontChar DG640::GetCharAtLoc(int loc) const
{
  int charAddr = loc & 0x3ff;
  FontChar ch = m_memory[charAddr];

  uint8_t attr = m_memory[0x400 + charAddr];

  // switch to graphics 
  ch += ((attr & 0x2) != 0) ? 0x100 : 0x000;

  return ch;
}

bool DG640::CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  // basic font data already in memory, but check size
  if (fontData.size() != (DG640_FONT_HEIGHT*DG640_VIRTUAL_FONT_CHARS)) {
    cerr << "error: DG640 font data is wrong size - " << fontData.size() << " instead of " << DG640_FONT_HEIGHT*256 << endl;
    exit(1);
  }

  // create inverted chars
  {
    uint8_t * src = &fontData[0];
    uint8_t * dst = &fontData[128 * fontInfo.m_height];
    for (int i = 0; i < 128*fontInfo.m_height; ++i)
      *dst++ = *src++ ^ 0xff;
  }

  // set graphics
  uint8_t maskRight = (1 << (fontInfo.m_width / 2)) - 1;
  uint8_t maskLeft  = maskRight << (fontInfo.m_width / 2);

  for (int i = 0; i < 256; ++i) {
    uint8_t * dst = &fontData[(256 + i) * fontInfo.m_height];
    uint8_t val = i;
    for (int y = 0; y < 4; ++y) {
      *dst = 0;
      if (val & 1)
        *dst |= maskRight;
      if (val & 2)
        *dst |= maskLeft;
      for (int z = 1; z < (fontInfo.m_height / 4); ++z)
        dst[z] = dst[0];  
      dst += fontInfo.m_height / 4;
      val = val >> 2;  
    }
  }

  cerr << "dg640: created font" << endl;

  return true;
}
