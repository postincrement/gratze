

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
#define   MICROBEE_VIDEO_END_ADDR     0xffff

#define   MICROBEE_SCREEN_COLS        64
#define   MICROBEE_SCREEN_ROWS        16

#define   MICROBEE_FONT_WIDTH         8
#define   MICROBEE_FONT_HEIGHT        16

#define   MICROBEE_VIRTUAL_FONT_CHARS    256

#define   MICROBEE_PCG_START_OFFS     0x0800
#define   MICROBEE_PCG_END_OFFS       0x0fff

#define   MICROBEE_PCG_SIZE           (MICROBEE_PCG_END_OFFS - MICROBEE_PCG_START_OFFS + 1)
#define   MICROBEE_ATTRIBUTE_SIZE     0x800

#define PORT_PIO        1      // PIO
#define PORT_6545       2      // 6545
#define PORT_FONT       3      // enable font ROM
#define PORT_COLOUR     4      // colour control port
#define PORT_VIATEL     5      // wait off/Viatel

#define   PORT_FDC      0x10   // floppy controller
#define   PORT_DRVSEL   0x11   // drive select

#define   PORT_LVDAT    0x12   // LV DAT
#define   PORT_BANK     0x13   // bank select

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
      //cerr << "mbee: font ROM " << (enable ? "enabled" : "disabled") << endl;
    }

    void SetFontOffset(uint16_t offset)
    { m_fontOffset = offset; }

    uint16_t GetFontOffset() const
    { return m_fontOffset; }

    virtual bool IsPCG(FontChar ch) const
    { return (ch >= 0x80) && (ch < 0x100); }

    void SetUpdateHandler(std::function<void ()> handler)
    { m_updateHandler = handler; }

    virtual void RewritePCG() override;

    void GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg) override;

    bool m_attributeRAMEnabled = false;
    bool m_extendedPCG = false;
    bool m_colourRAMEnabled = false;
    int m_bankSel = 0;
    std::vector<std::vector<uint8_t>> m_attributeRAM;
    std::vector<std::vector<uint8_t>> m_pcgRAM;
    std::vector<std::vector<uint8_t>> m_colourRAM;

  protected:
    int m_rows = 0;
    int m_cols = 0;
    int m_lines = 0;
    bool m_fontROMEnabled = false;
    uint16_t m_fontOffset = 0;
    std::function<void ()> m_updateHandler = nullptr;
};


MicrobeeVideo::MicrobeeVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : ColourMemoryMappedScreen(mainWindow, options, info)
{
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);

  m_pcgRAM.resize(16);
  m_attributeRAM.resize(16);
  m_colourRAM.resize(16);
  for (int i = 0; i < 16; ++i) {
    m_pcgRAM[i].resize(MICROBEE_PCG_SIZE);
    m_attributeRAM[i].resize(MICROBEE_ATTRIBUTE_SIZE);
    m_colourRAM[i].resize(MICROBEE_ATTRIBUTE_SIZE);
  }

  m_attributeRAMEnabled = false;
  m_colourRAMEnabled = false;
  m_extendedPCG = false;
  m_bankSel = 0;

  InitPCG();
}

FontChar MicrobeeVideo::GetCharAtLoc(int loc) const
{
  FontChar ch = m_memory[loc & 0x7ff];
  return ch;
}

static SDL_Colour g_colours[16] = 
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
  { 0xff, 0xff, 0xff, 0xff },  // white

};

void MicrobeeVideo::GetColourAtLoc(int loc, SDL_Colour & fg, SDL_Colour & bg)
{
  uint8_t attr = m_colourRAM[0][loc & 0x7ff];

  fg = g_colours[attr & 0xf];
  bg = g_colours[(attr >> 4) & 0xf];
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
  uint16_t offs = addr & 0x0fff;
  uint16_t voffs = addr & 0x07ff;
  if (offs >= 0x800) {
    if (m_colourRAMEnabled) {
      m_colourRAM[0][voffs] = val;
      RefreshCharAtLoc(voffs);
    }
    else {
      m_pcgRAM[m_bankSel][voffs] = val;
      int ch  = 128 + (voffs >> 4);
      int row = addr & 0xf;
      SetPCG(ch, row, val);
    }
  }
  else if (m_attributeRAMEnabled) {
    m_attributeRAM[m_bankSel][voffs] = val;
  }
  else if (!m_fontROMEnabled)
    ColourMemoryMappedScreen::WriteMemoryAtAddress(voffs, val);
}

uint8_t MicrobeeVideo::ReadMemoryAtAddress(int addr) const
{
  //cout << "mbeevideo: read " << HEXFORMAT0x4(addr) << endl;
  uint16_t offs  = addr & 0x0fff;
  uint16_t voffs = addr & 0x07ff;
  if (offs >= 0x800) {
    if (m_colourRAMEnabled) {
      return m_colourRAM[0][voffs];
    }
    else {
      return m_pcgRAM[m_bankSel][voffs];
    }
  }
  else if (m_fontROMEnabled) {
    const CharacterGeneratorROM * chargen = (m_fontOffset == 0) ? &g_charGen_mbee64x16 : &g_charGen_mbee80x24;
    return chargen->m_data[voffs];
  }
  else if (m_attributeRAMEnabled) {
    return m_attributeRAM[m_bankSel][voffs];
  }  
  else  
    return ColourMemoryMappedScreen::ReadMemoryAtAddress(voffs);
}

void MicrobeeVideo::SetScreenSize(int cols, int rows, int lines)
{
  m_cols  = cols;
  m_rows  = rows;
  m_lines = lines;

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

  KeyboardScanner * kb = new KeyboardScanner();
  SetKeyboard(kb);
  kb->Compile(g_microbeeKeys);

  using namespace std::placeholders;
  m_pio.SetInterruptHandler(std::bind(&Microbee_Emulator::OnPIOInterrupt, this, _1));

  m_crtc.SetScreenShapeHandler(std::bind(&Microbee_Emulator::OnSetScreenSize, this, _1, _2, _3));
  m_crtc.SetStartAddressHandler(std::bind(&Microbee_Emulator::OnSetVideoStartAddress, this, _1));
  m_crtc.SetCursorAddressHandler(std::bind(&Microbee_Emulator::OnSetCursorAddress, this, _1));
  m_crtc.SetCursorShapeHandler(std::bind(&Microbee_Emulator::OnSetCursorShape, this, _1, _2, _3));
  m_crtc.SetUpdateHandler(std::bind(&Microbee_Emulator::OnKeyboardScan, this, _1, _2));

  //using namespace std::placeholders;
  //m_keyboard.SetHandler(true, std::bind(&Z80PIO::SetData, &m_pio, 0, _1));

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
  while ((pos < 8) && ~(val & 1)) {
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
      addr = (row << 7) + (FindBitSet(out) << 4);
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
    //cout << "microbee: new keyboard code " << GetRowCol(addr) << endl;
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
    case PORT_FONT:
      return 0x00;
    case PORT_COLOUR:
      return 0x00;
  }
  cerr << "microbee: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void Microbee_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_PIO:
      return m_pio.Write(port & 0x3, data);
    case PORT_6545:
      return m_crtc.Write(port & 1, data);
    case PORT_FONT:
      m_video->EnableFontROM((data & 1) != 0);
      return;
    case PORT_COLOUR:
      m_video->m_colourRAMEnabled = (data & 0x40);
      return;
    case PORT_VIATEL:
      return;
  }
  cerr << "microbee: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

void Microbee_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  #if 0
  if (id == 8) {
    uint16_t offs = addr & (MICROBEE_PCG_SIZE-1);
    //if (m_pcgRAM[offs] != val) {
      m_video->m_pcgRAM[offs] = val;
      int ch  = 128 + (offs >> 4);
      int row = addr & 0xf;
      m_video->SetPCG(ch, row, val);
    //}
  }
  #endif
}

uint8_t Microbee_Emulator::ReadIOMemory(int id, uint16_t addr) const
{
  #if 0
  if (id == 8) {
    uint16_t offs = addr & (MICROBEE_PCG_SIZE-1);
    return m_video->m_pcgRAM[offs];
  }
  #endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

MicrobeeDisk_Emulator::MicrobeeDisk_Emulator(const EmulatorInfo * info)
  : Microbee_Emulator(info)
{
}

bool MicrobeeDisk_Emulator::MountDrive(int driveNum, VirtualDrive *drive, bool readOnly)
{
  return m_fdc->MountDrive(driveNum, drive, readOnly);
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
      {
        uint8_t v = m_fdc->Read(port & 0x3);
        //if ((port & 0x03) == 0)
        //  cerr << "mbee: fdc port 0x44 = " << HEXFORMAT0x2(v) << endl;
        return v;
      }
    case PORT_DRVSEL:
      {
        uint8_t val = (m_fdc->GetDRQ() || m_fdc->GetInterrupt()) ? 0x80 : 0x00;
        //m_fdcPending = false;
        //cerr << "mbee: port 0x48 return " << HEXFORMAT0x2(val) << endl;
        return val;
      }
  }
  return Microbee_Emulator::ReadIOPort(info, port);
}

void MicrobeeDisk_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_FDC:
      return m_fdc->Write(port & 0x3, data);
    case PORT_DRVSEL:
    {
      m_drive   = data & 0x03;
      m_fdc->SelectDrive(m_drive);
      m_side    = (data & 0x04) != 0;
      m_fdc->SelectSide(m_side);
      m_density = (data & 0x08) != 0;
    }
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
                          MICROBEE_VIDEO_START_ADDR, MICROBEE_VIDEO_END_ADDR, \
                          MICROBEE_SCREEN_COLS, MICROBEE_SCREEN_ROWS, \
                          MICROBEE_FONT_WIDTH, MICROBEE_FONT_HEIGHT, \
                          MICROBEE_VIRTUAL_FONT_CHARS, \
                          &g_charGen_mbee64x16, nullptr)

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

  INFO_INSERT(microbee56)
};

/////////////////////////////////////////////////////////////////////////////////////////////

#define MEMORY_LOWER_BANK       0x01
#define MEMORY_UPPER_BANK       0x02

extern unsigned char g_disk128_ROM[8192];

extern EmulatorInfo g_microbee128EmulatorInfo;

Microbee128_Emulator::Microbee128_Emulator()
  : MicrobeeDisk_Emulator(&g_microbee128EmulatorInfo)
{
}

bool Microbee128_Emulator::Open(const Options & options)
{
  if (!MicrobeeDisk_Emulator::Open(options))
    return false;

  return true;
}

void Microbee128_Emulator::Reset(int addr)
{
  MicrobeeDisk_Emulator::Reset(addr);
  SetBankSel(0x00);
}

void Microbee128_Emulator::SetBankSel(uint8_t data)
{
  m_bankSel          = data;
  m_currentLowerBank = m_bankedMemory[data & 3];
  m_romDisable       = data & 0x04;
  m_videoDisable     = data & 0x08;
  m_videoLower       = data & 0x10;;
  /*
  cerr << "mbee128: bank sel " << HEXFORMAT0x2(data) 
       << ", bank " << (data & 3)
       << ", ROM " << (m_romDisable ? "disabled" : "enabled")
       << ", video " << (m_videoDisable ? "disabled" : "enabled")
       << ", video addr " << (m_videoLower ? "0x8000" : "0xf000")
       << endl;
  */     
}

uint8_t Microbee128_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
//  switch (info.m_id) {
//    case PORT_BANK:
//      return m_bankSel;
//    case PORT_LVDAT:
//  }
  return MicrobeeDisk_Emulator::ReadIOPort(info, port);
}

void Microbee128_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case PORT_LVDAT:
      cerr << "mbee128: LVDAT " << HEXFORMAT0x2(data) << endl;
      m_video->m_bankSel             = data & 0x0f;
      m_video->m_attributeRAMEnabled = data & 0x10;
      m_video->m_extendedPCG         = data & 0x80;
      return;
    case PORT_BANK:
      SetBankSel(data);
      return;
  }
  return MicrobeeDisk_Emulator::WriteIOPort(info, port, data);
}

void Microbee128_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
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
      m_bankedMemory[0][addr & 0x7fff] = val;
      return;
  }
  return MicrobeeDisk_Emulator::WriteIOMemory(id, addr, val);
}

uint8_t Microbee128_Emulator::ReadIOMemory(int id, uint16_t addr) const
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
        return g_disk128_ROM[addr & 0x1fff];
      }
      return m_bankedMemory[0][addr & 0x7fff];
  }
  
  return MicrobeeDisk_Emulator::ReadIOMemory(id, addr);
}

#define   MICROBEE_128k_DISK_ROM_START_ADDR     0x8000

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

EmulatorInfo g_microbee128EmulatorInfo =
{
  "mbee128",                  // command line option
  "Microbee 128k",            // short name
  "Microbee 128k",            // long name

  INFO_INSERT(microbee128)
};

/////////////////////////////////////////////////////////////////////////////////////////////
