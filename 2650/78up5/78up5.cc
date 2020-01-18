

#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "2650/78up5/78up5.h"

using namespace std;

#define   PIPBUG_START_ADDR     0x0000
#define   PIPBUG_END_ADDR       0x03ff

#define   RAM_START_ADDR        0x0400
#define   RAM_END_ADDR          0x7fff

extern unsigned char g_rom_pipbug[1024];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EmulatorInfo g_emulatorInfo =
{
  "78up5",                                     // command line option
  "78UP5 with PIPBUG",                         // short name
  "Electronics Australia 78UP5 with PIPBUG",   // long name

  {
    INFO_CPU(1, 0x0000),
    INFO_ROM(0x0000, g_rom_pipbug),

    INFO_MAIN_RAM(RAM_START_ADDR, 16, 1, 32 - 1),

    INFO_TERMINAL(80, 24),

    INFO_END()
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////

class BitDecoder 
{
  public:
    BitDecoder();
    void Open(double clock_mhz, bool state, int baud);

    inline void OnBit(bool val, uint64_t cycles);

    void OnTransition(bool val, uint64_t cycles);

    virtual void OnBreak();

  protected:
    double m_clock_mhz;
    uint64_t m_prevTime;
    bool m_prevState;

    int m_bitTime_us;
    int m_minBitTime_us;

    int m_state = 0;
    unsigned m_data = 0;
    int m_count = 0;
};

BitDecoder::BitDecoder()
{

}

void BitDecoder::Open(double clock_mhz, bool state, int baud)
{
  m_clock_mhz = clock_mhz;
  m_prevTime  = 0;
  m_prevState = state;

  m_bitTime_us = m_clock_mhz / baud;

  m_minBitTime_us = m_bitTime_us * 8 / 10;

  cout << "serial: bit time is " << m_bitTime_us << " us, min " << m_minBitTime_us << " us" << endl;
}

void BitDecoder::OnBit(bool val, uint64_t cycles)
{
  if (val == m_prevState)
    return;

  OnTransition(val, cycles);  
}

void BitDecoder::OnBreak()
{
  cerr << "serial: BREAK" << endl;
}


void BitDecoder::OnTransition(bool val, uint64_t cycles)
{
  // each CPU cycle is 3 clock cycles
  int us = (cycles - m_prevTime) * 3 * 1e+6 / m_clock_mhz;

  //cout << "serial: transition is " << us << " us" << endl;

  switch (m_state) {

    // waiting for start bit
    case 0:
      if (!val) {
        //cout << "serial: start bit" << endl;
        m_state = 1;
      }
      break;

    // input has gone to 1
    // if previous run of zeroes was less than a bit time, it's a transient
    // if previous run of zeroes was more than 10 bit times, it's a break 
    case 1:
      if (us < m_minBitTime_us) {
        //cout << "serial: ignore low transient" << endl;
        m_state = 0;
        break;
      }
      if (us > (8*m_bitTime_us)) {
        //cout << "serial: break" << endl;
        OnBreak();
        m_state = 0;
        break;
      }

      m_state = 2;
      m_count = 9; // include start bit
      // fall through

    // decode the bits preior to the transition
    case 2:  
      //cout << "serial: data " << (m_prevState ? "1" : "0") << " " << us << " us" << endl;
      // extract bits
      while ((m_count > 0) && (us > m_minBitTime_us)) {
        //cout << "serial: bit " << (m_prevState ? "1" : "0") << endl;      
        m_data = (m_data >> 1) | (m_prevState ? 0x80 : 0);
        us -= m_bitTime_us;
        m_count--;
      }
      if (m_count == 0) {
        cout << "serial: extracted " << HEXFORMAT0x2(m_data) << endl;
        m_state = 0;
      }
      break;
  }

  m_prevTime  = cycles;
  m_prevState = val;     
}

BitDecoder m_bitDecoder;

//////////////////////////////////////////////////////////////////////////////////////////////

void EA78UP5_Emulator::Instantiate()
{  
}

EA78UP5_Emulator::EA78UP5_Emulator()
  : S2650Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool EA78UP5_Emulator::Open(const Options & options)
{
  if (!S2650Emulator::Open(options))
    return false;

  return true;
}

void EA78UP5_Emulator::Reset(int addr)
{
  S2650Emulator::Reset(addr);

  // make sure sense is not set to indicate start of serial char
  SetSense(true);

  m_bitDecoder.Open(m_targetCPUClock_Hz, true, 110);
}

bool EA78UP5_Emulator::Exec(int cycles)
{
  S2650Emulator::Exec(cycles);

  m_bitDecoder.OnBit(GetFlag(), m_cycleCounter);

  return true;
}