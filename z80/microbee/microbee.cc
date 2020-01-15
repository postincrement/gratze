

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

class Synertek6545 : public VirtualDevice
{
  public:
    const int m_maxReg = 19;    // don't include register 31

    enum StatusBits
    {
      eVBlanking   = 0x20,
      eLpenFull    = 0x40,
      eUpdateReady = 0x80
    };

    Synertek6545();

    virtual void Reset() override;

    virtual uint8_t GetStatus() const;
    virtual void SetStatus(uint8_t status);

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    void SetUpdateHandler(std::function<bool (bool, uint16_t &)> handler);
    void UpdateStatus();

    void Reg31();

  protected:
    int m_state = 0;
    uint8_t m_status = 0;
    uint8_t m_regSel = 0;
    bool m_writePending;
    std::vector<uint8_t> m_regs;
    std::function<bool (bool, uint16_t &)> m_updateHandler = nullptr;
};

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
    case 10:
    case 11:
      //cout << "6545: write cursor (" << ((m_regSel == 10) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[10] << 8) + m_regs[11]) << endl;
      break;
    case 12:
    case 13:
      //cout << "6545: write start address (" << ((m_regSel == 12) ? "H" : "L") << ")is " << HEXFORMAT0x4((m_regs[12] << 8) + m_regs[13]) << endl;
      break;
    case 14:
    case 15:
      //cout << "6545: write cursor address (" << ((m_regSel == 14) ? "H" : "L") << ") is " << HEXFORMAT0x4((m_regs[14] << 8) + m_regs[15]) << endl;
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

static Synertek6545 m_crtc;

/////////////////////////////////////////////////////////////////////////////////////////////


  /*
             p9  000 -----@ ........ G
            p10  001 -----H ........ O
    MA9     p11  010 -----P ........ W 
    MA8 --> p12  011 -----X ........ DEL
    MA7      p7  100 -----0 ........ 7
             p6  101 -----8 ........ /
             p5  110 ---ESC ........ SP
             p4  111 --CTRL ........ SHFT
                            |||||||| 
             p4  000 -------+|||||||  
             p3  001 --------+||||||
    MA6      p2  010 ---------+|||||    
    MA5 <--  p1  011 ----------+||||
    MA4     p15  100 -----------+|||
            p14  101 ------------+||
            p13  110 -------------+|
            p12  111 --------------+

  */

const KeyboardScanner::ScanCode keys[8*8] = {
  { "@" },      { "a" },          { "b" },    { "c" },    { "d"},       { "e" } ,       { "f" } ,     { "g" } ,
  { "h" },      { "i" },          { "j" },    { "k" },    { "l"},       { "m" } ,       { "n" } ,     { "o" } ,
  { "p" },      { "q" },          { "r" },    { "s" },    { "t"},       { "u" } ,       { "v" } ,     { "w" } ,
  { "x" },      { "y" },          { "z" },    { "[" },    { "\\" },     { "]" } ,       { "^" } ,     { "Delete" } ,
  { "0" },      { "1" },          { "2" },    { "3" },    { "4"},       { "5" } ,       { "6" } ,     { "7" } ,
  { "8" },      { "9" },          { ":" },    { ";" },    { ","},       { "-" } ,       { "." } ,     { "/" } ,
  { "Escape" }, { "Backspace" },  { "Tab" },  { "LF" },   { "Return"},  { "CapsLock" }, { "Break" },  { " " } ,
  { 0 },        { "Control" },    { 0 },      { 0 },      { 0 },        { 0 },          { 0 },        { "Shift" } 
};

const KeyboardScanner::ScanCode shiftedkeys[8*8] = {
  { "@" },      { "A" },          { "B" },    { "C" },    { "D"},       { "E" } ,       { "F" } ,     { "G" } ,
  { "H" },      { "I" },          { "J" },    { "K" },    { "L"},       { "M" } ,       { "N" } ,     { "O" } ,
  { "P" },      { "Q" },          { "R" },    { "S" },    { "T"},       { "U" } ,       { "V" } ,     { "W" } ,
  { "X" },      { "Y" },          { "Z" },    { "{" },    { "|" },      { "}" } ,       { "~" } ,     { "Delete" } ,
  { 0   },      { "!" },          { "\"" },   { "#" },    { "$"},       { "%" } ,       { "&" } ,     { "'" } ,
  { "(" },      { ")" },          { "*" },    { "+" },    { ","},       { "=" } ,       { "." } ,     { "?" } ,
  { "Escape" }, { "Backspace" },  { "Tab" },  { "LF" },   { "Return"},  { "CapsLock" }, { "Break" },  { " " } ,
  { 0 },        { "Control" },    { 0 },      { 0 },      { 0 },        { 0 },          { 0 },        { "Shift" } 
};


static KeyboardScanner::ScanLayout g_microbeeKeys = {
  8, 8,
  keys,
  shiftedkeys
};


Microbee_Emulator::Microbee_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Microbee_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  SetKeyboard(&m_keyboard);
  m_keyboard.Compile(g_microbeeKeys);

  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&Microbee_Emulator::OnPIOInterrupt, this, _1));

  m_crtc.SetUpdateHandler(std::bind(&Microbee_Emulator::OnKeyboardScan, this, _1, _2));

  //using namespace std::placeholders;
  //m_keyboard.SetHandler(true, std::bind(&Z80PIO::SetData, &m_pio, 0, _1));

  return true;
}

void Microbee_Emulator::Instantiate()
{  
  VirtualScreen::AddType<DG640>("dg640");
}

void Microbee_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);
  m_pio.Reset();
  m_crtc.Reset();
  //m_crtc.SetStatus(Synertek6545::eVBlanking); // always say we are in blanking
}

static int FindBitSet(uint8_t val)
{
  int pos = 0;
  while ((pos < 8) && ~(val & 1)) {
    ++pos;
    val = val >> 1;
  }
  return pos;
}

  // XX -- ---rr rccc ----


static std::string GetRowCol(uint16_t addr) 
{
  int row = (addr >> 7) & 0x7;
  int col = (addr >> 4) & 0x7;

  std::stringstream strm;
  strm << "addr " << HEXFORMAT0x4(addr) << " = col " << col << ", row " << row;
  return strm.str();
}  

bool Microbee_Emulator::ScanKeyboard(uint16_t & addr)
{
  uint8_t mask = 1;
  bool found = false;
  for (int row = 0; row < 8; row++) {
    uint8_t out = m_keyboard.Read(1 << row);
    if (out != 0x00) {
      addr = (row << 7) + (FindBitSet(out) << 4);
      //cout << "mbee: scan row " << (1 << row) << " returned " << HEXFORMAT0x2(out) << " -> " << GetRowCol(addr) << endl;;
      return true;
    }
    mask = mask << 1;  
  }

  return false;
}

bool Microbee_Emulator::OnKeyboardScan(bool doUpdate, uint16_t & addr)
{
  if (doUpdate) {
    //cout << "mbee: checking keyboard scan code " << GetRowCol(addr) << endl;

    int row = (addr >> 7) & 0x7;
    int col = (addr >> 4) & 0x7;

    uint8_t out = m_keyboard.Read(1 << row);
    if ((out & (1 << col)) != 0) {
      //cout << "mbee: update register matched" << endl;
      return true;
    }
    //cout << "mbee: no match on update register" << endl;
    return false;
  }

  if (ScanKeyboard(addr)) {
    //cout << "microbee: new keyboard code " << GetRowCol(addr) << endl;
    return true;
  }

  return false;
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

