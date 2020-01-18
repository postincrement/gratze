

#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "2650/78up5/78up5.h"

using namespace std;

#define   PIPBUG_START_ADDR     0x0000
#define   PIPBUG_END_ADDR       0x03ff

#define   RAM_START_ADDR        0x0400
#define   RAM_END_ADDR          0x7fff

extern unsigned char g_rom_pipbug[1024];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EmulatorInfo g_emulatorInfo =
{
  "78up5",                                     // command line option
  "78UP5 with PIPBUG",                         // short name
  "Eletroncics Australia 78UP5 with PIPBUG",   // long name

  {
    INFO_CPU(1, 0x0000),
    INFO_ROM(0x0000, g_rom_pipbug),

    INFO_RAM(RAM_START_ADDR, RAM_END_ADDR),

    // serial terminal?

    INFO_END()
  }
};

void EA78UP5_Emulator::Instantiate()
{  
}

EA78UP5_Emulator::EA78UP5_Emulator()
  : S2650Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool EA78UP5_Emulator::Open(const Options & options)
{
  if (!S2650Emulator::Open(options))
    return false;

  return true;
}

void EA78UP5_Emulator::Reset(int addr)
{
  S2650Emulator::Reset(addr);
}

