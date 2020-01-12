

#include <iostream>
#include <iomanip>
#include <functional>

#include "src/misc.h"
#include "z80/microbee/microbee.h"
#include "video/dg640.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"


using namespace std;

#define   MICROBEE_ROM_START_ADDR     0x8000
#define   MICROBEE_ROM_END_ADDR       0xbfff

#define   MICROBEE_RAM_START_ADDR     0x0000
#define   MICROBEE_RAM_END_ADDR       0x7fff

#define   MICROBEE_VIDEO_START_ADDR   0xf000
#define   MICROBEE_VIDEO_END_ADDR     0xf7ff

#define   MICROBEE_PCG_START_ADDR     0xf800
#define   MICROBEE_PCG_END_ADDR       0xffff

extern unsigned char g_microbeeBasic5_22e_ROM[16384];

static EmulatorInfo g_emulatorInfo = 
{
  "mbee",                    // command line option
  "Microbee 32k",            // short name
  "Microbee 32k",            // long name

  {
    INFO_CPU(4, MICROBEE_ROM_START_ADDR),

    INFO_ROM(MICROBEE_ROM_START_ADDR, g_microbeeBasic5_22e_ROM),

    INFO_MAIN_RAM(MICROBEE_RAM_START_ADDR, 32, 16, MICROBEE_ROM_START_ADDR / 1024),

    INFO_IO_PORT_RW(0x00, 0x03, 1),      // PIO
    INFO_IO_PORT_RW(0x08, 0x08, 4),      // colour control port
    INFO_IO_PORT_RW(0x0b, 0x0b, 3),      // ??
    INFO_IO_PORT_RW(0x0c, 0x0d, 2),      // 6545 

    DG640_VIDEO_DRIVER(MICROBEE_VIDEO_START_ADDR),

    INFO_RAM(MICROBEE_PCG_START_ADDR, MICROBEE_PCG_END_ADDR),

    INFO_END()
  }
};


/////////////////////////////////////////////////////////////////////////////////////////////

class Motorola6845 : public VirtualDevice
{
  public:
    const int m_regCount = 17;

    Motorola6845();

    virtual void Reset() override;

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    void SetLightPenHandler(std::function<uint16_t ()> handler);

  protected:
    uint8_t m_regSel = 0;
    std::vector<uint8_t> m_regs;
    std::function<uint16_t ()> m_lightPenHandler = nullptr;
};

Motorola6845::Motorola6845()
{
  m_regs.resize(m_regCount);
}

void Motorola6845::Reset()
{
  m_regSel = 0;
}

void Motorola6845::SetLightPenHandler(std::function<uint16_t ()> handler)
{
  m_lightPenHandler = handler;
}

uint8_t Motorola6845::Read(uint8_t reg)
{
  if (reg == 0)
    return 0x80;

  if (m_regSel >= m_regCount)
    return 0;

  // scan keyboard
  if ((m_regSel == 16) || (m_regSel == 17)) {
    if (m_lightPenHandler) {
      uint16_t addr = m_lightPenHandler();
      m_regs[16] = addr >> 8;
      m_regs[17] = addr & 0xff;
    }
    //cout << "6845: lightpen address (" << ((m_regSel == 16) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[16 << 8]) + m_regs[17]) << endl;
  } 

  return m_regs[m_regSel];
}

void Motorola6845::Write(uint8_t reg, uint8_t data)
{
  if (reg == 0) {
    m_regSel = data;
    return;
  }

  if (m_regSel >= m_regCount)
    return;

  m_regs[m_regSel] = data;

  switch (m_regSel) {
    case 12:
    case 13:
      //cout << "6845: start address (" << ((m_regSel == 12) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[12 << 8]) + m_regs[13]) << endl;
      break;
    case 14:
    case 15:
      //cout << "6845: cursor address (" << ((m_regSel == 14) ? "H" : "L") << ") is " << HEXFORMAT0x4((m_regs[14 << 8]) + m_regs[15]) << endl;
      break;
    case 16:
    case 17:
      //cout << "6845: lightpen address (" << ((m_regSel == 16) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[16 << 8]) + m_regs[17]) << endl;
      break;
    default:  
      break;
  }
  //cerr << "6845: write to reg " << (int)m_regSel << " - " << HEXFORMAT0x2(data) << endl;
}

static Motorola6845 m_crtc;

/////////////////////////////////////////////////////////////////////////////////////////////




const KeyboardScanner::ScanCode keys[8*8] = {
  { "@" }, { "H" }, { "P" },  {  "X" },      { "1"},  {     "9" } ,  {         " " } , {     "REP" } ,
  { "A" }, { "I" }, { "Q" },  {  "Y" },      { "2"},  {     ":" } ,  { "Backspace" } , {   "Shift" } ,
  { "B" }, { "J" }, { "R" },  {  "Z" },      { "3"},  {     ";" } ,  {       "Tab" } , {        0  } ,
  { "C" }, { "K" }, { "S" },  {  "[" },      { "4"},  {     "," } ,  {        "LF" } , { "Control" } ,
  { "D" }, { "L" }, { "T" },  { "\\" },      { "5"},  { "Break" } ,  {    "Return" } , {        0  } ,
  { "E" }, { "M" }, { "U" },  {  "]" },      { "6"},  {     "." } ,  {    "Escape" } , {        0  } ,
  { "F" }, { "N" }, { "V" },  {  "^" },      { "7"},  {     "/" } ,  {    "Delete" } , {        0  } ,
  { "G" }, { "O" }, { "W" },  {  "Delete" }, { "8"},  {     "0" } ,  {    "Insert" } , {        0  } 
};

const KeyboardScanner::ScanCode shiftedkeys[8*8] = {
  { "@" }, { "H" }, { "P" },  {  "X" },      { "1"},  {     "9" } ,  {         " " } , {     "REP" } ,
  { "A" }, { "I" }, { "Q" },  {  "Y" },      { "2"},  {     ":" } ,  { "Backspace" } , {   "Shift" } ,
  { "B" }, { "J" }, { "R" },  {  "Z" },      { "3"},  {     ";" } ,  {       "Tab" } , {        0  } ,
  { "C" }, { "K" }, { "S" },  {  "[" },      { "4"},  {     "," } ,  {        "LF" } , { "Control" } ,
  { "D" }, { "L" }, { "T" },  { "\\" },      { "5"},  { "Break" } ,  {    "Return" } , {        0  } ,
  { "E" }, { "M" }, { "U" },  {  "]" },      { "6"},  {     "." } ,  {    "Escape" } , {        0  } ,
  { "F" }, { "N" }, { "V" },  {  "^" },      { "7"},  {     "/" } ,  {    "Delete" } , {        0  } ,
  { "G" }, { "O" }, { "W" },  {  "Delete" }, { "8"},  {     "0" } ,  {    "Insert" } , {        0  } 
};


static KeyboardScanner::ScanLayout g_microbeeKeys = {
  8, 8,
  keys,
  shiftedkeys
};


Microbee_Emulator::Microbee_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
  SetKeyboard(&m_keyboard);

  m_keyboard.Compile(g_microbeeKeys);

  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&Microbee_Emulator::OnPIOInterrupt, this, _1));

  m_crtc.SetLightPenHandler(std::bind(&Microbee_Emulator::OnKeyboardScan, this));

  //using namespace std::placeholders;
  //m_keyboard.SetHandler(true, std::bind(&Z80PIO::SetData, &m_pio, 0, _1));
}

void Microbee_Emulator::Init()
{  
  VirtualScreen::AddType<DG640>("dg640");
}

void Microbee_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);
  m_pio.Reset();
  m_crtc.Reset();
}

static int FindBitSet(uint8_t val)
{
  int pos = 0;
  while ((pos < 8) && !(val & 1)) {
    ++pos;
    val = val >> 1;
  }
  return pos;
}

uint16_t Microbee_Emulator::OnKeyboardScan()
{
  uint16_t newKeyboardCode = 0xffff;

  uint8_t mask = 1;
  bool found = false;
  for (int i = 0; i < 8; ++i) {
    uint8_t out = m_keyboard.Read(i);
    if (out != 0x00) {
      newKeyboardCode = (i << 7) + (FindBitSet(out) << 4);
      break;
    }
    mask = mask << 1;  
  }

  if (m_prevKeyboardCode != newKeyboardCode) {
    cout << "microbee: new keyboard code " << HEXFORMAT0x4(newKeyboardCode) << endl;
    m_prevKeyboardCode = newKeyboardCode;
  }

  return newKeyboardCode;
}

void Microbee_Emulator::OnPIOInterrupt(uint8_t vector)
{
  Interrupt(vector);
}

uint8_t Microbee_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:
      return m_pio.Read(port & 0x3);
    case 2:
      return m_crtc.Read(port & 1);
    case 3:
      return 0x00;
    case 4:
      return 0x00;
  }
  cerr << "microbee: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void Microbee_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      return m_pio.Write(port & 0x3, data);
    case 2:
      return m_crtc.Write(port & 1, data);
    case 3:
      return;
    case 4:
      return;
  }
  cerr << "microbee: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

