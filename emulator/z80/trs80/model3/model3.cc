#include <iostream>
#include <iomanip>

#include "model3.h"
#include "chargen_model3.h"

using namespace std;

extern uint8_t g_model3ROM[14336];  

INFO_START(model3)
{
  INFO_CPU(MODEL3_CLOCK_SPEED, 0x0000),

  INFO_ROM(0x0000, g_model3ROM),
  INFO_MAIN_RAM(MODEL1_RAM_START_ADDR, 16, 16, 48),

  INFO_MONITOR(10.6445, 4.0, 3.0, ePAL),        

  INFO_SCREEN_MEMORY_MAPPED_FIXED("trs80", \
                            MODEL1_VIDEO_START_ADDR, MODEL1_VIDEO_END_ADDR, \
                            MODEL3_SCREEN_WIDTH_CHARS, MODEL3_SCREEN_HEIGHT_CHARS, \
                            MODEL3_FONT_WIDTH, MODEL3_FONT_HEIGHT, \
                            256, &g_charGen_Model3, TRS80Emulator::CreatePixelFont)
}
INFO_END(model3)

static struct EmulatorInfo g_emulatorInfo
{
  "m3",                           // command line option
  "Model 3",                      // short name
  "TRS-80 Model 3",               // long name

  INFO_INSERT(model3)
};

Model3_Emulator::Model3_Emulator()
  : Model1_Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead

}
