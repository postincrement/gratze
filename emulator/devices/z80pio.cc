#include "common/misc.h"

#include "devices/z80pio.h"

using namespace std;

Z80PIO::Z80PIO()
{
  Reset();
}

void Z80PIO::Reset()
{
  m_ports[0].Reset();
  m_ports[1].Reset();
}

void Z80PIO::SetReadHandler(int port, std::function<uint8_t ()> handler)
{ m_ports[port & 1].m_readHandler = handler; }

void Z80PIO::SetWriteHandler(int port, std::function<void (uint8_t, bool)> handler)
{ m_ports[port & 1].m_writeHandler = handler; }

void Z80PIO::SetInterruptHandler(std::function<void (uint8_t)> handler)
{ m_interruptHandler = handler; }

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
  }
  return 0; // not needed, but avoids a compiler warning
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
      break;
  }
}

// called by external application to get the data previously written by the CPU
// probably in response to a write handler
uint8_t Z80PIO::GetData(int portNum) const
{
  const Port & port = m_ports[portNum & 1];
  uint8_t data = m_ports[portNum & 1].m_data;

  // send interrupt if required
  if ((port.m_mode == Mode::Output) && port.m_ie && m_interruptHandler)
    m_interruptHandler(port.m_vector);

  return data;  
}

// called by external application to set input data to be read by the CPU
// probably in response to a read handler
void Z80PIO::SetData(int portNum, uint8_t data)
{
  Port & port = m_ports[portNum & 1];
  cerr << "z80pio: port " << ((portNum == 0) ? 'A' : 'B') << " received " << HEXFORMAT0x2(data) << endl;

  // save data
  port.m_data = (port.m_data & ~port.m_inputMask) | (data & port.m_inputMask);

  // send interrupt if required
  if ((port.m_mode == Mode::Input) && port.m_ie && m_interruptHandler) {
    cerr << "z80pio: port " << ((portNum == 0) ? 'A' : 'B') << " interrupt" << endl;
    m_interruptHandler(port.m_vector);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Z80PIO::Port::Port(int port)
  : m_port(port)
{
  Reset();
}

void Z80PIO::Port::Reset()
{
  m_ie        = false;
  m_data      = 0;
  m_state     = 0;
  m_mode      = Mode::Input;   // default mode
  m_inputMask = 0xff;
  m_intMask   = 0;
}

// used by CPU to write to the PIO channel control register
void Z80PIO::Port::WriteControl(uint8_t data)
{
  m_control = data;

  switch (m_state) {

    // waiting for command
    case 0:
      if ((data & 0x0f) == 0x7) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
        if (m_port == 0)
          cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
        if (data & 0x10)
          m_state = 2;
      }
      else if ((data & 0x0f) == 0x3) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
        if (m_port == 0)
          cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
      }
      else if ((data & 0x0f) == 0xf) {
        // select mode
        m_mode = (Mode)((data >> 6) & 3);
        switch (m_mode) {
          case Mode::Control:
            m_state = 1;
            break;
          case Mode::Bidir:
            if (m_port == 0) {
              m_inputMask = 0xff;
              break;
            }
            //[[fallthough]];
          case Mode::Input:  
            m_inputMask = 0xff;
            m_mode = Mode::Input;
            break;
          case Mode::Output:  
            m_inputMask = 0x00;
            break;
        }
        //if (m_port == 0)
        //  cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " in mode " << (int)m_mode << endl;
      }
      else if ((data & 0x01) == 0) {
        m_vector = data;
        //if (m_port == 0)
        //  cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt vector set to " << HEXFORMAT0x2(m_vector) << endl;
      }
      else {
        //if (m_port == 0)
        //  cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " received unknown command " << HEXFORMAT0x2(data) << endl;
      }
      break;

    // get mode 3 direction bits
    case 1:
      m_inputMask = data;
      m_state     = 0;
      break;

    // get interrupt mask
    case 2:
      m_intMask = data;
      m_state   = 0;
      if (m_port == 0)
        cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt mask set to " << HEXFORMAT0x2(m_intMask) << endl;
      break;
  }
}

// used by CPU to read from from the PIO channel control register
uint8_t Z80PIO::Port::ReadControl()
{
  return m_control;
}

// used by CPU to write data to the PIO channel data register
void Z80PIO::Port::WriteData(uint8_t data)
{
  if (m_mode == Mode::Bidir) {
  }
  else if (m_mode != Mode::Input) {
    m_data = (m_data & m_inputMask) | (data & ~m_inputMask);
    //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " write data " << HEXFORMAT0x2(data) << "->" << HEXFORMAT0x2(m_data) << endl;
    if (m_writeHandler)
      m_writeHandler(m_data, m_ie);
  }
}

// used by CPU to read data from the PIO channel data register
uint8_t Z80PIO::Port::ReadData()
{
  if (m_mode == Mode::Bidir) {
  }
  else if (m_mode != Mode::Output) {
    if (m_readHandler) {
      m_readHandler();
      //uint8_t data = m_readHandler();
      //m_data = (data & m_inputMask) | (m_data & ~m_inputMask);
    }
  }

  return m_data;
}




