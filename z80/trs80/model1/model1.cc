#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "z80/trs80/model1/model1.h"

using namespace std;

Model1_Emulator::Model1_Emulator(const EmulatorInfo * info)
  : TRS80Emulator(info)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Model1_Emulator::Open(const Options & options)
{
  if (!TRS80Emulator::Open(options))
    return false;

  m_withEI = options.m_withEI;

  cout << "info: expansion interface is " << (m_withEI ? "en" : "dis") << "abled" << endl; 

  m_rtcEnabled = m_withEI;
  m_fdcEnabled = m_withEI;

  return true;
}

void Model1_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  int reg = addr & 0x000f;

  if (reg >= 0xc)  
    WriteFDC(addr, val);
  else if (reg == 1)
    WriteDrvSel(addr, val);
  else if (reg == 8)  
    WritePrinter(addr, val);
  else  
    cerr << "TRS WRITE MEM IO " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(val) << endl;
}

uint8_t Model1_Emulator::ReadIOMemory(int id, uint16_t addr)
{
  if (id == 2)
    return ReadKeyboard(addr);
    
  int reg = addr & 0x000f;
  if (reg >= 0xc)  
    return ReadFDC(addr);
  else if (reg == 0)
    return ReadInterrupt(addr);  
  else if (reg == 1)
    return ReadDrvSel(addr);
  else if (reg == 8)  
    return ReadPrinter(addr);

  cerr << "TRS READ MEM IO " << HEXFORMAT0x4(addr)<< endl;
  return 0;
}

/////////////////////////////////////////////////////////////

extern unsigned char g_model1Level1ROM[4096];


static struct EmulatorInfo g_level1EmulatorInfo = 
{
  "m1",                           // command line option
  "Model 1 L1",                   // short name
  "TRS-80 Model 1, Level 1",      // long name

  {
    INFO_CPU(MODEL1_CLOCK_SPEED, 0x0000),

    INFO_MEM_IO_READ(0x37e0, 0x37ef,  1),
    INFO_MEM_IO_WRITE(0x37e0, 0x37ef, 1),

    INFO_ROM(0x0000, g_model1Level1ROM),
    INFO_MAIN_RAM(MODEL1_RAM_START_ADDR, 4, 4, 16),
    INFO_VIDEO_MEMORY_MAPPED("trs80", \
                             MODEL1_VIDEO_START_ADDR, MODEL1_VIDEO_END_ADDR, \
                             MODEL1_SCREEN_WIDTH_CHARS, MODEL1_SCREEN_HEIGHT_CHARS, \
                             MODEL1_FONT_WIDTH, MODEL4_FONT_HEIGHT, \
                             256, &g_charGen_MotorolaMCM6674, TRS80Emulator::CreatePixelFont),

    INFO_MEM_IO_READ(MODEL1_KB_START_ADDR, MODEL1_KB_END_ADDR, 2),

    INFO_END()
  }
};

Model1Level1_Emulator::Model1Level1_Emulator()
  : Model1_Emulator(&g_level1EmulatorInfo)
{
}

////////////////////////////////////////////////////////////////

extern unsigned char g_model1Level2ROM[12288];

static struct EmulatorInfo g_levelEmulatorInfo =
{
  "m2",                           // command line option
  "Model 1 L2",                   // short name
  "TRS-80 Model 1, Level 2",      // long name

  {
    INFO_CPU(MODEL1_CLOCK_SPEED, 0x0000),

    INFO_MEM_IO_READ(0x37e0, 0x37ef, 1),
    INFO_MEM_IO_WRITE(0x37e0,0x37ef, 1),

    INFO_ROM(0x0000, g_model1Level2ROM),
    INFO_MAIN_RAM(MODEL1_RAM_START_ADDR, 48, 4, 48),

    INFO_VIDEO_MEMORY_MAPPED("trs80", MODEL1_VIDEO_START_ADDR, MODEL1_VIDEO_END_ADDR, \
                              MODEL1_SCREEN_WIDTH_CHARS, MODEL1_SCREEN_HEIGHT_CHARS, \
                              MODEL1_FONT_WIDTH, MODEL4_FONT_HEIGHT, \
                              256, &g_charGen_MotorolaMCM6674, TRS80Emulator::CreatePixelFont),

    INFO_MONITOR(10.6445, 4.0, 3.0, ePAL),        

    INFO_MEM_IO_READ(MODEL1_KB_START_ADDR, MODEL1_KB_END_ADDR, 2),

    INFO_IO_PORT_RW(0xff, 0xff, 1),

    INFO_END()                  
  }
};

Model1Level2_Emulator::Model1Level2_Emulator()
  : Model1_Emulator(&g_levelEmulatorInfo)
{
}

