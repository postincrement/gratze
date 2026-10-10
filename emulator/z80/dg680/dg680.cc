

#include <iostream>
#include <iomanip>
#include <functional>

#include "common/misc.h"
#include "z80/dg680/dg680.h"
#include "video/dg640.h"
#include "devices/z80pio.h"

using namespace std;

#define   DG680_ROM_START_ADDR    0xd000
#define   DG680_ROM_END_ADDR      0xd7ff

#define   DG680_RAM_START_ADDR    0xd800
#define   DG680_RAM_END_ADDR      0xdfff

#define   DG680_VIDEO_START_ADDR  0xf000
#define   DG680_VIDEO_END_ADDR    0xf7ff

extern unsigned char g_dgos680_1_4ROM[DG680_ROM_END_ADDR - DG680_ROM_START_ADDR + 1];

INFO_START(dg680)
{
  INFO_CPU(4, DG680_ROM_START_ADDR),

  INFO_ROM(DG680_ROM_START_ADDR, g_dgos680_1_4ROM),

  INFO_MAIN_RAM(0x0000, 48, 8, DG680_ROM_START_ADDR / 1024),
  INFO_RAM(DG680_RAM_START_ADDR, DG680_RAM_END_ADDR),

  INFO_IO_PORT_RW(0x00, 0x03, 1),      // PIO
  INFO_IO_PORT_RW(0x04, 0x07, 2),      // CTC

  INFO_IO_PORT_RW(0x08, 0x08, 3),      // SWP

  INFO_IO_PORT_READ(0x09, 0x09, 4),    // parallel port

  INFO_IO_PORT_RW(0x0C, 0x0D, 5),      // PIC

  DG640_VIDEO_DRIVER(DG680_VIDEO_START_ADDR)
}
INFO_END(dg680);

static EmulatorInfo g_emulatorInfo = 
{
  "dg680",                        // command line option
  "DG-680",                       // short name
  "DG-680 with DGOS",             // long name
  true,                           // SDL window

  INFO_INSERT(dg680)
};

void DG680_Emulator::Instantiate()
{  
  MemoryMappedScreen::AddType<DG640>("dg640");
}

DG680_Emulator::DG680_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool DG680_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  ParallelKeyboard * kb = new ParallelKeyboard();
  SetKeyboard(kb);

  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&DG680_Emulator::OnPIOInterrupt, this, _1));
  kb->SetKeyCharCallback(std::bind(&DG680_Emulator::OnASCIIKey, this, _1));

  return true;
}

void DG680_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);
  m_pio.Reset();

  // DGOS copies an IM 2 vector to D968 that points at the keyboard ISR.
  // That ISR reads PIO port A. The monitor never programs the port, so the
  // ASCII keyboard has to be presented as an interrupting input.
  m_pio.Write(1, 0x4f); // port A: input
  m_pio.Write(1, 0x68); // interrupt vector, indexes D968
  m_pio.Write(1, 0x87); // interrupt enable
}

bool DG680_Emulator::OnHostKey(const SDL_Keysym & keysym)
{
  // DGOS cursor keys: left Ctrl-A, right Ctrl-S, up Ctrl-W, down Ctrl-Z.
  int ascii = -1;
  switch (keysym.sym) {
    case SDLK_LEFT:  ascii = 0x01; break;
    case SDLK_RIGHT: ascii = 0x13; break;
    case SDLK_UP:    ascii = 0x17; break;
    case SDLK_DOWN:  ascii = 0x1a; break;
    default:         return false;
  }

  OnASCIIKey((uint8_t)ascii);
  return true;
}

void DG680_Emulator::OnASCIIKey(uint8_t ch)
{
  if ((ch >= 'a') && (ch <= 'z'))
    ch = (uint8_t)(ch - 'a' + 'A');
  else if (ch == 0x08)
    ch = 0x7f;

  m_pio.Strobe(0, ch);
}

void DG680_Emulator::OnPIOInterrupt(uint8_t vector)
{
  Interrupt(vector);
}

uint8_t DG680_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:
      return m_pio.Read(port & 0x3);
  }
  cerr << "dg680: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void DG680_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      return m_pio.Write(port & 0x3, data);
  }
  cerr << "dg680: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

