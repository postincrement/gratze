#include <iostream>
#include <iomanip>

#include "model3.h"

using namespace std;

extern uint8_t g_model3ROM[14336];  

extern uint8_t g_trs80CharSets_256[][256][MODEL3_FONT_HEIGHT];

static struct EmulatorInfo g_emulatorInfo
{
  "m3",                           // command line option
  "Model 3",                      // short name
  "TRS-80 Model 3",               // long name

  INFO_CPU(MODEL3_CLOCK_SPEED, 0x0000),
  INFO_VIDEO_MEMORY_MAPPED(2, MODEL1_VIDEO_START_ADDR, MODEL3_SCREEN_WIDTH_CHARS, \
                              MODEL3_SCREEN_HEIGHT_CHARS, MODEL3_FONT_WIDTH, \
                              256, MODEL3_FONT_HEIGHT, g_trs80CharSets_256[7-4][0], TRS80Emulator::CreatePixelFont),
  INFO_ROM(0x0000, g_model3ROM),
  INFO_RAM(16, 16, 48)
};


Model3_Emulator::Model3_Emulator()
  : Model1_Emulator(&g_emulatorInfo)
{
}

void Model3_Emulator::WriteMemory(register uint16_t addr, register uint8_t val)
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

#if 0
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
#endif

  if ((addr >= MODEL1_KB_START_ADDR) && (addr <= MODEL1_KB_END_ADDR)) {
    return;
  }

  if (addr <= MODEL3_ROM_END_ADDR) {
    return;
  }

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t Model3_Emulator::ReadMemory(register uint16_t addr)
{
  if (addr <= m_romSize_bytes) {
    return m_rom[addr - MODEL1_ROM_START_ADDR];
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
#if 0
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
#endif
      return ReadLog(addr);
  }

  cerr << "error: no read handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
  return 0;
}