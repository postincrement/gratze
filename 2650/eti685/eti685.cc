

#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "2650/eti685/eti685.h"
#include "video/dg640.h"

using namespace std;

#define   BINBUG_START_ADDR     0x0000
#define   BINBUG_END_ADDR       0x03ff

#define   BINBUG_RAM_START_ADDR 0x0400
#define   BINBUG_RAM_END_ADDR   0x77ff

#define   BINBUG_VIDEO_START_ADDR   0x7800
#define   BINBUG_VIDEO_END_ADDR     0x7fff

extern unsigned char g_rom_binbug6_1ROM[1024];

class Intel8255
{
  public:
    Intel8255();

    void Reset();

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    virtual void SetData(int port, uint8_t data);
    virtual uint8_t GetData(int port) const;

    virtual uint8_t ReadControl();
    virtual void WriteControl(uint8_t data);

    void SetReadHandler(int port, std::function<uint8_t ()> handler);

    struct Port
    {
      Port(int port);
      void Reset();

      uint8_t ReadData();
      void WriteData(uint8_t data);

      void SetData(uint8_t data);
      uint8_t GetData() const;

      int m_port;
      uint8_t m_data = 0;

      std::function<uint8_t ()> m_readHandler;
    };

  protected:  
    uint8_t m_control;
    Port m_ports[3] = { 0, 1, 2 };  
};


Intel8255::Intel8255()
{
  Reset();
}

void Intel8255::Reset()
{
  m_ports[0].Reset();
  m_ports[1].Reset();
  m_ports[2].Reset();
}

void Intel8255::SetReadHandler(int port, std::function<uint8_t ()> handler)
{ m_ports[port % 3].m_readHandler = handler; }

uint8_t Intel8255::Read(uint8_t reg)
{
  switch (reg & 3) {
    case 0:
      return m_ports[0].ReadData();
    case 1:
      return m_ports[1].ReadData();
    case 2:
      return m_ports[2].ReadData();
    case 3:
      return ReadControl();
  }
}

void Intel8255::Write(uint8_t reg, uint8_t data)
{
  switch (reg & 3) {
    case 0:
      m_ports[0].WriteData(data);
      break;
    case 1:
      m_ports[1].WriteData(data);
      break;
    case 2:
      m_ports[2].WriteData(data);
      break;
    case 3:
      WriteControl(data);
      break;
  }
}

uint8_t Intel8255::ReadControl()
{
  return m_control;
}

void Intel8255::WriteControl(uint8_t data)
{
  m_control = data;
  /*
  if ((data & 0x80) == 0) {
    int bit = (data >> 1) & 0x7;
    if (data & 1)
      m_ports[2].m_data |= (1 << bit);
    else
      m_ports[2].m_data &= !(1 << bit);
  }
  else {
  }
  */
}

uint8_t Intel8255::GetData(int portNum) const
{
  return m_ports[portNum % 3].GetData();
}


void Intel8255::SetData(int portNum, uint8_t data)
{
  Port & port = m_ports[portNum % 3];
  cerr << "8255: port " << (char)('A' + portNum) << " received " << HEXFORMAT0x2(data) << endl;

  port.SetData(data);

  // send interrupt of required
//  if (port.m_ie && m_interruptHandler)
//    m_interruptHandler(port.m_vector);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Intel8255::Port::Port(int port)
  : m_port(port)
{
  Reset();
}

void Intel8255::Port::Reset()
{
  m_data = 0;
}

void Intel8255::Port::SetData(uint8_t data)
{
  m_data = data;
}

uint8_t Intel8255::Port::GetData() const
{
  return m_data;
}

uint8_t Intel8255::Port::ReadData()
{
  if (m_readHandler)
    m_data = m_readHandler();

  cerr << "8255: read " << HEXFORMAT0x2(m_data) << " from port " << (char)('A' + m_port) << endl;
  //if (m_data != 0xff)
  //  cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " data = " << HEXFORMAT0x2(m_data) << endl;

  return m_data;
}

void Intel8255::Port::WriteData(uint8_t data)
{
  cerr << "8255: write " << HEXFORMAT0x2(data) << " to port " << (char)('A' + m_port) << endl;
  m_data = data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Intel8255 m_ppi;

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
