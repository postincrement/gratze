#include "trs80.h"

extern "C" {
#include "trs_chars.c"
};


/////////////////////////////////////////////////////////////

TRS80Emulator::TRS80Emulator()
{}

bool TRS80Emulator::Open()
{
  return true;
}

bool TRS80Emulator::Start()
{
  m_font.reset(new MemoryMappedVideo::Font(6, 12, &trs_char_data[0][0][0]));
  if (!OpenVideo(16, 64, m_font.get())) {
    return -1;
  }

  return true;
}
