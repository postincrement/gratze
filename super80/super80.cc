

#include <iostream>
#include <iomanip>

#include "super80.h"
#include "video/dg640.h"

using namespace std;

#define   SUPER80_RAM_START_ADDR    0x0000
#define   SUPER80_RAM_END_ADDR      0xbfff

#define   SUPER80_ROM_U26_START_ADDR    0xc000
#define   SUPER80_ROM_U26_END_ADDR      0xcfff

#define   SUPER80_ROM_U33_START_ADDR    0xd000
#define   SUPER80_ROM_U33_END_ADDR      0xdfff

#define   SUPER80_ROM_U42_START_ADDR    0xe000
#define   SUPER80_ROM_U42_END_ADDR      0xefff

#define   SUPER80_VIDEO_START_ADDR  0xf000
#define   SUPER80_VIDEO_END_ADDR    0xf7ff

extern unsigned char g_dgos680_1_4ROM[DG680_ROM_END_ADDR - DG680_ROM_START_ADDR + 1];

static EmulatorInfo g_emulatorInfo = 
{
  "super80",                      // command line option
  "Super-80",                     // short name
  "Dicksmith Super80",            // long name

  {
    INFO_CPU(4, DG680_ROM_START_ADDR),

    INFO_ROM(DG680_ROM_START_ADDR, g_dgos680_1_4ROM),

    INFO_MAIN_RAM(0x0000, 48, 8, DG680_ROM_START_ADDR / 1024),
    INFO_RAM(DG680_RAM_START_ADDR, DG680_RAM_END_ADDR),

    DG640_VIDEO_DRIVER(DG680_VIDEO_START_ADDR),

    INFO_END()
  }
};


DG680_Emulator::DG680_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
}
