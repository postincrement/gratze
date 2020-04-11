#include <iostream>

#include "common/misc.h"
#include "devices/synertek6545.h"

using namespace std;

Synertek6545::Synertek6545()
{
  m_regs.resize(m_maxReg+1);
}

void Synertek6545::Reset()
{
  cout << "6545: reset" << endl;
  m_regSel = 0;
  m_state  = 0;
  m_status = eUpdateReady | eVBlanking;
  m_writePending = false;
  VECTOR_ZERO(m_regs);
}

void Synertek6545::SetScreenShapeHandler(std::function<void (int cols, int rows, int scan)> handler)
{
  m_screenShapeHandler = handler;
}

void Synertek6545::SetStartAddressHandler(std::function<void (uint16_t addr)> handler)
{
  m_startAddressHandler = handler;
}

void Synertek6545::SetCursorAddressHandler(std::function<void (uint16_t addr)> handler)
{
  m_cursorAddressHandler = handler;
}

void Synertek6545::SetCursorShapeHandler(std::function<void (uint8_t start, uint8_t end, int blinkRate)> handler)
{
  m_cursorShapeHandler = handler;
}

void Synertek6545::SetUpdateHandler(std::function<bool (bool, uint16_t &)> handler)
{
  m_updateHandler = handler;
}

void Synertek6545::Reg31()
{
  m_writePending = true;
  m_status &= ~eUpdateReady;
}

void Synertek6545::UpdateStatus()
{
  if (m_state++ >= 20) {
    if (m_updateHandler) {
      uint16_t addr = (m_regs[18] << 8) + m_regs[19];
      if (m_updateHandler(m_writePending, addr)) {
        //cout << "6545: lpen returned " << HEXFORMAT0x4(addr) << endl;
        m_regs[16] = addr >> 8;
        m_regs[17] = addr & 0xff;
        m_status |= eLpenFull;
      }
      m_writePending = false;
      m_status |= eUpdateReady;
    }
    m_state = 0;
  }
}

uint8_t Synertek6545::Read(uint8_t reg)
{
  if (reg == 0) {
    UpdateStatus();
    //if (m_status != 0)
    //  cout << "6545: read from status " << HEXFORMAT0x2(m_status) << endl;
    return m_status;
  }

  // update
  if (m_regSel == 31) {
    //cout << "6545: read from 31" << endl;
    Reg31();
    return 0x00;
  }  

  uint8_t data = 0x00;

  // lightpen
  if ((m_regSel == 16) || (m_regSel == 17)) {
    UpdateStatus();
    data = m_regs[m_regSel];
    m_status &= ~eLpenFull;
    uint16_t addr = (m_regs[16] << 8) + m_regs[17];
    //if (addr != 0)
      //cout << "6545: read lightpen address (" << ((m_regSel == 16) ? "H" : "L") << ") is " << HEXFORMAT0x4(addr) << endl;
  } 
  else if (m_regSel > m_maxReg) {
    //cout << "6545: read from status " << HEXFORMAT0x2(m_status) << endl;
    data = 0x00;
  }

  return data;
}

uint8_t Synertek6545::GetStatus() const
{
  return m_status;
}

void Synertek6545::SetStatus(uint8_t v)
{
  //m_status = v;
}

void Synertek6545::Write(uint8_t reg, uint8_t data)
{
  if (reg == 0) {
    //cerr << "6545: write to addr reg - " << HEXFORMAT0x2(data) << endl;
    m_regSel = data;
    return;
  }

  //cerr << "6545: write to data reg " << (int)m_regSel << " - " << HEXFORMAT0x2(data) << endl;

  // update
  if (m_regSel == 31) {
    //cout << "6545: write to 31" </< endl;
    Reg31();
    return;
  }  

  if (m_regSel > m_maxReg) {
    //cerr << "6545: write to bad reg " << (int)m_regSel << " - " << HEXFORMAT0x2(data) << endl;
    return;
  }

  m_regs[m_regSel] = data;

  switch (m_regSel) {
    case 1:
    case 6:
    case 9:
      if ((m_regs[1] != 0) && (m_regs[6] != 0) && (m_regs[9] != 0) && m_screenShapeHandler)
        m_screenShapeHandler(m_regs[1], m_regs[6], m_regs[9]);
      break;
      
    case 10:
    case 11:
      if (m_cursorShapeHandler) {
        static int m_rates[4] = { 0, -1, 16, 32 };
        m_cursorShapeHandler(m_regs[10] & 0x1f, m_regs[11] & 0x1f, m_rates[(m_regs[10] >> 5) & 0x3]);
      }
      //cout << "6545: write cursor (" << ((m_regSel == 10) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[10] << 8) + m_regs[11]) << endl;
      break;
    case 12:
    case 13:
      if (m_startAddressHandler)
        m_startAddressHandler((m_regs[12] << 8) + m_regs[13]);
      //cout << "6545: write start address (" << ((m_regSel == 12) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[12] << 8) + m_regs[13]) << endl;
      break;
    case 14:
    case 15:
      //cout << "6545: write cursor address (" << ((m_regSel == 14) ? "H" : "L") << ") is " << HEXFORMAT0x4((m_regs[14] << 8) + m_regs[15]) << endl;
      if (m_cursorAddressHandler)
        m_cursorAddressHandler((m_regs[14] << 8) + m_regs[15]);
      break;
    case 16:
    case 17:
      //cout << "6545: write lightpen address (" << ((m_regSel == 16) ? "H" : "L") << ") is " << HEXFORMAT0x4((m_regs[16] << 8) + m_regs[17]) << endl;
      break;
    case 18:
    case 19:
      //cout << "6545: write update address (" << ((m_regSel == 18) ? "H" : "L") << ") " << HEXFORMAT0x2(data) << " gives " << HEXFORMAT0x4((m_regs[18] << 8) + m_regs[19]) << endl;
      break;
    default:  
      //cerr << "6545: write to reg " << (int)m_regSel << " - " << HEXFORMAT0x2(data) << endl;
      break;
  }
}
