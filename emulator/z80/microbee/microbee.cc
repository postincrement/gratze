

#include <iostream>
#include <iomanip>
#include <functional>

#include "common/misc.h"
#include "z80/microbee/microbee.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"
#include "video/dg640.h"

using namespace std;

#define   MICROBEE_VIDEO_START_ADDR   0xf000
#define   MICROBEE_PCG_START_ADDR     0xf800
#define   MICROBEE_VIDEO_END_ADDR     0xffff

#define   MICROBEE_SCREEN_COLS        64
#define   MICROBEE_SCREEN_ROWS        16

#define   MICROBEE_FONT_WIDTH         8
#define   MICROBEE_FONT_HEIGHT        16

#define   MICROBEE_VIRTUAL_FONT_CHARS    256

#define   MICROBEE_PCG_SIZE           0x800
#define   MICROBEE_ATTRIBUTE_SIZE     0x800

#define   MICROBEE_13_5_SPEED         (13.5 / 4)
#define   MICROBEE_12_SPEED           (12 / 4)

#define PORT_PIO        1      // PIO
#define PORT_6545       2      // 6545
#define PORT_FONT       3      // enable font ROM
#define PORT_COLOUR     4      // colour control port
#define PORT_WAITOFF    5      // wait off (write to 0x09)
#define PORT_VIATEL     6      // Viatel clock select (read of 0x09, A8)

#define PORT_FDC      0x10   // floppy controller
#define PORT_DRVSEL   0x11   // drive select

#define PORT_LVDAT    0x12   // LV DAT
#define PORT_BANK     0x13   // bank select

#define MEMORY_LOWER_BANK       0x21
#define MEMORY_UPPER_BANK       0x22
#define MEMORY_PCG              0x23

extern EmulatorInfo g_microbeeEmulatorInfo;

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

const ScannedKeyboard::ScanCode gameKeys[8*8] = {
  { "'" },      { "a" },          { "b" },    { "c" },    { "d"},       { "e" } ,       { "f" } ,     { "g" } ,
  { "h" },      { "i" },          { "j" },    { "k" },    { "l"},       { "m" } ,       { "n" } ,     { "o" } ,
  { "p" },      { "q" },          { "r" },    { "s" },    { "t"},       { "u" } ,       { "v" } ,     { "w" } ,
  { "x" },      { "y" },          { "z" },    { "[" },    { "'"},       { "]" } ,       { 0   } ,     { "Delete" } ,
  { "0" },      { "1" },          { "2" },    { "3" },    { "4"},       { "5" } ,       { "6" } ,     { "7" } ,
  { "8" },      { "9" },          { "-" },    { ";" },    { ","},       { "="   } ,     { "." } ,   { "/" } ,
  { "Escape" }, { "Backspace" },  { "Tab" },  { "\\"},    { "Return"},  { "CapsLock" }, { "Break" },  { " " } ,
  { "Up" },     { "Control" },    { "Down" }, { "Left" }, { 0 },        { 0 },          { "Right" },  { "Shift" }
};

const ScannedKeyboard::ScanCode keys[8*8] = {
  { "@" },      { "a" },          { "b" },    { "c" },    { "d"},       { "e" } ,       { "f" } ,     { "g" } ,
  { "h" },      { "i" },          { "j" },    { "k" },    { "l"},       { "m" } ,       { "n" } ,     { "o" } ,
  { "p" },      { "q" },          { "r" },    { "s" },    { "t"},       { "u" } ,       { "v" } ,     { "w" } ,
  { "x" },      { "y" },          { "z" },    { "[" },    { "\\" },     { "]" } ,       { "^" } ,     { "Delete" } ,
  { "0" },      { "1" },          { "2" },    { "3" },    { "4"},       { "5" } ,       { "6" } ,     { "7" } ,
  { "8" },      { "9" },          { ":" },    { ";" },    { ","},       { "-" } ,       { "." } ,     { "/" } ,
  { "Escape" }, { "Backspace" },  { "Tab" },  { "LF" },   { "Return"},  { "CapsLock" }, { "Break" },  { " " } ,
  { "Up" },     { "Control" },    { "Down" }, { "Left" }, { 0 },        { 0 },          { "Right" },  { "Shift" }
};

const ScannedKeyboard::ScanCode shiftedkeys[8*8] = {
  { "`" },      { "A" },          { "B" },    { "C" },    { "D"},       { "E" } ,       { "F" } ,     { "G" } ,
  { "H" },      { "I" },          { "J" },    { "K" },    { "L"},       { "M" } ,       { "N" } ,     { "O" } ,
  { "P" },      { "Q" },          { "R" },    { "S" },    { "T"},       { "U" } ,       { "V" } ,     { "W" } ,
  { "X" },      { "Y" },          { "Z" },    { "{" },    { "|" },      { "}" } ,       { "~" } ,     { "Delete" } ,
  { 0   },      { "!" },          { "\"" },   { "#" },    { "$"},       { "%" } ,       { "&" } ,     { "'" } ,
  { "(" },      { ")" },          { "*" },    { "+" },    { ","},       { "=" } ,       { "." } ,     { "?" } ,
  { "Escape" }, { "Backspace" },  { "Tab" },  { "LF" },   { "Return"},  { "CapsLock" }, { "Break" },  { " " } ,
  { "Up" },     { "Control" },    { "Down" }, { "Left" }, { 0 },        { 0 },          { "Right" },  { "Shift" }
};


static ScannedKeyboard::ScanLayout g_microbeeKeys = {
  8, 8,
  gameKeys,
  keys,
  shiftedkeys,
  { }
};

/////////////////////////////////////////////////////////////////////////////////////////////

class MicrobeeVideo : public ColourMemoryMappedScreen
{
  public:
    MicrobeeVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    static bool CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

    FontChar GetCharAtLoc(int addr) const override;

    virtual void RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

    virtual void WriteMemoryAtAddress(int addr, uint8_t ch)  override;
    virtual uint8_t ReadMemoryAtAddress(int addr) const override;

    virtual void SetScreenSize(int cols, int rows, int lines);

    void EnableFontROM(bool enable)
    { 
      m_fontROMEnabled = enable; 
    }

    void SetFontOffset(uint16_t offset)
    { m_fontOffset = offset; }

    uint16_t GetFontOffset() const
    { return m_fontOffset; }

    virtual bool IsPCG(FontChar ch) const override
    { return (ch >= 0x80) && (ch < 0x100); }

    void SetUpdateHandler(std::function<void ()> handler)
    { m_updateHandler = handler; }

    virtual void RewritePCG() override;

    void GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg) override;

    enum {
      eOriginal,
      eColour,
      eAlpha,
      TC256
    };

    void SetAlphaEnable(bool v);
    void SetAttributeEnable(bool v);
    void SetColourEnable(bool v);
    void SetPCGBank(int v);
    void SetViatel(bool enable);
    bool IsViatel() const { return m_viatel; }

  protected:
    int m_mode = eOriginal;
    bool m_colourRAMEnabled = false;
    bool m_attributeRAMEnabled = false;
    int m_bankSel = 0;
    bool m_viatel = false;

    int m_rows = 0;
    int m_cols = 0;
    int m_lines = 0;
    bool m_fontROMEnabled = false;
    uint16_t m_fontOffset = 0;
    std::function<void ()> m_updateHandler = nullptr;

    std::vector<uint8_t> m_attributeRAM;
    std::vector<uint8_t> m_colourRAM;
    std::vector<std::vector<uint8_t>> m_pcgRAM;
};


MicrobeeVideo::MicrobeeVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : ColourMemoryMappedScreen(mainWindow, options, info)
{
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);

  m_attributeRAM.resize(MICROBEE_ATTRIBUTE_SIZE);
  m_colourRAM.resize(MICROBEE_ATTRIBUTE_SIZE);

  m_pcgRAM.resize(16);
  for (int i = 0; i < 16; ++i) {
    m_pcgRAM[i].resize(MICROBEE_PCG_SIZE);
  }

  m_mode                = eOriginal;
  m_attributeRAMEnabled = false;
  m_colourRAMEnabled    = false;
  m_bankSel             = 0;

  InitPCG();
}

FontChar MicrobeeVideo::GetCharAtLoc(int loc) const
{
  FontChar ch = m_memory[loc & 0x7ff];
  return ch;
}

void MicrobeeVideo::SetAlphaEnable(bool v)
{
  if (m_mode == eOriginal)
    m_mode = eAlpha;
}

void MicrobeeVideo::SetAttributeEnable(bool v)
{
  if (v && (m_mode == eOriginal))
    m_mode = eColour;
  m_attributeRAMEnabled = v;
}

void MicrobeeVideo::SetColourEnable(bool v)
{
  if (v && (m_mode == eOriginal))
    m_mode = eColour;
  m_colourRAMEnabled = v;
}

void MicrobeeVideo::SetViatel(bool enable)
{
  if (enable == m_viatel)
    return;

  m_viatel = enable;
  // Half CRTC character clock → double-wide pixels; memory layout unchanged.
  SetColScale(m_viatel ? 2 : 1, 1);
  ResizeScreen();
  RefreshScreen();
  OnUpdate();
  cerr << "mbee: viatel " << (m_viatel ? "on" : "off") << endl;
}

void MicrobeeVideo::SetPCGBank(int v)
{
  m_bankSel = v;
}

static SDL_Colour g_premiumColours[16] = 
{   //R     G     B
  { 0x00, 0x00, 0x00, 0xff },  // black
  { 0x80, 0x00, 0x00, 0xff },  // red 
  { 0x00, 0x80, 0x00, 0xff },  // green
  { 0x80, 0x80, 0x00, 0xff },  // yellow
  { 0x00, 0x00, 0x80, 0xff },  // blue
  { 0x80, 0x00, 0x80, 0xff },  // magenta
  { 0x00, 0x80, 0x80, 0xff },  // cyan
  { 0x80, 0x80, 0x80, 0xff },  // white
  
  { 0x00, 0x00, 0x00, 0xff },  // black
  { 0xff, 0x00, 0x00, 0xff },  // red 
  { 0x00, 0xff, 0x00, 0xff },  // green
  { 0xff, 0xff, 0x00, 0xff },  // yellow
  { 0x00, 0x00, 0xff, 0xff },  // blue
  { 0xff, 0x00, 0xff, 0xff },  // magenta
  { 0x00, 0xff, 0xff, 0xff },  // cyan
  { 0xff, 0xff, 0xff, 0xff }   // white
};

enum {
  eBlack,  eRed2, eGreen2, eYellow2, eBlue2, eMagenta2, eCyan2, eWhite2,
  eBlack2, eRed,  eGreen,  eYellow,  eBlue,  eMagenta,  eCyan,  eWhite
};

static int g_originalFgColours[64] = 
{
  eBlack, eBlue, eGreen, eCyan, eRed, eMagenta, eYellow, eWhite,
  eBlack, eBlue, eGreen, eCyan, eRed, eMagenta, eYellow, eWhite,

  eBlack2, eBlue2, eGreen2, eCyan2, eRed2, eMagenta2, eYellow2, eWhite2,
  eBlack2, eBlue2, eGreen2, eCyan2, eRed2, eMagenta2, eYellow2, eWhite2
};

static int g_originalBgColours[8] = 
{
  eBlack, eRed, eGreen, eYellow, eBlue, eMagenta, eCyan, eWhite
};

void MicrobeeVideo::GetColourAtLoc(int loc, SDL_Colour & fg, SDL_Colour & bg)
{
  int bgIndex = eBlack;
  int fgIndex = eGreen;

  if (m_mode != eOriginal) {
    uint8_t attr = m_colourRAM[loc & 0x7ff];

    if (m_mode == eColour) {
      fgIndex = g_originalFgColours[attr & 0x1f];
      bgIndex = g_originalBgColours[(attr >> 5) & 0x7];
    }
    else 
    {
      fgIndex = attr & 0xf;
      bgIndex = (attr >> 4) & 0xf;
    }
  }

  fg = g_premiumColours[fgIndex];
  bg = g_premiumColours[bgIndex];
  bg.a = 0x00;
}

void MicrobeeVideo::RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  if (withCursor) {
    m_font->RenderChar(ch, renderer, dstRect, bg, fg);
  }
  else {
    m_font->RenderChar(ch, renderer, dstRect, fg, bg);
  }
}

void MicrobeeVideo::WriteMemoryAtAddress(int addr, uint8_t val)
{
  //cout << "mbeevideo: write " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(val) << endl;
  uint16_t offs  = addr & 0x0fff;
  uint16_t voffs = addr & 0x07ff;

  // upper bank is colour or PCG
  if (offs >= 0x800) {
    if ((m_mode != eOriginal) && m_colourRAMEnabled && !m_fontROMEnabled) {
      m_colourRAM[voffs] = val;
      RefreshCharAtLoc(voffs);
      return;
    }

    m_pcgRAM[(m_mode < eAlpha) ? 0 : m_bankSel][voffs] = val;
    int ch  = 128 + (voffs >> 4);
    int row = addr & 0xf;
    SetPCG(ch, row, val);
    return;
  }

  if ((m_mode != eOriginal) && m_attributeRAMEnabled) {
    m_attributeRAM[voffs] = val;
    RefreshCharAtLoc(voffs);
    return;
  }

  if (m_fontROMEnabled)
    return;

  // First non-space glyph into VDU after Viatel enable.
  if (m_viatel && val != 0x20) {
    static bool s_charLogged = false;
    if (!s_charLogged) {
      s_charLogged = true;
      cerr << "mbee: first char RAM " << HEXFORMAT0x4(voffs)
           << " = " << HEXFORMAT0x2(val) << endl;
    }
  }
  ColourMemoryMappedScreen::WriteMemoryAtAddress(voffs, val);
}

uint8_t MicrobeeVideo::ReadMemoryAtAddress(int addr) const
{
  //cout << "mbeevideo: read " << HEXFORMAT0x4(addr) << endl;
  uint16_t offs  = addr & 0x0fff;
  uint16_t voffs = addr & 0x07ff;
  
  // upper bank is colour or PCG
  if (offs >= 0x800) {
    if ((m_mode != eOriginal) && m_colourRAMEnabled && !m_fontROMEnabled) {
      return m_colourRAM[voffs];
    }
    return m_pcgRAM[(m_mode < eAlpha) ? 0 : m_bankSel][voffs];
  }

  // lower bank is font, attribute, or char
  if (m_fontROMEnabled) {
    const CharacterGeneratorROM * chargen = (m_fontOffset == 0) ? &g_charGen_mbee64x16 : &g_charGen_mbee80x24;
    return chargen->m_data[voffs];
  }

  if ((m_mode != eOriginal) && m_attributeRAMEnabled) {
    return m_attributeRAM[voffs];
  }

  return ColourMemoryMappedScreen::ReadMemoryAtAddress(voffs);
}

void MicrobeeVideo::SetScreenSize(int cols, int rows, int lines)
{
  m_cols  = cols;
  m_rows  = rows;
  m_lines = lines;

  if (lines <= 0)
    lines = m_lines = 16;

  if (m_options.m_verbose)  
    cout << "mbee: setting screen to " << m_cols << "x" << m_rows << "x" << lines << endl;
    
  PixelFont * font = new PixelFont(
      (m_fontOffset == 0) ? g_charGen_mbee64x16 : g_charGen_mbee80x24,
      256, 
      nullptr);
  font->SetHeight(lines);
  SetFont(font, m_cols, m_rows);
  RefreshScreen();
  OnUpdate();
}

void MicrobeeVideo::RewritePCG() 
{
  for (FontChar ch = 0x80; ch < 0x100; ++ch) {
    int offs = (ch - 0x80) * 16;
    SetPCG(ch, 0, 16, &m_pcgRAM[0][offs]);
  }  
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Microbee_Emulator::Instantiate()
{
  MemoryMappedScreen::AddType<MicrobeeVideo>("microbee");
}

Microbee_Emulator::Microbee_Emulator(const EmulatorInfo * info)
  : Z80Emulator(info)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Microbee_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  ScannedKeyboard * kb = new ScannedKeyboard();
  kb->SetGameMode(options.m_gameKb);
  kb->SetDebug(options.m_keyboardDebug);
  SetKeyboard(kb);
  kb->Compile(g_microbeeKeys);

  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&Microbee_Emulator::OnPIOInterrupt, this, _1));

  m_crtc.SetScreenShapeHandler(std::bind(&Microbee_Emulator::OnSetScreenSize, this, _1, _2, _3));
  m_crtc.SetStartAddressHandler(std::bind(&Microbee_Emulator::OnSetVideoStartAddress, this, _1));
  m_crtc.SetCursorAddressHandler(std::bind(&Microbee_Emulator::OnSetCursorAddress, this, _1));
  m_crtc.SetCursorShapeHandler(std::bind(&Microbee_Emulator::OnSetCursorShape, this, _1, _2, _3));
  m_crtc.SetUpdateHandler(std::bind(&Microbee_Emulator::OnKeyboardScan, this, _1, _2));
  // 13.5 MHz / 4 CPU, 13.5 MHz / 8 character clock → 2 CPU cycles per char.
  m_crtc.SetCPUCyclesPerChar(2);

  return true;
}

void Microbee_Emulator::Reset(int addr)
{
  m_video = (MicrobeeVideo *)m_screen.get();

  Z80Emulator::Reset(addr);
  m_pio.Reset();
  // Serial/modem status on PIO B: bits 3 and 4 idle-high.
  m_pio.SetData(1, 0x18);
  m_crtc.Reset();
  m_crtc.SetCPUCyclesPerChar(2);
  // Viatel half-clock is sticky until software reselects it via IN 09h.
  if (m_video)
    m_video->SetViatel(false);
}

int Microbee_Emulator::Exec(int cycles)
{
  // Tick the CRTC in short bursts so status bit 5 tracks the real frame
  // blanking interval even when software is spinning on IN A,(0Ch).
  int left = cycles;
  while (left > 0) {
    int chunk = std::min(left, 64);
    int rem = Z80Emulator::Exec(chunk);
    int done = chunk - rem;
    if (done < 1)
      done = 1;
    m_crtc.Tick(done);
    left -= done;
  }
  return left;
}

static int FindBitSet(uint8_t val)
{
  int pos = 0;
  while ((pos < 8) && ((val & 1) == 0)) {
    ++pos;
    val = val >> 1;
  }
  return pos;
}

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
    uint8_t out = m_keyboard->Read(1 << row);
    if (out != 0x00) {
      int col = FindBitSet(out);
      //cout << "scan out " << hex << (int)out << " = " << row << ", " << col << endl;
      addr = (row << 7) + (col << 4);
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

    uint8_t out = m_keyboard->Read(1 << row);
    if ((out & (1 << col)) != 0) {
      //cout << "mbee: update register matched" << endl;
      return true;
    }
    //cout << "mbee: no match on update register" << endl;
    return false;
  }

  {
    static uint16_t s_last = 0xffff;
    if (ScanKeyboard(addr)) {
      if (addr != s_last) {
        s_last = addr;
        cerr << "kb: scan " << GetRowCol(addr) << endl;
      }
      return true;
    }
    s_last = 0xffff;
  }

  return false;
}

void Microbee_Emulator::OnSetScreenSize(int cols, int rows, int lines)
{
  ++lines;
  if (
      (m_screen->GetRows() == rows) && 
      (m_screen->GetCols() == cols) && 
      (m_lines == lines)
     )
    return;

  m_lines = lines;
  m_video->SetScreenSize(cols, rows, lines);
  m_video->Update(true);
}

void Microbee_Emulator::OnSetVideoStartAddress(uint16_t addr)
{
  uint16_t memOffset  = addr & 0x7ff;
  uint16_t fontOffset = ((addr & (1 << 13)) != 0) ? 0x800 : 0x000;
  m_video->SetOffset(memOffset);

  if (fontOffset != m_fontOffset) {
    m_fontOffset = fontOffset;
    m_video->SetFontOffset(fontOffset);
    m_video->SetScreenSize(m_screen->GetCols(), m_screen->GetRows(), m_lines);
  }
}

void Microbee_Emulator::OnSetCursorAddress(uint16_t addr)
{
  //cout << "Set cursor to " << HEXFORMAT0x4(addr) << endl;
  m_screen->SetCursorPos(addr % m_screen->GetCols(), addr / m_screen->GetCols());
}

void Microbee_Emulator::OnSetCursorShape(uint8_t start, uint8_t end, int blinkRate)
{
  //cout << "mbee: cursor start row = " << (int)start << ", end = " << (int)end << ", rate = " << (int)blinkRate << endl;
  if (blinkRate < 0)
    m_screen->EnableCursor(false);
  else {
    m_screen->EnableCursor(true);
  }
}

void Microbee_Emulator::OnPIOInterrupt(uint8_t vector)
{
  Interrupt(vector);
}

uint8_t Microbee_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case PORT_PIO:
      return m_pio.Read(port & 0x3);
    case PORT_6545:
      return m_crtc.Read(port & 1);
    case PORT_VIATEL:
      // Older Viatel board: half CRTC clock is selected by A8 on a read of
      // port 0x09 (IN A,(09h) with A holding the select bit).
      {
        bool enable = (port & 0x100) != 0;
        m_video->SetViatel(enable);
        // Half character clock → twice as many CPU cycles per CRTC tick.
        m_crtc.SetCPUCyclesPerChar(enable ? 4 : 2);
      }
      return (port >> 8) & 1;
    //case PORT_FONT:
    //  return 0x00;
    //case PORT_COLOUR:
    //  return 0x00;
  }
  return 0x00;
}

void Microbee_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_PIO:
      switch (port & 0x3) {
        case 0:
          //cerr << "PIO A data write: " << HEXFORMAT0x2(data) << endl;
          break;
        case 1:  
          //cerr << "PIO A control write: " << HEXFORMAT0x2(data) << endl;
          break;
        case 2:
          //cerr << "PIO B data write: " << HEXFORMAT0x2(data) << endl;
          break;
        case 3:  
          //cerr << "PIO B control write: " << HEXFORMAT0x2(data) << endl;
          break;
      }
      return m_pio.Write(port & 0x3, data);
    case PORT_6545:
      return m_crtc.Write(port & 1, data);
    case PORT_FONT:
      m_video->EnableFontROM((data & 1) != 0);
      return;
    case PORT_COLOUR:
      m_video->SetColourEnable((data & 0x40) != 0);
      return;
    case PORT_WAITOFF:
      // Any write clears CPU wait; data is ignored.
      return;
    case PORT_LVDAT:
      m_video->SetPCGBank        (data & 0x0f);
      m_video->SetAttributeEnable(data & 0x10);
      m_video->SetAlphaEnable    (data & 0x80);
      return;
  }
}

void Microbee_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  if (id == MEMORY_PCG) {
    m_video->WriteMemoryAtAddress(addr, val);
    return;
  }
  Z80Emulator::WriteIOMemory(id, addr, val);
}

uint8_t Microbee_Emulator::ReadIOMemory(int id, uint16_t addr) const
{
  if (id == MEMORY_PCG) {
    return m_video->ReadMemoryAtAddress(addr);
  }
  return Z80Emulator::ReadIOMemory(id, addr);
}

/////////////////////////////////////////////////////////////////////////////////////////////

MicrobeeDisk_Emulator::MicrobeeDisk_Emulator(const EmulatorInfo * info)
  : Microbee_Emulator(info)
{
}

bool MicrobeeDisk_Emulator::MountDrive(int driveNum, std::shared_ptr<VirtualDrive> drive, bool readOnly)
{
  return m_fdc->MountDrive(driveNum, drive, readOnly);
}

void MicrobeeDisk_Emulator::RefreshPanelDrives()
{
  CollectFdcDrives(m_fdc.get());
}

bool MicrobeeDisk_Emulator::Open(const Options & options)
{
  if (!Microbee_Emulator::Open(options))
    return false;

  m_fdc.reset(new WD_FD1793(options.m_fdcDebug));

  return true;
}

uint8_t MicrobeeDisk_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case PORT_FDC:
      if (!m_fdc)
        return 0;
      return m_fdc->Read(port & 0x3);
    case PORT_DRVSEL:
      if (!m_fdc)
        return 0;
      return (m_fdc->GetDRQ() || m_fdc->GetInterrupt()) ? 0x80 : 0x00;
  }
  return Microbee_Emulator::ReadIOPort(info, port);
}

void MicrobeeDisk_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_FDC:
      if (m_fdc)
        m_fdc->Write(port & 0x3, data);
      return;

    case PORT_DRVSEL:
      if (!m_fdc)
        return;
      m_drive   = data & 0x03;
      m_fdc->SelectDrive(m_drive);
      m_side    = (data & 0x04) != 0;
      m_fdc->SelectSide(m_side);
      m_density = (data & 0x08) != 0;
      return;
  }
  return Microbee_Emulator::WriteIOPort(info, port, data);
}

/////////////////////////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_microbee32EmulatorInfo;

Microbee32_Emulator::Microbee32_Emulator()
  : Microbee_Emulator(&g_microbee32EmulatorInfo)
{}

/////////////////////////////////////////////////////////////////////////////////////////////

#define MICROBEE_FIXED_VIDEO() \
  INFO_SCREEN_MEMORY_MAPPED_FIXED("microbee", \
                          MICROBEE_VIDEO_START_ADDR, MICROBEE_PCG_START_ADDR-1, \
                          MICROBEE_SCREEN_COLS, MICROBEE_SCREEN_ROWS, \
                          MICROBEE_FONT_WIDTH, MICROBEE_FONT_HEIGHT, \
                          MICROBEE_VIRTUAL_FONT_CHARS, \
                          &g_charGen_mbee64x16, nullptr), \
  INFO_MEM_IO(MICROBEE_PCG_START_ADDR, MICROBEE_VIDEO_END_ADDR, MEMORY_PCG)                        

#define MICROBEE_VARIABLE_VIDEO() \
  INFO_SCREEN_MEMORY_MAPPED_VARIABLE("microbee", 0x800, \
                          MICROBEE_SCREEN_COLS, MICROBEE_SCREEN_ROWS, \
                          MICROBEE_FONT_WIDTH, MICROBEE_FONT_HEIGHT, \
                          MICROBEE_VIRTUAL_FONT_CHARS, \
                          &g_charGen_mbee64x16, nullptr)

INFO_START(microbee)
{
  INFO_IO_PORT_RW(0x00, 0x03, PORT_PIO),      // PIO
  INFO_IO_PORT_RW(0x08, 0x08, PORT_COLOUR),   // colour control port
  INFO_IO_PORT_WRITE(0x09, 0x09, PORT_WAITOFF), // wait off
  INFO_IO_PORT_READ(0x09, 0x09, PORT_VIATEL),  // Viatel clock select (A8)
  INFO_IO_PORT_RW(0x0b, 0x0b, PORT_FONT),     // enable font ROM
  INFO_IO_PORT_RW(0x0c, 0x0d, PORT_6545),     // 6545
  INFO_IO_PORT_RW(0x1c, 0x1c, PORT_LVDAT),    // LVDAT

  INFO_MONITOR(12.0, 4.0, 3.0, ePAL),
}
INFO_END(microbee);

///////////////////////////////////////////////////////////////////////////

#define   MICROBEE_BASIC_ROM_START_ADDR     0x8000
#define   MICROBEE_BASIC_ROM_END_ADDR       0xbfff

#define   MICROBEE_BASIC_RAM_START_ADDR     0x0000
#define   MICROBEE_BASIC_RAM_END_ADDR       0x7fff

extern unsigned char g_microbeeBasic5_22e_ROM[16384];

INFO_START(microbee32)
{
  INFO_CPU(MICROBEE_13_5_SPEED, MICROBEE_BASIC_ROM_START_ADDR),
  INFO_PARENT(microbee),
  INFO_ROM(MICROBEE_BASIC_ROM_START_ADDR, g_microbeeBasic5_22e_ROM),
  INFO_MAIN_RAM(MICROBEE_BASIC_RAM_START_ADDR, 32, 16, MICROBEE_BASIC_ROM_START_ADDR / 1024),

  MICROBEE_FIXED_VIDEO()
}
INFO_END(microbee32);


EmulatorInfo g_microbee32EmulatorInfo =
{
  "mbee32",                  // command line option
  "Microbee 32k",            // short name
  "Microbee 32k",            // long name
  true,                      // SDL window

  INFO_INSERT(microbee32)
};

///////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_microbee56EmulatorInfo;

Microbee56_Emulator::Microbee56_Emulator()
  : MicrobeeDisk_Emulator(&g_microbee56EmulatorInfo)
{
}

#define   MICROBEE_56k_DISK_ROM_START_ADDR     0xe000
#define   MICROBEE_56k_DISK_ROM_END_ADDR       0xefff

#define   MICROBEE_56k_DISK_RAM_START_ADDR     0x0000
#define   MICROBEE_56k_DISK_RAM_END_ADDR       0xdfff

extern unsigned char g_disk56_ROM[4096];

INFO_START(microbee56)
{
  INFO_CPU(MICROBEE_13_5_SPEED, MICROBEE_56k_DISK_ROM_START_ADDR),
  INFO_PARENT(microbee),
  INFO_ROM(MICROBEE_56k_DISK_ROM_START_ADDR, g_disk56_ROM),
  INFO_MAIN_RAM(MICROBEE_56k_DISK_RAM_START_ADDR, 56, 56, MICROBEE_56k_DISK_RAM_END_ADDR / 1024),

  MICROBEE_FIXED_VIDEO(),

  INFO_IO_PORT_RW(0x44, 0x47, PORT_FDC),     // FDC
  INFO_IO_PORT_RW(0x48, 0x48, PORT_DRVSEL)   // drive select
}
INFO_END(microbee56);

EmulatorInfo g_microbee56EmulatorInfo =
{
  "mbee56",                  // command line option
  "Microbee 56k",            // short name
  "Microbee 56k",            // long name
  true,                      // SDL window

  INFO_INSERT(microbee56)
};

/////////////////////////////////////////////////////////////////////////////////////////////

extern unsigned char g_disk128_ROM[8192];
extern unsigned char g_bn5443_ROM[8192];

extern EmulatorInfo g_microbee128EmulatorInfo;
extern EmulatorInfo g_microbee128bnEmulatorInfo;

Microbee128_BaseEmulator::Microbee128_BaseEmulator(const unsigned char * rom, const EmulatorInfo * info)
  : MicrobeeDisk_Emulator(info)
  , m_0x8000_ROM(rom)
{
}

bool Microbee128_BaseEmulator::Open(const Options & options)
{
  if (!MicrobeeDisk_Emulator::Open(options))
    return false;

  return true;
}

void Microbee128_BaseEmulator::Reset(int addr)
{
  MicrobeeDisk_Emulator::Reset(addr);
  SetBankSel(0x00);
}

#define UPPER_BANK   0

// 0x00 = 0 0000   => RESET MAP  DRAM bank 0, VDU from F000 
// 0x0C = 0 1100   => BIOS MAP   DRAM bank 0, no VDU
// 0x14 = 1 0100   => VDR MAP    DRAM bank 0, VDU from 8000
// 0x0D = 0 1101   => BANK A     0-7fff = DRAM bank 1 lower, 8000-ffff DRAM bank 0 upper
// 0x0F = 0 1111   => BANK B     0-7fff = DRAM bank 1 upper, 8000-ffff DRAM bank 0 upper

static int g_bankMap[4] = {
  0,    // 00 -> bank 0, lower
  2,    // 01 -> bank 1, lower
  2,    // 10 -> bank 1, lower ??
  3,    // 11 -> bank 1, upper
};

void Microbee128_BaseEmulator::SetBankSel(uint8_t data)
{
  const bool prevVideoDisable = m_videoDisable;
  m_bankSel          = data;
  m_currentLowerBank = m_bankedMemory[data & 3]; //g_bankMap[data & 3]];
  m_romDisable       = data & 0x04;
  m_videoDisable     = data & 0x08;
  m_videoLower       = data & 0x10;
  // Blank-screen clue: first flip only (BIOS banks video around Starnet I/O).
  if (m_videoDisable != prevVideoDisable) {
    // Count is owned by Starnet client when present; base just stays quiet
    // after the first transition pair would otherwise flood.
    static int s_videoFlips = 0;
    if (s_videoFlips++ < 2)
      cerr << "mbee128: video " << (m_videoDisable ? "disabled" : "enabled")
           << " bank " << HEXFORMAT0x2(data)
           << " pc " << HEXFORMAT0x4(GetPC()) << endl;
  }
}

uint8_t Microbee128_BaseEmulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
//  switch (info.m_id) {
//    case PORT_BANK:
//      return m_bankSel;
//    case PORT_LVDAT:
//  }
  return MicrobeeDisk_Emulator::ReadIOPort(info, port);
}

void Microbee128_BaseEmulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_BANK:
      SetBankSel(data);
      return;
  }
  return MicrobeeDisk_Emulator::WriteIOPort(info, port, data);
}

void Microbee128_BaseEmulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  switch (id) {
    case MEMORY_LOWER_BANK:
      m_currentLowerBank[addr] = val;
      return;

    case MEMORY_UPPER_BANK:
      if (!m_videoDisable) {
        // videoLower (bank 0x14) puts the VDU at 8000. Hardware decodes a
        // 4K window, but vtex keeps SP at 9000h — pushes land at 8FFx.
        // Starnet's sub_db56h also EI while bank 14 is active, so an IRQ
        // would write its frame into VDU and lose the DRAM return address.
        // Map only the 2K screen (80x24) at 8000-87FF; keep 8800-8FFF as
        // DRAM for the TPA stack.
        if (m_videoLower && addr >= 0x8000 && addr < 0x8800) {
          m_video->WriteMemoryAtAddress(addr & 0x7ff, val);
          return;
        }
        if (!m_videoLower && (addr >= 0xf000)) {
          m_video->WriteMemoryAtAddress(addr & 0x0fff, val);
          return;
        }
      }
      if (!m_romDisable && (addr < 0xa000)) {
        return;
      }
      m_bankedMemory[UPPER_BANK][addr & 0x7fff] = val;
      return;
  }
  return MicrobeeDisk_Emulator::WriteIOMemory(id, addr, val);
}


uint8_t Microbee128_BaseEmulator::ReadIOMemory(int id, uint16_t addr) const
{
  switch (id) {
    case MEMORY_LOWER_BANK:          // 0x0000 to 0x7ffff
      return m_currentLowerBank[addr];

    case MEMORY_UPPER_BANK:          // 0x8000 to 0xffff
      if (!m_videoDisable) {
        if (m_videoLower && addr >= 0x8000 && addr < 0x8800) {
          return m_video->ReadMemoryAtAddress(addr & 0x7ff);
        }
        if (!m_videoLower && (addr >= 0xf000)) {
          return m_video->ReadMemoryAtAddress(addr & 0x0fff);
        }
      }
      if (!m_romDisable && (addr < 0xa000)) {
        return m_0x8000_ROM[addr & 0x1fff];
      }
      return m_bankedMemory[UPPER_BANK][addr & 0x7fff];
  }
  
  return MicrobeeDisk_Emulator::ReadIOMemory(id, addr);
}

#define   MICROBEE_128k_DISK_ROM_START_ADDR     0x8000

Microbee128_Emulator::Microbee128_Emulator()
  : Microbee128_BaseEmulator(g_disk128_ROM, &g_microbee128EmulatorInfo)
{
}

INFO_START(microbee128)
{
  INFO_CPU(MICROBEE_13_5_SPEED, MICROBEE_128k_DISK_ROM_START_ADDR),
  INFO_PARENT(microbee),

  INFO_MEM_IO(0x0000, 0x7fff, MEMORY_LOWER_BANK),
  INFO_MEM_IO(0x8000, 0xffff, MEMORY_UPPER_BANK),

  MICROBEE_VARIABLE_VIDEO(),

  INFO_IO_PORT_RW(0x44, 0x47, PORT_FDC),     // FDC
  INFO_IO_PORT_RW(0x48, 0x48, PORT_DRVSEL),  // drive select

  INFO_IO_PORT_RW(0x50, 0x50, PORT_BANK)     // memory bank select
}
INFO_END(microbee128);

////////////////////////////////////////////////////////////////////////////

Microbee128_BN_Emulator::Microbee128_BN_Emulator()
  : Microbee128_BaseEmulator(g_bn5443_ROM, &g_microbee128bnEmulatorInfo)
{
}

Microbee128_BN_Emulator::Microbee128_BN_Emulator(const EmulatorInfo * info)
  : Microbee128_BaseEmulator(g_bn5443_ROM, info)
{
}

EmulatorInfo g_microbee128EmulatorInfo =
{
  "mbee128",                  // command line option
  "Microbee 128k",            // short name
  "Microbee 128k",            // long name
  true,                       // SDL window

  INFO_INSERT(microbee128)
};

INFO_START(microbee128bn)
{
  INFO_CPU(MICROBEE_13_5_SPEED, MICROBEE_128k_DISK_ROM_START_ADDR),
  INFO_PARENT(microbee128)
}
INFO_END(microbee128bn);

EmulatorInfo g_microbee128bnEmulatorInfo =
{
  "mbee128bn",                  // command line option
  "Microbee 128k BN",            // short name
  "Microbee 128k BN",            // long name
  true,                          // SDL window

  INFO_INSERT(microbee128bn)
};

/////////////////////////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_microbee128starnetEmulatorInfo;

Microbee128_StarnetClient_Emulator::Microbee128_StarnetClient_Emulator()
: Microbee128_BN_Emulator(&g_microbee128starnetEmulatorInfo)
{
  using namespace std::placeholders;
  m_pio.SetWriteHandler(0, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOAWrite, this, _1, _2));
  m_pio.SetReadHandler (0, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOARead,  this));
  m_pio.SetWriteHandler(1, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOBWrite, this, _1, _2));
  m_pio.SetReadHandler (1, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOBRead,  this));
}

bool Microbee128_StarnetClient_Emulator::Open(const Options & options)
{
  if (!Microbee128_BN_Emulator::Open(options))
    return false;

  m_fdc.reset();

  if (options.m_starnetStation >= 0 && options.m_starnetStation < Starnet::kStationCount)
    m_client.SetStation((uint8_t)options.m_starnetStation);

  return true;
}

void Microbee128_StarnetClient_Emulator::Reset(int addr)
{
  Microbee128_BN_Emulator::Reset(addr);
  m_transmit = false;
  m_rxPhase = RxPhase::Sync;
  m_bodyLeft = 0;
  m_logNextInterrupt = false;
  m_seenBoot = false;
  m_seenSetup = false;
  m_seenCcp = false;
  m_seenBiosRead = false;
  m_seenHalt = false;
  m_bootProbeDone = false;
  m_logLastPc = 0;
  m_logSamePcCycles = 0;
  m_logHeartbeatCycles = 0;
  m_logPioBReads = 0;
  m_logPioBWrites = 0;
  m_logCrtcStatus = 0;
  m_bdos6Pending = false;
  m_bdos6Return = 0;
  m_bdos6Steps = 0;
  m_logBdos6 = 0;
  m_logKeyWait = 0;
  m_keyWaitPending = false;
  m_seenKeyWait = false;
  m_seenProbeMemory = false;
  m_seenLoadCustomer = false;
  m_seenMainLoop = false;
  m_seenMenu = false;
  m_seenDrawHelp = false;
  m_seenReadCmdline = false;
  m_seenBackToCcp = false;
  m_loggedBdos6Poll = false;
  m_starnetDirFlips = 0;
  m_blockedDf4a = 0;
  m_blockedPioCtrl = 0;
  m_prevPc = 0;
  m_loadCustomerRet = 0;
  m_watchingLoadCustomer = false;
  m_bdosPending = false;
  m_bdosFn = 0;
  m_bdosReturn = 0;
  m_client.Reset();
}

void Microbee128_StarnetClient_Emulator::ProbeBoot()
{
  if (m_bootProbeDone)
    return;

  uint16_t pc = (uint16_t)GetPC();
  if (!m_seenBoot && pc == 0xd600)
    m_seenBoot = true;
  if (!m_seenSetup && pc == 0xd887)
    m_seenSetup = true;
  if (!m_seenCcp && pc == 0xc000)
    m_seenCcp = true;
  if (!m_seenBiosRead && pc == 0xd9ff)
    m_seenBiosRead = true;
  if (!m_seenHalt && m_seenSetup
      && (m_cpu.IFF & (IFF_HALT | IFF_1)) == (IFF_HALT | IFF_1))
    m_seenHalt = true;

  // Keep stepping until we reach disk I/O, or we stop in HALT after the
  // banners (with or without entering the CCP).
  if (m_seenBiosRead || (m_seenHalt && (m_seenSetup || m_seenCcp)))
    m_bootProbeDone = true;
}

void Microbee128_StarnetClient_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  // EDB0 is ED B0. While that LDIR is in progress the PC is EDB2.
  if (addr == 0xdf45 && (GetPC() == 0xedb0 || GetPC() == 0xedb2))
    m_logNextInterrupt = true;

  // vtex hw_init patches DF4A with a modem bit-bang ISR when it sees a
  // D6xx BIOS. That collides with Starnet's IM2 vector table at DF48 and
  // sends the next interrupt into TPA modem code → stack trash → CCP.
  if (addr >= 0xdf48 && addr <= 0xdf4f && (uint16_t)GetPC() < 0x8000) {
    if (m_blockedDf4a++ == 0)
      cerr << "vtex: blocked DF4A modem ISR patch (Starnet vectors)"
           << " pc " << HEXFORMAT0x4(GetPC()) << endl;
    return;
  }

  // vtex SP sits at 9000h (upper half). Log anyone stomping the cold_start
  // return slot while load_customer_id is on the stack.
  if (m_watchingLoadCustomer && m_loadCustomerRet != 0
      && (addr == 0x8ffe || addr == 0x8fff)
      && (uint16_t)GetPC() >= 0x8000) {
    cerr << "vtex: stack stomp @" << HEXFORMAT0x4(addr)
         << " =" << HEXFORMAT0x2(val)
         << " pc " << HEXFORMAT0x4(GetPC())
         << " sp " << HEXFORMAT0x4(m_cpu.SP.W) << endl;
  }

  Microbee128_BN_Emulator::WriteIOMemory(id, addr, val);
}

void Microbee128_StarnetClient_Emulator::Interrupt(uint16_t vector)
{
  if (m_logNextInterrupt && !m_inExec)
    m_logNextInterrupt = false;
  Microbee128_BN_Emulator::Interrupt(vector);
}

int Microbee128_StarnetClient_Emulator::Exec(int cycles)
{
  // EE35 arms vector 0x4E and then executes EI; HALT. A strobe raised on
  // the IN after the earlier EI is taken as vector 0x4C instead.
  WakeReceive();

  // Step one instruction at a time until the cold-boot probes have fired
  // so we cannot skip D887 / C000 / D9FF inside a multi-cycle ExecZ80.
  if (!m_bootProbeDone) {
    int toDo = cycles;
    while (toDo > 0 && !m_bootProbeDone) {
      ProbeBoot();
      int remaining = Microbee128_BN_Emulator::Exec(1);
      int done = 1 - remaining;
      if (done <= 0)
        done = 1;
      toDo -= done;
      ProbeBoot();
    }
    if (toDo > 0)
      return Microbee128_BN_Emulator::Exec(toDo);
    return toDo;
  }

  // vtex_153.asm cold_start: probe_memory → load_customer_id (BDOS
  // open/read VTLCUSTOMER) → screen_buf_init → cells_clear → main_loop
  // → menu_command → key_wait. Until key_wait, single-step TPA so BDOS
  // entries at 0005 are not skipped inside a 256-cycle slice.
  int rem = cycles;
  while (rem > 0) {
    const uint16_t pc = (uint16_t)GetPC();
    const bool preKeyWait = m_seenProbeMemory && !m_seenKeyWait;

    if (!m_seenProbeMemory && (pc == 0x1ca8 || pc == 0x1ce1)) {
      m_seenProbeMemory = true;
      cerr << "vtex: probe_memory l1c41="
           << HEXFORMAT0x2(ReadMemory(0x1c41)) << endl;
    }
    if (!m_seenLoadCustomer && pc == 0x01de) {
      m_seenLoadCustomer = true;
      if (!m_seenProbeMemory) {
        m_seenProbeMemory = true;
        cerr << "vtex: probe_memory (implied) l1c41="
             << HEXFORMAT0x2(ReadMemory(0x1c41)) << endl;
      }
      m_loadCustomerRet = ReadMemoryWord(m_cpu.SP.W);
      m_watchingLoadCustomer = true;
      cerr << "vtex: load_customer_id ret=" << HEXFORMAT0x4(m_loadCustomerRet)
           << " sp " << HEXFORMAT0x4(m_cpu.SP.W) << endl;
    }

    // Force a clean return from load_customer_id — don't trust the stack
    // slot after Starnet BDOS (corruption lands at 025e → menu → CCP).
    if (m_watchingLoadCustomer && (pc == 0x0212 || pc == 0x01eb)) {
      const uint16_t got = ReadMemoryWord(m_cpu.SP.W);
      const uint16_t want = m_loadCustomerRet ? m_loadCustomerRet : 0x0191;
      if (got != want)
        cerr << "vtex: load_customer ret was " << HEXFORMAT0x4(got)
             << " (want " << HEXFORMAT0x4(want) << ")" << endl;
      m_watchingLoadCustomer = false;
      m_cpu.SP.W = 0x9000;
      SetPC(want);
      continue;
    }

    // Fallen into menu via 025e (data as code).
    if (pc == 0x025e && m_cpu.SP.W == 0x9000) {
      cerr << "vtex: abort fall-through at 025e -> 0191"
           << " bank " << HEXFORMAT0x2(m_bankSel) << endl;
      SetPC(0x0191);
      continue;
    }

    // Empty-stack RET at the vtex SP base pops leftover (9000) — observed
    // as 3e71 — then falls through NOPs into 4000h which holds JP C082
    // (CCP). Block that RET and resume the cmdline loop.
    if (m_seenDrawHelp && !m_seenBackToCcp && m_cpu.SP.W == 0x9000) {
      const uint8_t op = ReadMemory(pc);
      const bool isRet = (op == 0xc9) || (op == 0xc8) || (op == 0xc0)
                      || (op == 0xd8) || (op == 0xd0) || (op == 0xe8)
                      || (op == 0xe0) || (op == 0xf8) || (op == 0xf0);
      const bool isReti = (op == 0xed && ReadMemory(pc + 1) == 0x4d);
      if (isRet || isReti) {
        cerr << "vtex: block empty-stack RET at " << HEXFORMAT0x4(pc)
             << " (9000)=" << HEXFORMAT0x4(ReadMemoryWord(0x9000)) << endl;
        m_cpu.SP.W = 0x8ffe;
        WriteMemory(0x8ffe, 0x9d); // return to main_loop after menu_command
        WriteMemory(0x8fff, 0x01);
        SetPC(0x0282); // clear_cmdline / read_cmdline
        continue;
      }
    }

    // Trampoline left in RAM: C3 82 C0 = JP CCP.
    if (!m_seenBackToCcp && pc == 0x4000
        && ReadMemory(0x4000) == 0xc3
        && ReadMemoryWord(0x4001) == 0xc082) {
      cerr << "vtex: block JP CCP trampoline at 4000"
           << " prev " << HEXFORMAT0x4(m_prevPc) << endl;
      m_cpu.SP.W = 0x8ffe;
      WriteMemory(0x8ffe, 0x9d);
      WriteMemory(0x8fff, 0x01);
      SetPC(0x0282);
      continue;
    }

    if (!m_seenMainLoop && pc == 0x0197) {
      m_seenMainLoop = true;
      cerr << "vtex: main_loop" << endl;
    }
    if (!m_seenMenu && pc == 0x0260) {
      m_seenMenu = true;
      cerr << "vtex: menu_command"
           << " sp " << HEXFORMAT0x4(m_cpu.SP.W)
           << " [sp] " << HEXFORMAT0x4(ReadMemoryWord(m_cpu.SP.W))
           << " a8 " << HEXFORMAT0x2(ReadMemory(0x1a8))
           << " prev " << HEXFORMAT0x4(m_prevPc)
           << endl;
      // Not a real CALL from main_loop (that would have SP=8FFE, [sp]=019d).
      if (m_cpu.SP.W == 0x9000) {
        cerr << "vtex: menu entered without CALL — forcing main_loop" << endl;
        SetPC(0x0197);
        m_seenMenu = false;
        continue;
      }
    }
    if (!m_seenDrawHelp && pc == 0x0309) {
      m_seenDrawHelp = true;
      cerr << "vtex: draw_help_menu" << endl;
    }
    if (!m_seenReadCmdline && pc == 0x0369) {
      m_seenReadCmdline = true;
      cerr << "vtex: read_cmdline" << endl;
    }

    // CCP entry is C000→C082. SP=9002 means RET with SP=9000 to C000
    // (BIOS warm boot would have set SP to D682 first).
    if (m_seenProbeMemory && !m_seenKeyWait && !m_seenBackToCcp
        && pc >= 0xc000 && pc < 0xc800) {
      m_seenBackToCcp = true;
      cerr << "vtex: returned to CCP pc " << HEXFORMAT0x4(pc)
           << " sp " << HEXFORMAT0x4(m_cpu.SP.W)
           << " [sp] " << HEXFORMAT0x4(ReadMemoryWord(m_cpu.SP.W))
           << " prev " << HEXFORMAT0x4(m_prevPc)
           << " (9000)=" << HEXFORMAT0x4(ReadMemoryWord(0x9000))
           << endl;
    }

    if (pc == 0x1ce9) {
      m_keyWaitPending = true;
      if (!m_seenKeyWait) {
        m_seenKeyWait = true;
        cerr << "vtex: key_wait enter l1c41="
             << HEXFORMAT0x2(ReadMemory(0x1c41)) << endl;
      }
    }

    if (pc == 0x0005) {
      const uint8_t fn = m_cpu.BC.B.l;
      if (fn == 6) {
        m_bdos6Pending = true;
        m_bdos6Return = ReadMemoryWord(m_cpu.SP.W);
        m_bdos6Steps = 0;
        ++m_logBdos6;
      }
      else if (preKeyWait && !m_seenBackToCcp
               && fn >= 0x0f && fn <= 0x16) {
        // File BDOS only during init (open/close/read/write/make…).
        m_bdosPending = true;
        m_bdosFn = fn;
        m_bdosReturn = ReadMemoryWord(m_cpu.SP.W);
        cerr << "vtex: bdos fn " << HEXFORMAT0x2(fn)
             << " de " << HEXFORMAT0x4(m_cpu.DE.W) << endl;
      }
    }

    // Include cold_start (0182+) — a 256-cycle slice from 018b CALL
    // otherwise swallows all of probe_memory before PC is sampled.
    const bool stepHere =
        m_bdos6Pending ||
        m_bdosPending ||
        m_keyWaitPending ||
        (preKeyWait && !m_seenBackToCcp && pc < 0x8000) ||
        (pc >= 0x0182 && pc <= 0x0506) ||
        (pc >= 0x0900 && pc <= 0x0a9e) ||
        (pc >= 0x0d77 && pc <= 0x0db2) ||
        (pc >= 0x1415 && pc <= 0x15ff) ||
        (pc >= 0x1ca8 && pc <= 0x1d91) ||
        (pc >= 0x2147 && pc <= 0x219b) ||
        (pc == 0x0005) ||
        (pc == 0x0028);

    const int slice = stepHere ? 1 : std::min(rem, 256);
    const uint16_t pcBefore = pc;
    int left = Microbee128_BN_Emulator::Exec(slice);
    int did = slice - left;
    if (did < 1)
      did = 1;
    rem -= did;

    const uint16_t now = (uint16_t)GetPC();
    m_prevPc = pcBefore;

    if (m_keyWaitPending && now == 0x1cee) {
      const uint8_t a = m_cpu.AF.B.h;
      cerr << "key: got " << HEXFORMAT0x2(a);
      if (a >= 0x20 && a < 0x7f)
        cerr << " '" << (char)a << "'";
      cerr << endl;
      m_keyWaitPending = false;
    }

    if (m_bdosPending && now == m_bdosReturn) {
      cerr << "vtex: bdos fn " << HEXFORMAT0x2(m_bdosFn)
           << " -> a " << HEXFORMAT0x2(m_cpu.AF.B.h) << endl;
      m_bdosPending = false;
    }

    if (m_bdos6Pending) {
      m_bdos6Steps += did;
      if (now == m_bdos6Return) {
        const uint8_t a = m_cpu.AF.B.h;
        if (a != 0) {
          cerr << "bdos6: got " << HEXFORMAT0x2(a)
               << " steps " << m_bdos6Steps << endl;
        }
        else if (!m_loggedBdos6Poll) {
          m_loggedBdos6Poll = true;
          cerr << "bdos6: polling (a=0)" << endl;
        }
        m_bdos6Pending = false;
        m_bdos6Steps = 0;
      }
      else if (m_bdos6Steps == 50000) {
        cerr << "bdos6: hung"
             << " pc " << HEXFORMAT0x4(now)
             << " sp " << HEXFORMAT0x4(m_cpu.SP.W)
             << endl;
      }
    }
  }
  return rem;
}

void Microbee128_StarnetClient_Emulator::WakeReceive()
{
  if (m_transmit || m_rxPhase != RxPhase::Sync)
    return;
  if ((m_cpu.IFF & (IFF_HALT | IFF_1)) != (IFF_HALT | IFF_1))
    return;
  if (!m_pio.GetIE(0) || !m_client.HasTx())
    return;

  // EE50 reads this byte into B, decrements it, and INIRs the frame.
  m_bodyLeft = m_client.TakeFrameSize();
  if (m_bodyLeft == 0)
    return;
  m_rxPhase = RxPhase::Length;
  m_pio.SetData(0, (uint8_t)(m_bodyLeft + 1));
  m_pio.Strobe(0);
}

void Microbee128_StarnetClient_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  // vtex hw_init reprograms PIO control for a modem bit-bang ISR. That
  // breaks Starnet's PIO A/B setup and the next interrupt can RET into
  // CCP with the TPA stack still at 9000h.
  if (info.m_id == PORT_PIO && ((port & 1) == 1) && (uint16_t)GetPC() < 0x8000) {
    if (m_blockedPioCtrl++ == 0)
      cerr << "vtex: blocked PIO ctrl " << HEXFORMAT0x2(port & 3)
           << " data " << HEXFORMAT0x2(data)
           << " pc " << HEXFORMAT0x4(GetPC()) << endl;
    return;
  }

  Microbee128_BN_Emulator::WriteIOPort(info, port, data);

  // Viatel reprograms PIO B as mode-3 with DDR 1Fh (bits 0-4 in). Keep
  // bit 7 an output so the Starnet direction line cannot float.
  if (info.m_id == PORT_PIO && (port & 3) == 3 && (uint16_t)GetPC() < 0x8000)
    m_pio.ForceOutputBits(1, 0x80);
}

uint8_t Microbee128_StarnetClient_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  return Microbee128_BN_Emulator::ReadIOPort(info, port);
}

void Microbee128_StarnetClient_Emulator::OnPIOBWrite(uint8_t data, bool)
{
  // Bit 7 turns the external Starnet buffer around (1 = TX, 0 = RX).
  // Viatel RMW of modem bits on PIO B runs from TPA (< 0x8000) and must
  // not flip that direction line; ROM/LBIOS (>= 0x8000) own it.
  const uint16_t pc = (uint16_t)GetPC();

  if (pc < 0x8000) {
    const uint8_t fixed = (uint8_t)((data & 0x7f) | (m_transmit ? 0x80 : 0));
    if (fixed != data)
      m_pio.SetOutputLatch(1, fixed);
    return;
  }

  bool transmit = (data & 0x80) != 0;
  if (transmit != m_transmit) {
    ++m_starnetDirFlips;
    // First flip + occasional reminder; BIOS banks around every frame.
    if (m_starnetDirFlips <= 2 || (m_starnetDirFlips % 100) == 0) {
      cerr << "mbee: starnet dir " << (m_transmit ? 1 : 0)
           << " -> " << (transmit ? 1 : 0)
           << " pc " << HEXFORMAT0x4(pc)
           << " n=" << m_starnetDirFlips;
      if (!m_seenKeyWait)
        cerr << " (no key_wait yet)";
      cerr << endl;
    }
  }
  if (m_transmit && !transmit) {
    m_rxPhase = RxPhase::Sync;
    m_bodyLeft = 0;
  }
  m_transmit = transmit;
}

uint8_t Microbee128_StarnetClient_Emulator::OnPIOBRead()
{
  // Modem/status lines: bits 3 and 4 always read high, even if the CPU
  // programmed them as outputs and wrote 0 (SetData alone cannot override
  // output-latch bits).
  return m_pio.OrDataBits(1, 0x18);
}

void Microbee128_StarnetClient_Emulator::OnPIOAWrite(uint8_t data, bool)
{
  if (!m_transmit)
    return;

  m_client.OnReceive(data);
  if (m_pio.GetMode(0) != Z80PIO::Mode::Control)
    m_pio.Strobe(0);
}

uint8_t Microbee128_StarnetClient_Emulator::OnPIOARead()
{
  uint8_t byte = 0;
  if (!m_transmit) {
    if (m_rxPhase == RxPhase::Length) {
      byte = (uint8_t)(m_bodyLeft + 1);
      m_rxPhase = RxPhase::Body;
    }
    else if (m_rxPhase == RxPhase::Body && m_bodyLeft > 0) {
      byte = m_client.OnSend();
      if (--m_bodyLeft == 0)
        m_rxPhase = RxPhase::Sync;
    }
  }
  m_pio.SetData(0, byte);
  return byte;
}

INFO_START(microbee128starnet)
{
  INFO_CPU(MICROBEE_13_5_SPEED, MICROBEE_128k_DISK_ROM_START_ADDR),
  INFO_PARENT(microbee128bn)
}
INFO_END(microbee128starnet);

EmulatorInfo g_microbee128starnetEmulatorInfo =
{
  "mbee128starnet",                  // command line option
  "Microbee 128k Starnet Client",            // short name
  "Microbee 128k Starnet Client",            // long name
  true,                          // SDL window

  INFO_INSERT(microbee128starnet)
};

/////////////////////////////////////////////////////////////////////////////////////////////
