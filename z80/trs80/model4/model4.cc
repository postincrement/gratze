#include "model4.h"
#include "z80/trs80/model3/chargen_model3.h"

static struct EmulatorInfo g_emulatorInfo
{
  "m4",                           // command line option
  "Model 4",                      // short name
  "TRS-80 Model 4",               // long name

  {
    INFO_CPU(MODEL4_CLOCK_SPEED, 0x0000),

    INFO_MAIN_RAM(MODEL1_RAM_START_ADDR, 16, 16, 48),

    INFO_VIDEO_MEMORY_MAPPED("trs80", \
                              MODEL1_VIDEO_START_ADDR, MODEL1_VIDEO_END_ADDR, \
                              MODEL4_SCREEN_WIDTH_CHARS, MODEL4_SCREEN_HEIGHT_CHARS, \
                              MODEL4_FONT_WIDTH, MODEL4_FONT_HEIGHT, \
                              256, &g_charGen_Model3, TRS80Emulator::CreatePixelFont),
    INFO_END()
  }
};


Model4_Emulator::Model4_Emulator()
  : TRS80Emulator(&g_emulatorInfo)
{
}





