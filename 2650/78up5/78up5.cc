

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

class SerialDecoder 
{
  public:
    SerialDecoder();

    inline void OnBit(bool val, uint64_t cycles);

    virtual void OnBreak();

    void SetHandler(std::function<void (uint8_t)> handler);

  protected:
    std::function<void (uint8_t)> m_handler = nullptr;

    int m_state = 0;
    unsigned m_data = 0;
};

SerialDecoder::SerialDecoder()
{
}

void SerialDecoder::SetHandler(std::function<void (uint8_t)> handler)
{
  m_handler = handler;
}

void SerialDecoder::OnBreak()
{
  cerr << "serial: BREAK" << endl;
}

void SerialDecoder::OnBit(bool val, uint64_t cycles)
{
  /*
      ---+   +---+---+---+---+---+---+---+---+---+---
         | S | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | S
         +---+---+---+---+---+---+---+---+---+
          01  2345
  */

  int start = 2;

  // waiting for start bit
  if (m_state == 0) {
    if (!val) {
      cout << "serial: start bit" << endl;
      m_state = 1;
    }
  }

  // ensure input stays low until middle of start bit
  else if (m_state < start) {
    if (!val) {
      m_state++;
    }
    else {
      cout << "serial: ignoring transient" << endl;
      m_state = 0;
    }
  }

  // in the middle of (or close to) the start bit
  else {
    int b = (m_state - start);
    if ((m_state % 4) == start) {
      m_data = (m_data >> 1) | (val ? 0x80 : 0);
    }
    if (m_state == (start + 8*4)) {
      m_state = 0;
      cout << "serial: extracted " << HEXFORMAT0x2(m_data) << endl;
      if (m_handler)
        m_handler(m_data);
    }
    else {
      m_state++;
    }
  }
}

SerialDecoder m_serialDecoder;

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

  // add decoder for flag
  uint64_t clockInterval = m_targetCPUClock_Hz / 110.0 / 4;
  cout << "78up5: poll interval = " << clockInterval << " = " << 1000000.0 / clockInterval << " Hz" << endl;
  using namespace std::placeholders;
  AddCPUTimePollDef(clockInterval, std::bind(&EA78UP5_Emulator::SerialIn, this, _1, _2));  

  m_serialDecoder.SetHandler(std::bind(&Terminal::WriteChar, m_terminal.get(), _1));
  //m_terminal->m_keyboard.SetHandler(std::bind(&Terminal::OnKeyboard, this, _1))
}

void EA78UP5_Emulator::SerialIn(double secs, uint64_t clocks)
{
  //cout << "78up5: poll" << endl;
  m_serialDecoder.OnBit(GetFlag(), clocks);
}

void EA78UP5_Emulator::SerialOut(uint8_t ch)
{

}
