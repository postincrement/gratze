

#include <iostream>
#include <iomanip>

#include "dg680.h"
#include "video/dg640.h"

using namespace std;

#define   DG680_ROM_START_ADDR    0xd000
#define   DG680_ROM_END_ADDR      0xd7ff

#define   DG680_RAM_START_ADDR    0xd800
#define   DG680_RAM_END_ADDR      0xdfff

#define   DG680_VIDEO_START_ADDR  0xf000
#define   DG680_VIDEO_END_ADDR    0xf7ff

extern unsigned char g_dgos680_1_4ROM[DG680_ROM_END_ADDR - DG680_ROM_START_ADDR + 1];

static EmulatorInfo g_emulatorInfo = 
{
  "dg680",                        // command line option
  "DG-680",                       // short name
  "DG-680 with DGOS",             // long name

  {
    INFO_CPU(4, DG680_ROM_START_ADDR),

    INFO_ROM(DG680_ROM_START_ADDR, g_dgos680_1_4ROM),

    INFO_MAIN_RAM(0x0000, 48, 8, DG680_ROM_START_ADDR / 1024),
    INFO_RAM(DG680_RAM_START_ADDR, DG680_RAM_END_ADDR),

    DG640_VIDEO_DRIVER(DG680_VIDEO_START_ADDR),

    INFO_END()
  }
};

void DG680_Emulator::Init()
{  
  VirtualScreen::AddType<DG640>("dg640");
}

DG680_Emulator::DG680_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
}
