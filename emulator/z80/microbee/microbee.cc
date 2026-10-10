

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

#define PORT_PIO        1      // PIO
#define PORT_6545       2      // 6545
#define PORT_FONT       3      // enable font ROM
#define PORT_COLOUR     4      // colour control port
#define PORT_VIATEL     5      // wait off/Viatel

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

  protected:
    int m_mode = eOriginal;
    bool m_colourRAMEnabled = false;
    bool m_attributeRAMEnabled = false;
    int m_bankSel = 0;

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
  if (m_mode == eOriginal) {
    cout << "mbee: alpha video enabled" << endl;
    m_mode = eAlpha; 
  }
}

void MicrobeeVideo::SetAttributeEnable(bool v)
{
  if (v && (m_mode == eOriginal)) {
    cout << "microbee: original colour mode enabled (attr)" << endl;
    m_mode = eColour;
  }
  m_attributeRAMEnabled = v;
}

void MicrobeeVideo::SetColourEnable(bool v)
{
  if (v && (m_mode == eOriginal)) {
    cout << "microbee: original colour mode enabled (colour)" << endl;
    m_mode = eColour;
  }
  m_colourRAMEnabled = v;
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

  if (!m_fontROMEnabled)
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

  if (m_options.m_verbose)  
    cout << "mbee: setting screen to " << m_cols << "x" << m_rows << "x" << lines << endl;
    
  PixelFont * font = new PixelFont(
      (m_fontOffset == 0) ? g_charGen_mbee64x16 : g_charGen_mbee80x24,
      256, 
      nullptr);
  font->SetHeight(lines);
  SetFont(font, m_cols, m_rows);
  RefreshScreen();
}

void MicrobeeVideo::RewritePCG() 
{
 cerr << "pcg: rewriting PCG" << endl;
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

  return true;
}

void Microbee_Emulator::Reset(int addr)
{
  m_video = (MicrobeeVideo *)m_screen.get();

  Z80Emulator::Reset(addr);
  m_pio.Reset();
  m_crtc.Reset();
  //m_crtc.SetStatus(Synertek6545::eVBlanking); // always say we are in blanking
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

  if (ScanKeyboard(addr)) {
    //cout << "microbee: new keyboard code " << hex << addr << " " << GetRowCol(addr) << endl;
    return true;
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
  cout << "mbee: video start address set to " << HEXFORMAT0x4(memOffset) << endl;

  if (fontOffset != m_fontOffset) {
    m_fontOffset = fontOffset;
    cout << "mbee: video font offset set to " << HEXFORMAT0x4(m_fontOffset) << endl;
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
    //case PORT_FONT:
    //  return 0x00;
    //case PORT_COLOUR:
    //  return 0x00;
  }
  cerr << "microbee: read port " << HEXFORMAT0x2(port) << endl;
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
    case PORT_VIATEL:
      return;
    case PORT_LVDAT:
      cerr << "mbee: LVDAT " << HEXFORMAT0x2(data) << endl;
      m_video->SetPCGBank        (data & 0x0f);
      m_video->SetAttributeEnable(data & 0x10);
      m_video->SetAlphaEnable    (data & 0x80);
      return;
  }
  cerr << "microbee: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
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
  INFO_IO_PORT_RW(0x09, 0x09, PORT_VIATEL),   // wait off/Viatel
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
  INFO_CPU(4, MICROBEE_BASIC_ROM_START_ADDR),
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
  INFO_CPU(4, MICROBEE_56k_DISK_ROM_START_ADDR),
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
  m_bankSel          = data;
  m_currentLowerBank = m_bankedMemory[data & 3]; //g_bankMap[data & 3]];
  m_romDisable       = data & 0x04;
  m_videoDisable     = data & 0x08;
  m_videoLower       = data & 0x10;
  /*
  cerr << "mbee128: bank sel " << HEXFORMAT0x2(data) 
       << ", bank " << (data & 3)
       << ", ROM " << (m_romDisable ? "disabled" : "enabled")
       << ", video " << (m_videoDisable ? "disabled" : "enabled")
       << ", video addr " << (m_videoLower ? "0x8000" : "0xf000")
       << endl;
  */     
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
        if (m_videoLower && (addr <= 0x8fff)) {
          m_video->WriteMemoryAtAddress(addr & 0xfff, val);
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
        if (m_videoLower && (addr <= 0x8fff)) {
          return m_video->ReadMemoryAtAddress(addr & 0x0fff);
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
  INFO_CPU(4, MICROBEE_128k_DISK_ROM_START_ADDR),
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
  INFO_CPU(4, MICROBEE_128k_DISK_ROM_START_ADDR),
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
, m_server("", "")
, m_client(m_server)
{
  using namespace std::placeholders;
  m_pio.SetWriteHandler(0, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOAWrite, this, _1, _2));
  m_pio.SetReadHandler (0, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOARead,  this));
  m_pio.SetWriteHandler(1, std::bind(&Microbee128_StarnetClient_Emulator::OnPIOBWrite, this, _1, _2));
}

bool Microbee128_StarnetClient_Emulator::Open(const Options & options)
{
  if (!Microbee128_BN_Emulator::Open(options))
    return false;

  m_fdc.reset();  

  return true;
}

void Microbee128_StarnetClient_Emulator::Reset(int addr)
{
  Microbee128_BN_Emulator::Reset(addr);
  m_transmit = false;
  m_rxPhase = RxPhase::Sync;
  m_bodyLeft = 0;
  m_client.Reset();
  m_server.Reset();
}

int Microbee128_StarnetClient_Emulator::Exec(int cycles)
{
  // A strobe raised during the IN that follows EI is taken as vector 0x4C,
  // before the client has armed the vector it uses for the block read.
  WakeReceive();
  return Microbee128_BN_Emulator::Exec(cycles);
}

void Microbee128_StarnetClient_Emulator::WakeReceive()
{
  if (m_transmit || m_rxPhase != RxPhase::Sync)
    return;
  if ((m_cpu.IFF & (IFF_HALT | IFF_1)) != (IFF_HALT | IFF_1))
    return;
  if (!m_pio.GetIE(0) || !m_client.HasTx())
    return;

  // The halted client reads one byte, decrements it, and block-reads the rest.
  m_bodyLeft = m_client.TakeFrameSize();
  if (m_bodyLeft == 0)
    return;
  m_rxPhase = RxPhase::Length;
  m_pio.SetData(0, (uint8_t)(m_bodyLeft + 1));
  m_pio.Strobe(0);
}

void Microbee128_StarnetClient_Emulator::OnPIOBWrite(uint8_t data, bool)
{
  // Bit 7 turns the external buffer around. 1 is transmit, 0 is receive.
  bool transmit = (data & 0x80) != 0;
  if (m_transmit && !transmit) {
    m_rxPhase = RxPhase::Sync;
    m_bodyLeft = 0;
  }
  m_transmit = transmit;
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
  INFO_CPU(4, MICROBEE_128k_DISK_ROM_START_ADDR),
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
