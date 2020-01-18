

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

extern unsigned char g_rom_binbug6_1[1024];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EmulatorInfo g_emulatorInfo =
{
  "eti685",                                // command line option
  "ETI-685 with BINBUG",                   // short name
  "ETI-685 2650 with DG-640 and BINBUG",   // long name

  {
    INFO_CPU(1, 0x0000),
    INFO_ROM(0x0000, g_rom_binbug6_1),

    //INFO_MAIN_RAM(BINBUG_RAM_START_ADDR, 16, 1, 32 - 1 - DG640_VIDEO_RAM_SIZE_K),
    INFO_RAM(BINBUG_RAM_START_ADDR, BINBUG_RAM_END_ADDR),

    INFO_IO_PORT_RW(0x30, 0x33, 2),       // 8255 PPI
    INFO_IO_PORT_READ(0x35, 0x35, 1),     // keyboard read latch
    INFO_IO_PORT_WRITE(0x36, 0x36, 1),    // keyboard reset latch

    DG640_VIDEO_DRIVER(BINBUG_VIDEO_START_ADDR),

    INFO_END()
  }
};

void ETI685::Instantiate()
{  
  VirtualScreen::AddType<DG640>("dg640");
}

ETI685::ETI685()
  : S2650Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool ETI685::Open(const Options & options)
{
  if (!S2650Emulator::Open(options))
    return false;

  SetKeyboard(&m_keyboard);

  // set keyboard handler
  using namespace std::placeholders;
  m_keyboard.SetHandler(true, std::bind(&ETI685::SetKeyboardData, this, _1));

  return true;
}

void ETI685::Reset(int addr)
{
  S2650Emulator::Reset(addr);
  m_ppi.Reset();

  // make sure sense is not set
  m_cpu->registers.psu |= (1 << 7);
  SetKeyboardData(0x00);

  // indicate use of parallel keyboard
  m_ppi.SetData(2, 1 << 6); // bit 6 = 0 

}

void ETI685::SetKeyboardData(uint8_t data)
{
  m_keyboardData = data;
}

uint8_t ETI685::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:
      return m_keyboardData;
    case 2:
      {
        uint8_t data = m_ppi.Read(port);  
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
      SetKeyboardData(0x00);
      return;
    case 2:
      m_ppi.Write(port, data);
      return;  
  }
  cerr << "binbug: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}
