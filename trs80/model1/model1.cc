#include "model1.h"

#include <iostream>
#include <iomanip>

using namespace std;

extern uint8_t g_trs80CharSets_128[][128][MODEL1_FONT_HEIGHT];

Model1_Emulator::Model1_Emulator()
  : m_withEI(true)
{
}

int Model1_Emulator::GetVideoMemSize_k()
{
  return 1;
}

void Model1_Emulator::GetScreenSizePixels(int & x, int & y) const
{
  x = MODEL1_SCREEN_WIDTH_PIXELS;
  y = MODEL1_SCREEN_HEIGHT_PIXELS;
}

void Model1_Emulator::GetScreenSizeChars(int & x, int & y) const
{
  x = MODEL1_SCREEN_WIDTH_CHARS;
  y = MODEL1_SCREEN_HEIGHT_CHARS;
}

double Model1_Emulator::GetTargetClockSpeed_Hz() const
{
  return 1774000;
}

bool Model1_Emulator::Open(const Options & options)
{
  m_withEI = options.m_withEI;

  cout << "info: expansion interface is " << (m_withEI ? "en" : "dis") << "abled" << endl; 

  m_rtcEnabled = m_withEI;
  m_fdcEnabled = m_withEI;

  return TRS80Emulator::Open(options);
}


bool Model1_Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  if (!Z80Emulator::OpenVideo(mainWindow,  options) || !m_video)
    return false;

  // set the font
  CreateFontData(options, MODEL1_FONT_WIDTH, MODEL1_FONT_HEIGHT, g_trs80CharSets_128[1][0]);

  return true;
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

  if ((addr >= MODEL1_KB_START_ADDR) && (addr <= MODEL1_KB_END_ADDR)) {
    return;
  }

  if (addr <= MODEL1_L2_ROM_END_ADDR) {
    return;
  }

  cerr << "error: no write handler for 0x" << setw(4) << setfill('0') << hex << addr << endl;
}

uint8_t Model1_Emulator::ReadMemory(register uint16_t addr)
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
  m_rom     = g_model1Level2ROM;
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

