

#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "2650/eti685/eti685.h"
#include "video/dg640.h"
#include "devices/intel8255.h"

using namespace std;

#define   BINBUG_START_ADDR     0x0000
#define   BINBUG_END_ADDR       0x03ff

#define   BINBUG_RAM_START_ADDR 0x0400
#define   BINBUG_RAM_END_ADDR   0x77ff

#define   BINBUG_VIDEO_START_ADDR   0x7800
#define   BINBUG_VIDEO_END_ADDR     0x7fff

extern unsigned char g_rom_binbug6_1ROM[1024];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EmulatorInfo g_emulatorInfo =
{
  "eti685",                                // command line option
  "ETI-685 with BINBUG",                   // short name
  "ETI-685 2650 with DG-640 and BINBUG",   // long name

  {
    INFO_CPU(1, 0x0000),
    INFO_ROM(0x0000, g_rom_binbug6_1ROM),

    //INFO_MAIN_RAM(BINBUG_RAM_START_ADDR, 16, 1, 32 - 1 - DG640_VIDEO_RAM_SIZE_K),
    INFO_RAM(BINBUG_RAM_START_ADDR, BINBUG_RAM_END_ADDR),

    INFO_IO_PORT_RW(0x30, 0x33, 2),       // 8255 PPI
    INFO_IO_PORT_READ(0x35, 0x35, 1),     // keyboard read latch
    INFO_IO_PORT_WRITE(0x36, 0x36, 1),    // keyboard reset latch

    DG640_VIDEO_DRIVER(BINBUG_VIDEO_START_ADDR),

    INFO_END()
  }
};

void ETI685::Init()
{  
  VirtualScreen::AddType<DG640>("dg640");
}

ETI685::ETI685()
  : S2650Emulator(&g_emulatorInfo)
{
}

void ETI685::Reset(int addr)
{
  S2650Emulator::Reset(addr);
  m_ppi.Reset();

  // make sure sense is not set
  m_cpu->registers.psu |= (1 << 7);  
  m_keyboardData = 0x00;

  // indicate use of parallel keyboard
  m_ppi.SetData(2, 1 << 6); // bit 6 = 0 
}

void ETI685::OnKeyDown(const SDL_Keysym & keysym)
{
  if ((keysym.sym < 0x80) && (keysym.sym > 0)) {
    m_keyboardData = toupper((char)keysym.sym);
    cout << "binbug: keyboard set to " << HEXFORMAT0x2(m_keyboardData) << endl;
  }
}

uint8_t ETI685::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:
      //cout << "binbug: keyboard reset" << endl;
      return m_keyboardData;
    case 2:
      {
        uint8_t data = m_ppi.Read(port);  
        //cout << "binbug: PPI read " << HEXFORMAT0x2(port) << " of " << HEXFORMAT0x2(data) << endl;
        return data;
      }
  }
  cerr << "binbug: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void ETI685::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      //cout << "binbug: keyboard read" << endl;
      m_keyboardData = 0x00;
      return;
    case 2:
      //cout << "binbug: PPI write " << HEXFORMAT0x2(port) << " - " << HEXFORMAT0x2(data) << endl;
      m_ppi.Write(port, data);
      return;  
  }
  cerr << "binbug: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}
