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

Z80PIO::Mode Z80PIO::GetMode(int port) const
{
  return m_ports[port & 1].m_mode;
}

bool Z80PIO::GetIE(int port) const
{
  return m_ports[port & 1].m_ie;
}

bool Z80PIO::GetReady(int port) const
{
  return m_ports[port & 1].m_ready;
}

uint8_t Z80PIO::GetData(int portNum) const
{
  return m_ports[portNum & 1].m_outputLatch;
}

void Z80PIO::SetData(int portNum, uint8_t data)
{
  Port & port = m_ports[portNum & 1];

  // An output port has no input bits. The external buffer can still place a
  // byte where a following IN will read it.
  if (port.m_inputMask == 0)
    port.m_data = data;
  else
    port.m_data = (port.m_data & ~port.m_inputMask) | (data & port.m_inputMask);
}

void Z80PIO::RequestInterrupt(const Port & port)
{
  if (port.m_ie && m_interruptHandler)
    m_interruptHandler(port.m_vector);
}

void Z80PIO::Strobe(int portNum)
{
  Port & port = m_ports[portNum & 1];
  bool acknowledge = port.m_ready
                  || port.m_mode == Mode::Input
                  || port.m_mode == Mode::Bidir;
  port.m_ready = false;
  if (acknowledge)
    RequestInterrupt(port);
}

void Z80PIO::Strobe(int portNum, uint8_t data)
{
  Port & port = m_ports[portNum & 1];
  if (port.m_mode == Mode::Input || port.m_mode == Mode::Bidir || port.m_mode == Mode::Control) {
    SetData(portNum, data);
    port.m_ready = false;
    RequestInterrupt(port);
    return;
  }
  SetData(portNum, data);
  Strobe(portNum);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Z80PIO::Port::Port(int port)
  : m_port(port)
{
  Reset();
}

void Z80PIO::Port::Reset()
{
  m_ie           = false;
  m_ready        = false;
  m_data         = 0;
  m_outputLatch  = 0;
  m_state        = 0;
  m_mode         = Mode::Input;   // default mode
  m_inputMask    = 0xff;
  m_intMask      = 0;
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
        if (data & 0x10)
          m_state = 2;
      }
      else if ((data & 0x0f) == 0x3) {
        // set interrupt status
        m_ie = (data & 0x80) != 0;
      }
      else if ((data & 0x0f) == 0xf) {
        // select mode
        m_mode = (Mode)((data >> 6) & 3);
        m_ready = false;
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
      }
      else if ((data & 0x01) == 0) {
        m_vector = data;
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
      break;
  }
}

// used by CPU to read from from the PIO channel control register
uint8_t Z80PIO::Port::ReadControl()
{
  return m_control;
}

// A data write updates the output latch in every mode, including reset
// input mode. Mode 0 and the output half of mode 2 raise ready. The
// interrupt waits for Strobe().
void Z80PIO::Port::WriteData(uint8_t data)
{
  m_outputLatch = data;
  m_data = (uint8_t)((m_data & m_inputMask) | (data & (uint8_t)~m_inputMask));
  if (m_mode == Mode::Output || m_mode == Mode::Bidir)
    m_ready = true;
  if (m_writeHandler)
    m_writeHandler(m_outputLatch, m_ie);
}

// Mode 1 and the input half of mode 2 raise ready once the CPU has taken
// the latched byte. Mode 3 reads whatever the handler presented.
uint8_t Z80PIO::Port::ReadData()
{
  if (m_readHandler)
    m_readHandler();
  if (m_mode == Mode::Input || m_mode == Mode::Bidir)
    m_ready = true;
  return m_data;
}
