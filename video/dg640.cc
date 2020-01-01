#include "cpu/emulator.h"
#include "video/virtual_screen.h"
#include "dg640.h"

extern unsigned char g_dg640Char_ROM[1024];

static unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void Emulator::InitializeDG640()
{
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
}