

#include <iostream>
#include <iomanip>
#include <queue>

#include "common/misc.h"
#include "2650/78up5/78up5.h"

using namespace std;

#define   PIPBUG_START_ADDR     0x0000
#define   PIPBUG_END_ADDR       0x03ff

#define   RAM_START_ADDR        0x0400
#define   RAM_END_ADDR          0x7fff

extern unsigned char g_rom_pipbug_110[1024];
extern unsigned char g_rom_pipbug_300[1024];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INFO_START(78up5_110)
{
  INFO_CPU(1, 0x0000),

  INFO_ROM(0x0000, g_rom_pipbug_110),

  INFO_MAIN_RAM(RAM_START_ADDR, 32-1, 1, 32-1),

  INFO_TERMINAL(80, 24)
}
INFO_END(78up5_110);

static EmulatorInfo g_pipbug110Info =
{
  "78up5-110",                                            // command line option
  "78UP5 with PIPBUG at 100 baud",                        // short name
  "Electronics Australia 78UP5 with PIPBUG at 110 baud",  // long name

  INFO_INSERT(78up5_110)
};

INFO_START(78up5_300)
{
  INFO_CPU(1, 0x0000),

  INFO_ROM(0x0000, g_rom_pipbug_300),

  INFO_MAIN_RAM(RAM_START_ADDR, 32-1, 1, 32-1),

  INFO_TERMINAL(80, 24)
}
INFO_END(78up5_300)


static EmulatorInfo g_pipbug300Info =
{
  "78up5",                                                // command line option
  "78UP5 with PIPBUG at 300 baud",                        // short name
  "Electronics Australia 78UP5 with PIPBUG at 300 baud",  // long name

  INFO_INSERT(78up5_300)
};

//////////////////////////////////////////////////////////////////////////////////////////////

class VirtualUART : public VirtualDevice 
{
  public:
    virtual void Reset() override;

    // set function to poll for serial input
    void SetInHandler(std::function<bool ()> handler);

    // set function to set serial output
    void SetOutHandler(std::function<void (bool)> handler);

    // set function to call when serial input decodes a character
    void SetRXHandler(std::function<void (uint8_t)> handler);

    // call to send a character 
    void SerialOut(uint8_t);

    // call at 4 x the baud rate
    inline void OnClock();

  protected:
    void HandleSerialIn(bool val);
    bool HandleSerialOut();

    std::function<bool ()> m_inHandler = nullptr;
    std::function<void (bool)> m_outHandler = nullptr;

    std::function<void (uint8_t)> m_rxHandler = nullptr;

    int m_inState = 0;
    unsigned m_rxData = 0;

    int m_outState = 0;
    unsigned m_outData;
    std::queue<uint8_t> m_outQueue;

    bool m_debugOut = false; //true;
    bool m_debugIn = false; //true;
};

void VirtualUART::Reset()
{
  
}

void VirtualUART::SetInHandler(std::function<bool ()> handler)
{
  m_inHandler = handler;
}

void VirtualUART::SetOutHandler(std::function<void (bool)> handler)
{
  m_outHandler = handler;
}

// set function to call when serial input decodes a character
void VirtualUART::SetRXHandler(std::function<void (uint8_t)> rxHandler)
{
  m_rxHandler = rxHandler;
}

void VirtualUART::SerialOut(uint8_t ch)
{
  if (m_debugOut)
    cerr << "uart: queueing " << (int)ch << endl;
  m_outQueue.push(ch);
}

void VirtualUART::OnClock()
{
  // look for incoming serial chars
  if (m_inHandler)
    HandleSerialIn(m_inHandler());

  // do outgoing chars
  if (m_outHandler)
    m_outHandler(HandleSerialOut());
}

void VirtualUART::HandleSerialIn(bool val)
{    
  int start = 4;

  // waiting for start bit
  if (m_inState == 0) {
    if (!val) {
      if (m_debugIn)
        cout << "uart: in " << m_inState << " " << (val ? "1" : "0") << " (start)" << endl;
      m_inState = 1;
    }
  }

  // ensure input stays low until middle of start bit
  else if (m_inState < start) {
    if (m_debugIn)
      cout << "uart: in " << m_inState << " " << (val ? "1" : "0") << " (start)" << endl;
    if (!val) {
      m_inState++;
    }
    else {
      if (m_debugIn)
        cout << "uart: ignoring in transient" << endl;
      m_inState = 0;
    }
  }

  // in the middle of (or close to) the start bit
  else if (m_inState < (start + (8 * 4))) {
    int b = (m_inState - start);
    if (m_debugIn)
      cout << "uart: in " << m_inState << " " << (val ? "1" : "0");
    if ((m_inState % 4) == 2) {
      if (m_debugIn)
        cout << " (data " << ((m_inState - start) / 4) << ")" << endl;;
      m_rxData = (m_rxData >> 1) | (val ? 0x80 : 0);
    }
    else if (m_debugIn)
      if (m_debugIn)
        cout << endl;
    m_inState++;
  }

  else if (m_inState < (start + (8 * 4) + 4)) {
    if (m_debugIn)
      cout << "uart: in " << m_inState << " " << (val ? "1" : "0") << " (stop)" << endl;
    m_inState++;  
  }
  else {
    if (m_debugIn)
      cout << "uart: received " << HEXFORMAT0x2(m_rxData) << endl;
    if (m_rxHandler)
      m_rxHandler(m_rxData);
    m_inState = 0;
  }
}

bool VirtualUART::HandleSerialOut()
{
  int start = 4;

  // if not sending, we can only start if we have a char to send
  // and the input is not decoding something
  if (m_outState == 0) {
    if ((m_outQueue.size() == 0) || (m_inState != 0))
      return true;

    m_outData = m_outQueue.front();
    m_outQueue.pop();
    if (m_debugOut) {
      cerr << "uart: sending " << HEXFORMAT0x2(m_outData) << endl;
      cout << "uart: out " << m_outState << " 0 (start)" << endl;
    }
    m_outState = 1;
    return false;
  }

  // do reset of start bit
  else if (m_outState < start) {
    if (m_debugOut) {
      cout << "uart: out " << m_outState << " 0 (start)" << endl;
    }
    m_outState++;
    return false;
  }

  else if (m_outState < (start + (8 * 4))) {
    bool bit = (m_outData & 0x01) != 0;
    if (((m_outState - start) % 4) == 3) {
      m_outData = m_outData >> 1;
    }
    if (m_debugOut)
      cout << "uart: out " << m_outState << " " << (bit ? "1" : "0") << " (data (" << ((m_outState - start) / 4) << ")" << endl;
    m_outState++;
    return bit;
  }

  else if (m_outState < (start + (9 * 4))) {
    if (m_debugOut)
      cout << "uart: out " << m_outState << " 1 (stop)" << endl;
    m_outState++;
  }
  else
    m_outState = 0;

  return true;
}


VirtualUART m_virtualUART;

//////////////////////////////////////////////////////////////////////////////////////////////

EA78UP5_PIPBUG_110::EA78UP5_PIPBUG_110()
  : EA78UP5_Emulator(&g_pipbug110Info, 110)
{
}

EA78UP5_PIPBUG_300::EA78UP5_PIPBUG_300()
  : EA78UP5_Emulator(&g_pipbug300Info, 300)
{
}

void EA78UP5_Emulator::Instantiate()
{  
}

//////////////////////////////////////////////////////////////////////////////////////////////

EA78UP5_Emulator::EA78UP5_Emulator(const EmulatorInfo * info, int baud)
  : S2650Emulator(info)
  , m_baud(baud)
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

  m_serialInterval = m_targetCPUClock_Hz / (m_baud * 1.0) / 4;
  cout << "78up5: poll interval = " << m_serialInterval << " = " << 1000000.0 / m_serialInterval << " Hz" << endl;

  using namespace std::placeholders;

  // make sure sense starts high
  SetSense(true);

  // set clock
  AddCPUTimePollDef(m_serialInterval, std::bind(&VirtualUART::OnClock, &m_virtualUART));

  // set serial in and out
  m_virtualUART.SetInHandler (std::bind(&EA78UP5_Emulator::GetFlag,  this));
  m_virtualUART.SetOutHandler(std::bind(&EA78UP5_Emulator::SetSense, this, _1));

  // set callbacks to terminal
  using namespace std::placeholders;
  using sig = void (Terminal::*)(uint8_t);
  m_virtualUART.SetRXHandler(std::bind(static_cast<sig>(&Terminal::WriteChar), m_terminal.get(), _1));
  m_terminal->SetKeyboardHandler(std::bind(&VirtualUART::SerialOut,   &m_virtualUART, _1));
}

