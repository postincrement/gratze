#include <iostream>
#include <algorithm>

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
  m_cycleCounter = 0;
  m_timingDirty = true;
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

void Synertek6545::SetCPUCyclesPerChar(int cycles)
{
  if (cycles < 1)
    cycles = 1;
  if (cycles != m_cpuCyclesPerChar) {
    m_cpuCyclesPerChar = cycles;
    m_timingDirty = true;
  }
}

void Synertek6545::RecalcFrameTiming()
{
  m_timingDirty = false;

  const int r0 = m_regs[0];
  const int r4 = m_regs[4];
  const int r5 = m_regs[5];
  const int r6 = m_regs[6];
  const int r9 = m_regs[9];

  // Until the CRTC is programmed, fall back to a 50 Hz frame with ~20% blanking.
  if (r0 == 0 || r4 == 0 || r9 == 0 || r6 == 0) {
    m_frameCpuCycles  = (uint64_t)m_cpuCyclesPerChar * 67500ull;
    m_activeCpuCycles = (m_frameCpuCycles * 4) / 5;
    return;
  }

  const int charsPerLine    = r0 + 1;
  const int scanlinesPerRow = r9 + 1;
  const int totalRows       = r4 + 1;
  const int totalScanlines  = totalRows * scanlinesPerRow + r5;
  const int activeScanlines = std::min(r6, totalRows) * scanlinesPerRow;

  m_frameCpuCycles  = (uint64_t)charsPerLine * (uint64_t)totalScanlines * (uint64_t)m_cpuCyclesPerChar;
  m_activeCpuCycles = (uint64_t)charsPerLine * (uint64_t)activeScanlines * (uint64_t)m_cpuCyclesPerChar;

  if (m_activeCpuCycles >= m_frameCpuCycles && m_frameCpuCycles > 0)
    m_activeCpuCycles = m_frameCpuCycles - 1;
}

void Synertek6545::RefreshVBlank()
{
  if (m_timingDirty)
    RecalcFrameTiming();
  if (m_frameCpuCycles == 0)
    return;

  const uint64_t phase = m_cycleCounter % m_frameCpuCycles;
  const bool inBlank = phase >= m_activeCpuCycles;

  // Status bit 5 follows the real vertical blanking interval only.
  if (inBlank)
    m_status |= eVBlanking;
  else
    m_status &= ~eVBlanking;
}

void Synertek6545::RunKeyboardUpdate()
{
  // LPEN / update-ready are independent of the blanking bit. Microbee
  // Method B (R31) expects update-ready within a short poll loop, not only
  // once per frame — gating it on blanking entry made keyboard scans stall.
  if (!m_updateHandler)
    return;

  uint16_t addr = (m_regs[18] << 8) + m_regs[19];
  if (m_updateHandler(m_writePending, addr)) {
    m_regs[16] = addr >> 8;
    m_regs[17] = addr & 0xff;
    m_status |= eLpenFull;
  }
  m_writePending = false;
  m_status |= eUpdateReady;
}

void Synertek6545::Tick(int cpuCycles)
{
  if (cpuCycles <= 0)
    return;
  m_cycleCounter += (uint64_t)cpuCycles;
  RefreshVBlank();
}

void Synertek6545::Reg31()
{
  m_writePending = true;
  m_status &= ~eUpdateReady;
}

void Synertek6545::UpdateStatus()
{
  RefreshVBlank();

  // Complete keyboard LPEN / R31 updates on a short status-poll delay.
  if (m_state++ >= 20) {
    m_state = 0;
    RunKeyboardUpdate();
  }
}

uint8_t Synertek6545::Read(uint8_t reg)
{
  if (reg == 0) {
    UpdateStatus();
    return m_status;
  }

  // update
  if (m_regSel == 31) {
    Reg31();
    return 0x00;
  }  

  uint8_t data = 0x00;

  // lightpen
  if ((m_regSel == 16) || (m_regSel == 17)) {
    UpdateStatus();
    data = m_regs[m_regSel];
    m_status &= ~eLpenFull;
  } 
  else if (m_regSel > m_maxReg) {
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
    m_regSel = data;
    return;
  }

  if (m_regSel == 31) {
    Reg31();
    return;
  }  

  if (m_regSel > m_maxReg) {
    return;
  }

  m_regs[m_regSel] = data;

  switch (m_regSel) {
    case 0:
    case 4:
    case 5:
    case 6:
    case 9:
      m_timingDirty = true;
      break;
    default:
      break;
  }

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
      break;
    case 12:
    case 13:
      if (m_startAddressHandler)
        m_startAddressHandler((m_regs[12] << 8) + m_regs[13]);
      break;
    case 14:
    case 15:
      if (m_cursorAddressHandler)
        m_cursorAddressHandler((m_regs[14] << 8) + m_regs[15]);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
      break;
    default:  
      break;
  }
}
