#include "model1.h"

#include <iostream>
#include <iomanip>

using namespace std;

Model1_Emulator::Model1_Emulator()
{
}

void Model1_Emulator::WrZ80(register uint16_t addr, register uint8_t val)
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

  if ((addr >= MODEL1_KB_START_ADDR) && (addr <= MODEL1_KB_END_ADDR)) {
    return;
  }

  if (addr <= MODEL1_ROM_END_ADDR) {
    return;
  }

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t Model1_Emulator::RdZ80(register uint16_t addr)
{
  if (addr <= m_romSize) {
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

/////////////////////////////////////////////////////////////

extern unsigned char g_model1Level1ROM[4096];

Model1Level1_Emulator::Model1Level1_Emulator()
{
  m_rom = g_model1Level1ROM;
  m_romSize = sizeof(g_model1Level1ROM);
}

std::string Model1Level1_Emulator::GetTitle() const
{ 
  return "TRS-80 Model 1, Level 1"; 
}

int Model1Level1_Emulator::GetDefaultRAMSize_k() const
{ 
  return 4;
}

////////////////////////////////////////////////////////////////

extern unsigned char g_model1Level2ROM[12288];

Model1Level2_Emulator::Model1Level2_Emulator()
{
  m_rom = g_model1Level2ROM;
  m_romSize = sizeof(g_model1Level2ROM);
}

std::string Model1Level2_Emulator::GetTitle() const
{ 
  return "TRS-80 Model 1, Level 2"; 
}

int Model1Level2_Emulator::GetDefaultRAMSize_k() const
{ 
  return 48;
}

