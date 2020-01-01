#include "model4.h"

extern uint8_t g_trs80CharSets_256[][256][MODEL3_FONT_HEIGHT];

static struct EmulatorInfo g_emulatorInfo
{
  "m4",                           // command line option
  "Model 4",                      // short name
  "TRS-80 Model 4",               // long name

  MODEL4_CLOCK_SPEED,             // nominal CPU clock speed
  0x0000,                         // address to start when reset

  {
    EmulatorInfo::VideoDriver::eExplicit,

    2,                              // video memory size in k
    MODEL4_SCREEN_WIDTH_CHARS,      // screen char cols (X)
    MODEL4_SCREEN_HEIGHT_CHARS,     // screen char rows (Y)

    MODEL4_SCREEN_WIDTH_PIXELS,     // screen width in pixels (X)
    MODEL4_SCREEN_HEIGHT_PIXELS,    // screen height in pixels (Y)

    MODEL4_FONT_WIDTH,              // nominal font width in pixels (X)
    MODEL4_FONT_HEIGHT              // nominal font height in pixels (X)
  },

  //DEFINE_ROM(0x0000, g_model3ROM),
  NO_ROM(),
  
  16,                              // default RAM size, in k
  16,                              // min RAM size, in k
  48                               // max RAM size, in k
};


Model4_Emulator::Model4_Emulator()
  : TRS80Emulator(&g_emulatorInfo)
{
}

bool Model4_Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!Z80Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  // set the font
  CreateFontData(options, MODEL4_FONT_WIDTH, MODEL4_FONT_HEIGHT, g_trs80CharSets_256[7-4][0]);

  return true;
}

void Model4_Emulator::WriteMemory(register uint16_t addr, register uint8_t val)
{}

uint8_t Model4_Emulator::ReadMemory(register uint16_t addr)
{
  return 0;
}





