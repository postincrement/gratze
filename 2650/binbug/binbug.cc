

#include <iostream>
#include <iomanip>

#include "binbug.h"
#include "video/dg640.h"

using namespace std;

#define   BINBUG_START_ADDR     0x0000
#define   BINBUG_END_ADDR       0x03ff

#define   BINBUG_RAM_START_ADDR 0x0400
#define   BINBUG_RAM_END_ADDR   0x77ff

#define   BINBUG_VIDEO_START_ADDR   0x7800
#define   BINBUG_VIDEO_END_ADDR     0x7fff

extern unsigned char g_rom_binbug6_1ROM[2048];

static EmulatorInfo g_emulatorInfo =
{
  "binbug",                       // command line option
  "BINBUG",                       // short name
  "Signetics 2650 with BINBUG",   // long name

  {
    INFO_CPU(1, 0x0000),
    INFO_ROM(0x0000, g_rom_binbug6_1ROM),

    INFO_MAIN_RAM(BINBUG_RAM_START_ADDR, 16, 1, 32 - 1 - DG640_VIDEO_RAM_SIZE_K),
    INFO_RAM(BINBUG_RAM_START_ADDR, BINBUG_RAM_END_ADDR),

    DG640_VIDEO_DRIVER(BINBUG_VIDEO_START_ADDR),

    INFO_END()
  }
};

BINBUG_2650::BINBUG_2650()
  : S2650Emulator(&g_emulatorInfo)
{
}
