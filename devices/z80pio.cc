#include "misc.h"

#include "z80pio.h"

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
        //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
        if (data & 0x10)
          m_state = 2;
      }
      else if ((data & 0x0f) == 0x3) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
        //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " IE is " << (m_ie ? "on" : "off") << endl;
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
        //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " in mode " << (int)m_mode << endl;
      }
      else if ((data & 0x01) == 0) {
        m_vector = data;
        //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt vector set to " << HEXFORMAT0x2(m_vector) << endl;
      }
      else {
        //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " received unknown command " << HEXFORMAT0x2(data) << endl;
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
      //cerr << "z80pio: port " << ((m_port == 0) ? 'A' : 'B') << " interrupt mask set to " << HEXFORMAT0x2(m_intMask) << endl;
      break;
  }
}

bool Z80PIO::Port::ReceiveData(uint8_t data)
{
  m_data = data;
  return m_ie;
}

void Z80PIO::Port::WriteData(uint8_t data)
{
  m_data = data;
}

uint8_t Z80PIO::Port::ReadControl()
{
  return m_control;
}

uint8_t Z80PIO::Port::ReadData()
{
  return m_data;
}

