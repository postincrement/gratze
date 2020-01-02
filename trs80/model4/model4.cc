#include "model4.h"

extern uint8_t g_trs80CharSets_256[][256][MODEL3_FONT_HEIGHT];

static struct EmulatorInfo g_emulatorInfo
{
  "m4",                           // command line option
  "Model 4",                      // short name
  "TRS-80 Model 4",               // long name

  INFO_CPU(MODEL4_CLOCK_SPEED, 0x0000),
  INFO_VIDEO_MEMORY_MAPPED(2, MODEL1_VIDEO_START_ADDR, MODEL4_SCREEN_WIDTH_CHARS, \
                              MODEL4_SCREEN_HEIGHT_CHARS, MODEL4_FONT_WIDTH, \
                              256, MODEL4_FONT_HEIGHT, g_trs80CharSets_256[7-4][0], TRS80Emulator::CreatePixelFont),
  INFO_ROM_NONE(),
  INFO_RAM(16, 16, 48)
};


Model4_Emulator::Model4_Emulator()
  : TRS80Emulator(&g_emulatorInfo)
{
}

void Model4_Emulator::WriteMemory(register uint16_t addr, register uint8_t val)
{}

uint8_t Model4_Emulator::ReadMemory(register uint16_t addr)
{
  return 0;
}





