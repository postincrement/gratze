#include "model1.h"

#include <iostream>
#include <iomanip>

using namespace std;

extern uint8_t g_trs80CharSets_128[][128][MODEL1_FONT_HEIGHT];

Model1_Emulator::Model1_Emulator(EmulatorInfo * info)
  : TRS80Emulator(info)
{
}

bool Model1_Emulator::Open(const Options & options)
{
  m_withEI = options.m_withEI;

  cout << "info: expansion interface is " << (m_withEI ? "en" : "dis") << "abled" << endl; 

  m_rtcEnabled = m_withEI;
  m_fdcEnabled = m_withEI;

  return TRS80Emulator::Open(options);
}

void Model1_Emulator::WriteMemory(register uint16_t addr, register uint8_t val)
{
  if (addr >= MODEL1_RAM_START_ADDR) {
    m_ram[(addr - MODEL1_RAM_START_ADDR) & m_ramMask] = val;
    return;
  }

  if ((addr >= MODEL1_VIDEO_START_ADDR) && (addr <= MODEL1_VIDEO_END_ADDR)) {
    if (val < 0x20)
      val += 0x40;
    unsigned offset = addr - MODEL1_VIDEO_START_ADDR;  
    if (m_videoRAM[offset] != val) {
      m_videoRAM[offset] = val;
      WriteVideoChar(offset, val);
    }
    return;
  }

  if ((addr >= MODEL1_MEMIO_START_ADDR) && (addr <= MODEL1_MEMIO_END_ADDR)) {
    if ((addr & 0xfff0) == 0x37e0) {
      int reg = addr & 0x000f;
      if (reg >= 0xc)  
        WriteFDC(addr, val);
      else if (reg == 1)
        WriteDrvSel(addr, val);
      else if (reg == 8)  
        WritePrinter(addr, val);
      else
        WriteLog(addr, val);
    }
    else
      WriteLog(addr, val);
    return;
  }

  //if ((addr >= MODEL1_KB_START_ADDR) && (addr <= MODEL1_KB_END_ADDR)) {
  //  return;
  //}

  //if (addr <= MODEL1_L2_ROM_END_ADDR) {
  //  return;
  //}

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t Model1_Emulator::ReadMemory(register uint16_t addr)
{
  if (addr <= m_romSize_bytes) {
    return m_rom[addr];
  }

  if (addr >= MODEL1_RAM_START_ADDR) {
    return m_ram[(addr - MODEL1_RAM_START_ADDR) & m_ramMask];
  }

  if ((addr >= MODEL1_VIDEO_START_ADDR) && (addr <= MODEL1_VIDEO_END_ADDR)) {
    return m_videoRAM[addr - MODEL1_VIDEO_START_ADDR];
  }

  if ((addr >= MODEL1_KB_START_ADDR) && (addr <= MODEL1_KB_END_ADDR)) {
    return ReadKeyboard(addr);
  }

  if ((addr >= MODEL1_MEMIO_START_ADDR) && (addr <= MODEL1_MEMIO_END_ADDR)) {
    if ((addr & 0xfff0) == 0x37e0) {
      int reg = addr & 0x000f;
      if (reg >= 0xc)  
        return ReadFDC(addr);
      else if (reg == 0)
        return ReadInterrupt(addr);  
      else if (reg == 1)
        return ReadDrvSel(addr);
      else if (reg == 8)  
        return ReadPrinter(addr);
      else
        return ReadLog(addr);
    }
    else
      return ReadLog(addr);
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}

/////////////////////////////////////////////////////////////

extern unsigned char g_model1Level1ROM[4096];

static struct EmulatorInfo g_level1EmulatorInfo
{
  "m1",                           // command line option
  "Model 1 L1",                   // short name
  "TRS-80 Model 1, Level 1",      // long name

  INFO_CPU(MODEL1_CLOCK_SPEED, 0x0000),
  INFO_VIDEO_NONE(),
  #if 0
  INFO_VIDEO_MEMORY_MAPPED(1, MODEL1_VIDEO_START_ADDR, \
                           MODEL1_SCREEN_WIDTH_CHARS, MODEL1_SCREEN_HEIGHT_CHARS, \
                           MODEL1_FONT_WIDTH, MODEL4_FONT_HEIGHT, \
                           256, g_trs80CharSets_128[1][0], TRS80Emulator::CreatePixelFont),
#endif
  INFO_ROM_NONE(),
  //INFO_ROM(0x0000, g_model1Level1ROM),
  //INFO_RAM(4, 4, 16)
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

  INFO_CPU(MODEL1_CLOCK_SPEED, 0x0000),

  INFO_VIDEO_MEMORY_MAPPED(1, MODEL1_VIDEO_START_ADDR, \
                           MODEL1_SCREEN_WIDTH_CHARS, MODEL1_SCREEN_HEIGHT_CHARS, \
                           MODEL1_FONT_WIDTH, MODEL1_FONT_HEIGHT, \
                           256, g_trs80CharSets_128[1][0], TRS80Emulator::CreatePixelFont),

  INFO_ROM(0x0000, g_model1Level2ROM),
  INFO_RAM(48, 4, 48)
};

Model1Level2_Emulator::Model1Level2_Emulator()
  : Model1_Emulator(&g_levelEmulatorInfo)
{
}

