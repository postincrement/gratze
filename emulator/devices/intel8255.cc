#include <iostream>
#include <vector>
#include <functional>

#include "common/misc.h"
#include "devices/intel8255.h"

using namespace std;

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
  return 0; // not needed, but avoids a compiler warning
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
      m_ports[2].m_data &= ~(1 << bit);
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
