#ifndef _CHARGEN_ROM_H
#define _CHARGEN_ROM_H

#include <stdint.h>

struct CharacterGeneratorROM
{
  const char * m_name;
  int m_count;         // number of chars
  int m_width;         // width of char in pixels
  int m_height;        // height of char in pixels
  int m_stride;        // height of each character data block
  const uint8_t * m_data;    // font data - must be stride * count
  int m_reverse;       // if non-zero, font is reversed
};

#endif // _CHARGEN_ROM_H
