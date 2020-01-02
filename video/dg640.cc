#include <iostream>

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

void CreateDG640PixelData(const Options & options, const EmulatorInfo::FontInfo & fontInfo, std::vector<uint8_t> & fontData)
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
