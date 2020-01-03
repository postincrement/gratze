

#include <iostream>
#include <iomanip>
#include <functional>

#include "misc.h"
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

class Z80PIO
{
  public:
    Z80PIO();
    void Reset();

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    virtual void ReceiveData(int port, uint8_t data);

    void SetInterruptHandler(std::function<void (uint8_t)> handler)
    { m_interruptHandler = handler; }

    struct Port
    {
      Port(int port);
      void Reset();

      void WriteControl(uint8_t data);
      void WriteData(uint8_t data);

      uint8_t ReadControl();
      uint8_t ReadData();

      bool ReceiveData(uint8_t data);

      int m_port;
      int m_state;
      uint8_t m_data = 0;
      bool m_ie = false;
      int8_t m_mode;
      uint8_t m_mode3 = 0;
      uint8_t m_vector = 0;
      uint8_t m_intMask = 0;
      uint8_t m_control = 0;
    };

  protected:
    Port m_ports[2] = { 0, 1 };  
    std::function<void (uint8_t)> m_interruptHandler;
};

Z80PIO::Z80PIO()
{
  Reset();
}

void Z80PIO::Reset()
{
  m_ports[0].Reset();
  m_ports[1].Reset();
}

uint8_t Z80PIO::Read(uint8_t reg)
{
  switch (reg & 3) {
    case 0:
      return m_ports[0].ReadData();
    case 1:
      return m_ports[0].ReadControl();
    case 2:
      return m_ports[1].ReadData();
    case 3:
      return m_ports[1].ReadControl();
      break;
  }
}

void Z80PIO::Write(uint8_t reg, uint8_t data)
{
  switch (reg & 3) {
    case 0:
      m_ports[0].WriteData(data);
      break;
    case 1:
      m_ports[0].WriteControl(data);
      break;
    case 2:
      m_ports[1].WriteData(data);
      break;
    case 3:
      m_ports[1].WriteControl(data);
  }
}

void Z80PIO::ReceiveData(int portNum, uint8_t data)
{
  Port & port = m_ports[portNum & 1];
  cerr << "z80pio: port " << ((portNum == 0) ? 'A' : 'B') << " received " << HEXFORMAT0x2(data) << endl;
  if (port.ReceiveData(data) && port.m_ie && m_interruptHandler)
    m_interruptHandler(port.m_vector);
}


Z80PIO::Port::Port(int port)
  : m_port(port)
{
  Reset();
}

void Z80PIO::Port::Reset()
{
  m_ie = false;
  m_data = 0;
  m_state = 0;
  m_mode  = 0;
  m_mode3 = 0;
  m_intMask = 0;
}

void Z80PIO::Port::WriteControl(uint8_t data)
{
  m_control = data;

  switch (m_state) {

    // waiting for command
    case 0:
      if ((data & 0x0f) == 0x7) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
        if (data & 0x10)
          m_state = 2;
      }
      else if ((data & 0x0f) == 0x3) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
      }
      else if ((data & 0x0f) == 0xf) {
        // select mode
        m_mode = data >> 6;
        if (m_mode == 3)
          m_state = 1;  
        else if (m_mode == 2) {
          cerr << "z80pio: cannot set port B to mode 3" << endl;
          m_mode = 0;
        }
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " in mode " << (int)m_mode << endl;
      }
      else if ((data & 0x01) == 0) {
        m_vector = data;
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt vector set to " << HEXFORMAT0x2(m_vector) << endl;
      }
      else {
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " received unknown command " << HEXFORMAT0x2(data) << endl;
      }
      break;

    // get mode 3 direction bits
    case 1:
      m_mode3 = data;
      m_state = 0;
      break;

    // get inrerrupt mask
    case 2:
      m_intMask = data;
      m_state = 0;
      cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt mask set to " << HEXFORMAT0x2(m_intMask) << endl;
      break;
  }
}

bool Z80PIO::Port::ReceiveData(uint8_t data)
{
  m_data = toupper(data);
  return m_ie; 
}


void Z80PIO::Port::WriteData(uint8_t data)
{

}

uint8_t Z80PIO::Port::ReadControl()
{
  return m_control;
}

uint8_t Z80PIO::Port::ReadData()
{
  return m_data;
}



Z80PIO m_pio;

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

    INFO_IO_PORT_RW(0x00, 0x03, 1),    // PIO
    INFO_IO_PORT_RW(0x04, 0x07, 2),    // CTC

    INFO_IO_PORT_RW(0x08, 0x08, 3),      // SWP

    INFO_IO_PORT_READ(0x09, 0x09, 4),    // parallel port

    INFO_IO_PORT_RW(0x0C, 0x0D, 5),      // PIC

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
  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&DG680_Emulator::OnPIOInterrupt, this, _1));
}

void DG680_Emulator::OnPIOInterrupt(uint8_t vector)
{
  Interrupt(vector);
}

void DG680_Emulator::OnKeyDown(const SDL_Keysym & keysym)
{
  if ((keysym.sym < 0x80) && (keysym.sym > 0)) {
    m_pio.ReceiveData(0, keysym.sym);
  }
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

