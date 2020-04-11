

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
#define   MICROBEE_VIDEO_END_ADDR     0xf7ff

#define   MICROBEE_SCREEN_COLS        64
#define   MICROBEE_SCREEN_ROWS        16

#define   MICROBEE_FONT_WIDTH         8
#define   MICROBEE_FONT_HEIGHT        16

#define   MICROBEE_VIRTUAL_FONT_CHARS    256

#define   MICROBEE_PCG_START_ADDR     0xf800
#define   MICROBEE_PCG_END_ADDR       0xffff

#define   MICROBEE_PCG_SIZE           (MICROBEE_PCG_END_ADDR - MICROBEE_PCG_START_ADDR + 1)


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

class MicrobeeVideo : public SingleColourMemoryMappedScreen
{
  public:
    MicrobeeVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    static bool CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

    FontChar GetCharAtLoc(int addr) const override;

    virtual void RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

    virtual void WriteMemoryAtAddress(int addr, uint8_t ch)  override;
    virtual uint8_t ReadMemoryAtAddress(int addr) const override;

    virtual void SetScreenSize(int cols, int rows, int lines);

    virtual void SetCharData(FontChar ch, int line, uint8_t val);

    void EnableFontROM(bool enable)
    { m_fontROMEnabled = enable; }

    void SetFontOffset(uint16_t offset)
    { m_fontOffset = offset; }

    uint16_t GetFontOffset() const
    { return m_fontOffset; }

  protected:
    int m_rows = 0;
    int m_cols = 0;
    int m_lines = 0;
    bool m_fontROMEnabled = false;
    uint16_t m_fontOffset = 0;
};


MicrobeeVideo::MicrobeeVideo(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : SingleColourMemoryMappedScreen(mainWindow, options, info)
{
  memset(&m_memory[0],             0x20, m_visibleSize);
  memset(&m_memory[m_visibleSize], 0x00, m_memory.size() - m_visibleSize);
  InitUsage();
}

FontChar MicrobeeVideo::GetCharAtLoc(int loc) const
{
  FontChar ch = m_memory[loc & 0x7ff];

  if (ch >= 0x80) {
    //ch = ch & 0x7f;
  }

  //uint8_t attr = m_memory[0x400 + charAddr];
  // switch to graphics
  //ch += ((attr & 0x2) != 0) ? 0x100 : 0x000;

  return ch;
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

void MicrobeeVideo::WriteMemoryAtAddress(int addr, uint8_t ch)
{
  if (!m_fontROMEnabled)
    SingleColourMemoryMappedScreen::WriteMemoryAtAddress(addr, ch);
}

uint8_t MicrobeeVideo::ReadMemoryAtAddress(int addr) const
{
  if (!m_fontROMEnabled)
    return SingleColourMemoryMappedScreen::ReadMemoryAtAddress(addr);
  else
    return g_charGen_MotorolaMCM6574.m_data[(addr & 0x7ff) + m_fontOffset];
}

static bool CreateFontWithPCG(const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  cerr << "Setting default invert font" << endl;

  // create inverted chars
  {
    uint8_t * src = &fontData[0];
    uint8_t * dst = &fontData[128 * fontInfo.m_height];
    for (int i = 0; i < 128*fontInfo.m_height; ++i)
      *dst++ = *src++ ^ 0xff;
  }
  return true;
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
      &CreateFontWithPCG);
  font->SetHeight(lines);

  SetFont(font, m_cols, m_rows);
  RefreshScreen();
}

void MicrobeeVideo::SetCharData(FontChar ch, int line, uint8_t val)
{
  size_t count = m_usage.count(ch);
  if (count > 0) {
    m_font->Modify(ch, line, val);
    auto & s = m_usage[ch];
    for (auto & r : s.m_locs) 
      RefreshCharAtLoc(r);
    Update(true);  
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

  m_pcgRAM.resize(MICROBEE_PCG_SIZE);

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
    uint8_t out = m_keyboard->Read(1 << row);
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
}

void Microbee_Emulator::OnSetVideoStartAddress(uint16_t addr)
{
  uint16_t memOffset  = addr & 0x7ff;
  uint16_t fontOffset = ((addr & (1 << 13)) != 0) ? 0x800 : 0x000;
  m_video->SetOffset(memOffset);
  cout << "mbee: video start address set to " << HEXFORMAT0x4(memOffset) << endl;

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
      m_video->EnableFontROM((data & 1) != 0);
      return;
    case 4:
    case 5:
      return;
  }
  cerr << "microbee: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

void Microbee_Emulator::WriteIOMemory(int id, uint16_t addr, uint8_t val)
{
  if (id == 8) {
    uint16_t offs = addr & (MICROBEE_PCG_SIZE-1);
    m_pcgRAM[offs] = val;

    int pcgChar = 128 + (offs >> 4);

    m_video->SetCharData(pcgChar, addr & 0xf, val);
    
    // need to write this to the screen somehow
  }
}

uint8_t Microbee_Emulator::ReadIOMemory(int id, uint16_t addr) const
{
  if (id == 8) {
    uint16_t offs = addr & (MICROBEE_PCG_SIZE-1);
    return m_pcgRAM[offs];
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_microbee32EmulatorInfo;

Microbee32_Emulator::Microbee32_Emulator()
  : Microbee_Emulator(&g_microbee32EmulatorInfo)
{}

/////////////////////////////////////////////////////////////////////////////////////////////

extern EmulatorInfo g_microbee56EmulatorInfo;

Microbee56_Emulator::Microbee56_Emulator()
  : Microbee_Emulator(&g_microbee56EmulatorInfo)
{
}

bool Microbee56_Emulator::MountDrive(int driveNum, VirtualDrive *drive, bool readOnly)
{
  return m_fdc->MountDrive(driveNum, drive, readOnly);
}

bool Microbee56_Emulator::Open(const Options & options)
{
  if (!Microbee_Emulator::Open(options))
    return false;

  m_fdc.reset(new WD_FD1793(options.m_fdcDebug));

  return true;
}

uint8_t Microbee56_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 0x10:
      {
        uint8_t v = m_fdc->Read(port & 0x3);
        //if ((port & 0x03) == 0)
        //  cerr << "mbee: fdc port 0x44 = " << HEXFORMAT0x2(v) << endl;
        return v;
      }
    case 0x11:
      {
        uint8_t val = (m_fdc->GetDRQ() || m_fdc->GetInterrupt()) ? 0x80 : 0x00;
        //m_fdcPending = false;
        //cerr << "mbee: port 0x48 return " << HEXFORMAT0x2(val) << endl;
        return val;
      }
  }
  return Microbee_Emulator::ReadIOPort(info, port);
}

void Microbee56_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 0x10:
      return m_fdc->Write(port & 0x3, data);
    case 0x11:
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

INFO_START(microbee)
{
  INFO_IO_PORT_RW(0x00, 0x03, 1),      // PIO
  INFO_IO_PORT_RW(0x08, 0x08, 4),      // colour control port
  INFO_IO_PORT_RW(0x09, 0x09, 5),      // wait off/Viatel
  INFO_IO_PORT_RW(0x0b, 0x0b, 3),      // enable font ROM
  INFO_IO_PORT_RW(0x0c, 0x0d, 2),      // 6545

  INFO_SCREEN_MEMORY_MAPPED_FIXED("microbee", \
                          MICROBEE_VIDEO_START_ADDR, MICROBEE_VIDEO_END_ADDR, \
                          MICROBEE_SCREEN_COLS, MICROBEE_SCREEN_ROWS, \
                          MICROBEE_FONT_WIDTH, MICROBEE_FONT_HEIGHT, \
                          MICROBEE_VIRTUAL_FONT_CHARS, \
                          &g_charGen_mbee64x16, \
                          CreateFontWithPCG),

  INFO_MONITOR(12.0, 4.0, 3.0, ePAL),

  INFO_MEM_IO_READ(MICROBEE_PCG_START_ADDR, MICROBEE_PCG_END_ADDR, 8),
  INFO_MEM_IO_WRITE(MICROBEE_PCG_START_ADDR, MICROBEE_PCG_END_ADDR, 8)
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

#define   MICROBEE_DISK_ROM_START_ADDR     0xe000
#define   MICROBEE_DISK_ROM_END_ADDR       0xefff

#define   MICROBEE_DISK_RAM_START_ADDR     0x0000
#define   MICROBEE_DISK_RAM_END_ADDR       0xdfff

extern unsigned char g_disk56_ROM[4096];

INFO_START(microbee56)
{
  INFO_CPU(4, MICROBEE_DISK_ROM_START_ADDR),
  INFO_PARENT(microbee),
  INFO_ROM(MICROBEE_DISK_ROM_START_ADDR, g_disk56_ROM),
  INFO_MAIN_RAM(MICROBEE_DISK_RAM_START_ADDR, 56, 56, MICROBEE_DISK_ROM_START_ADDR / 1024),

  INFO_IO_PORT_RW(0x44, 0x47, 0x10),      // FDC
  INFO_IO_PORT_RW(0x48, 0x48, 0x11)      // drive select
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
