#include "trs80.h"

#include <functional>
#include <iostream>
#include <iomanip>

extern "C"
{
#include "trs_chars.c"
};

using namespace std;

#define KB_MEM_ADDR 0x3800
#define VIDEO_MEM_ADDR 0x3c00
#define RAM_MEM_ADDR 0x4000

#define RTC_INTERVAL_MS 40

#define RESET_SYM   SDLK_F1
#define TRACE_SYM   SDLK_F2
#define REBOOT_SYM  SDLK_F12

#define EXTENDED_SYM_START 0x4000004f

#define FONT_HEIGHT   12    // must be divisble by 3
#define FONT_WIDTH    6     // must be divisible by 2

#define FONT_NUMBER   0  


/////////////////////////////////////////////////////////////

TRS80Emulator::TRS80Emulator()
  : Emulator("TRS-80 Model 1")
{
}

bool TRS80Emulator::Open(const Options &options)
{
  m_fdc.SetInterruptHandler(std::bind(&TRS80Emulator::FDCInterrupt, this));

  // load ROM
  if (!ReadROMFromFile(options.m_romFn, m_rom))
    return false;

  m_rtcTimer = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
  m_rtcPending = false;
  m_fdcPending = false;

  m_cassetteMotor    = false;
  m_cassetteTrigger  = false;
  m_cassette2        = false;

  return Emulator::Open(options);
}

bool TRS80Emulator::Start(uint16_t addr)
{
  // create font with graphics chars
  m_fontData.resize(FONT_HEIGHT * 256);
  memcpy(&m_fontData[0], &trs_char_data[FONT_NUMBER][0][0], 128 * FONT_HEIGHT);

  uint8_t maskRight = (1 << (FONT_WIDTH / 2)) - 1;
  uint8_t maskLeft  = maskRight << (FONT_WIDTH / 2);

  for (uint8_t i = 0; i < 64; ++i) {
    uint8_t * dst = &m_fontData[(128 + i) * FONT_HEIGHT];
    uint8_t val = i;
    for (int y = 0; y < 3; ++y) {
      *dst = 0;
      if (val & 1)
        *dst |= maskRight;
      if (val & 2)
        *dst |= maskLeft;
      for (int z = 1; z < FONT_HEIGHT / 3; ++z)
        dst[z] = dst[0];  
      dst += FONT_HEIGHT / 3;
      val = val >> 2;  
    }
  }
  memcpy(&m_fontData[(128 + 64) * FONT_HEIGHT], &m_fontData[128 * FONT_HEIGHT], 64 * FONT_HEIGHT);

  m_font.reset(new MemoryMappedVideo::Font(FONT_WIDTH, FONT_HEIGHT, &m_fontData[0]));
  if (!OpenVideo(16, 64, m_font.get())) {
    return -1;
  }

  memset(m_kbData, 0x00, sizeof(m_kbData));
  m_shiftDown = 0;

  InitFDC();

  return Emulator::Start(addr);
}

void TRS80Emulator::Poll()
{
  auto now = std::chrono::system_clock::now();
  if (now > m_rtcTimer) {
    if (!m_rtcPending) {
      //cerr << "RTC INTERRUPT" << endl;
      m_rtcTimer = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
      //m_rtcPending = true;
      //Interrupt();
    }
  }
}

/////////////////////////////////////////////////////////////

#define KB_SYM_COUNT 0x85

// 0x80 = unshift to get code
// 0x40 = shift to get code

static uint8_t g_symToCode[KB_SYM_COUNT][2][2] = {

    {{0x00, 0x00}, {0x00, 0x00}}, // 0x00
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x01
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x02
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x03
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x04
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x05
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x06
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x07

    {{0x06, 0x20}, {0x06, 0x20}}, // 0x08 - BS
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x09 - TAB
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x0a
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x0b
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x0c
    {{0x06, 0x01}, {0x00, 0x00}}, // 0x0d - Enter
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x0e
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x0f

    {{0x00, 0x00}, {0x00, 0x00}}, // 0x10
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x11
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x12
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x13
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x14
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x15
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x16
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x17

    {{0x00, 0x00}, {0x00, 0x00}}, // 0x18
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x19
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x1a
    {{0x06, 0x04}, {0x06, 0x04}}, // 0x1b  ESC = break
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x1c
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x1d
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x1e
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x1f

    {{0x06, 0x80}, {0x60, 0x80}}, // 0x20 - space
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x21
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x22
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x23
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x24
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x25
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x26
    {{0x00, 0x00}, {0x04, 0x04}}, // 0x27    ()  "

    {{0x00, 0x00}, {0x00, 0x00}}, // 0x28
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x29
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x2a
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x2b
    {{0x05, 0x10}, {0x50, 0x10}}, // 0x2c    , <
    {{0x05, 0x20}, {0x00, 0x00}}, // 0x2d    -
    {{0x05, 0x40}, {0x40, 0x40}}, // 0x2e    . >
    {{0x05, 0x80}, {0x05, 0x80}}, // 0x2f    / ?

    {{0x04, 0x01}, {0x05, 0x02}}, // 0x30   0  )
    {{0x04, 0x02}, {0x04, 0x02}}, // 0x31   1  !
    {{0x04, 0x04}, {0x80, 0x01}}, // 0x32   2  @
    {{0x04, 0x08}, {0x04, 0x08}}, // 0x33   3  #
    {{0x04, 0x10}, {0x04, 0x10}}, // 0x34   4  $
    {{0x04, 0x20}, {0x04, 0x20}}, // 0x35   5  %
    {{0x04, 0x40}, {0x00, 0x00}}, // 0x36   6
    {{0x04, 0x80}, {0x04, 0x40}}, // 0x37   7  &

    {{0x05, 0x01}, {0x05, 0x04}}, // 0x38   8  *
    {{0x05, 0x02}, {0x05, 0x01}}, // 0x39   9  ()
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x3a
    {{0x05, 0x08}, {0x85, 0x04}}, // 0x3b   ;  :
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x3c
    {{0x45, 0x20}, {0x45, 0x08}}, // 0x3d   =  +
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x3e
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x3f

    {{0x00, 0x00}, {0x00, 0x00}}, // 0x40   @
    {{0x00, 0x02}, {0x00, 0x00}}, // 0x41   a
    {{0x00, 0x04}, {0x00, 0x00}}, // 0x42   b
    {{0x00, 0x08}, {0x00, 0x00}}, // 0x43   c
    {{0x00, 0x10}, {0x00, 0x00}}, // 0x44   d
    {{0x00, 0x20}, {0x00, 0x00}}, // 0x45   e
    {{0x00, 0x40}, {0x00, 0x00}}, // 0x46   f
    {{0x00, 0x80}, {0x00, 0x00}}, // 0x47   g

    {{0x01, 0x01}, {0x00, 0x00}}, // 0x48   h
    {{0x01, 0x02}, {0x00, 0x00}}, // 0x49   i
    {{0x01, 0x04}, {0x00, 0x00}}, // 0x4a   j
    {{0x01, 0x08}, {0x00, 0x00}}, // 0x4b   k
    {{0x01, 0x10}, {0x00, 0x00}}, // 0x4c   l
    {{0x01, 0x20}, {0x00, 0x00}}, // 0x4d   m
    {{0x01, 0x40}, {0x00, 0x00}}, // 0x4e   n
    {{0x01, 0x80}, {0x00, 0x00}}, // 0x4f   o

    {{0x02, 0x01}, {0x00, 0x00}}, // 0x50   p
    {{0x02, 0x02}, {0x00, 0x00}}, // 0x51   q
    {{0x02, 0x04}, {0x00, 0x00}}, // 0x52   r
    {{0x02, 0x08}, {0x00, 0x00}}, // 0x53   s
    {{0x02, 0x10}, {0x00, 0x00}}, // 0x54   t
    {{0x02, 0x20}, {0x00, 0x00}}, // 0x55   u
    {{0x02, 0x40}, {0x00, 0x00}}, // 0x56   v
    {{0x02, 0x80}, {0x00, 0x00}}, // 0x57   w

    {{0x03, 0x01}, {0x30, 0x01}}, // 0x58   x
    {{0x03, 0x02}, {0x30, 0x02}}, // 0x59   y
    {{0x03, 0x04}, {0x30, 0x04}}, // 0x5a   z
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x5b
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x5c
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x5d
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x5e
    {{0x00, 0x00}, {0x00, 0x00}}, // 0x5f

    {{0x06, 0x02}, {0x06, 0x02}}, // 0x60   ' = clear
    {{0x00, 0x02}, {0x00, 0x00}}, // 0x61   a
    {{0x00, 0x04}, {0x00, 0x00}}, // 0x62   b
    {{0x00, 0x08}, {0x00, 0x00}}, // 0x63   c
    {{0x00, 0x10}, {0x00, 0x00}}, // 0x64   d
    {{0x00, 0x20}, {0x00, 0x00}}, // 0x65   e
    {{0x00, 0x40}, {0x00, 0x00}}, // 0x66   f
    {{0x00, 0x80}, {0x00, 0x00}}, // 0x67   g

    {{0x01, 0x01}, {0x00, 0x00}}, // 0x68   h
    {{0x01, 0x02}, {0x00, 0x00}}, // 0x69   i
    {{0x01, 0x04}, {0x00, 0x00}}, // 0x6a   j
    {{0x01, 0x08}, {0x00, 0x00}}, // 0x6b   k
    {{0x01, 0x10}, {0x00, 0x00}}, // 0x6c   l
    {{0x01, 0x20}, {0x00, 0x00}}, // 0x6d   m
    {{0x01, 0x40}, {0x00, 0x00}}, // 0x6e   n
    {{0x01, 0x80}, {0x00, 0x00}}, // 0x6f   o

    {{0x02, 0x01}, {0x00, 0x00}}, // 0x70   p
    {{0x02, 0x02}, {0x00, 0x00}}, // 0x71   q
    {{0x02, 0x04}, {0x00, 0x00}}, // 0x72   r
    {{0x02, 0x08}, {0x00, 0x00}}, // 0x73   s
    {{0x02, 0x10}, {0x00, 0x00}}, // 0x74   t
    {{0x02, 0x20}, {0x00, 0x00}}, // 0x75   u
    {{0x02, 0x40}, {0x00, 0x00}}, // 0x76   v
    {{0x02, 0x80}, {0x00, 0x00}}, // 0x77   w

    {{0x03, 0x01}, {0x30, 0x01}}, // 0x78   x
    {{0x03, 0x02}, {0x30, 0x02}}, // 0x79   y
    {{0x03, 0x04}, {0x30, 0x04}}, // 0x7a   z
    {{0x04, 0x00}, {0x00, 0x00}}, // 0x7b
    {{0x04, 0x00}, {0x00, 0x00}}, // 0x7c
    {{0x04, 0x00}, {0x00, 0x00}}, // 0x7d
    {{0x04, 0x00}, {0x00, 0x00}}, // 0x7e
    {{0x04, 0x00}, {0x00, 0x00}}, // 0x7f

    {{0x06, 0x40}, {0x06, 0x40}}, // 0x80 = 0x4000004f    right arrow
    {{0x06, 0x20}, {0x06, 0x20}}, // 0x81 = 0x40000050    left arrow
    {{0x06, 0x10}, {0x06, 0x10}}, // 0x82 = 0x40000051    down arrow
    {{0x06, 0x08}, {0x06, 0x08}}, // 0x83 = 0x40000052    up arrow

    {{0x06, 0x02}, {0x06, 0x02}} // 0x84 = 0x40000053    clear
};

void TRS80Emulator::OnKeyDown(SDL_Keysym &keysym)
{
  if (keysym.sym == REBOOT_SYM)
    Reset();

  else if (keysym.sym == RESET_SYM)
    NMI();

  else if (keysym.sym == TRACE_SYM)
    SetTrace(true);

  else if (keysym.sym == TRACE_SYM + 1)
    SetTrace(false);

  else if (keysym.sym == SDLK_LSHIFT)
  {
    m_shiftDown |= 1;
    m_shiftDown &= ~4;
    m_kbData[7] |= 1;
  }
  else if (keysym.sym == SDLK_RSHIFT)
  {
    m_shiftDown |= 2;
    m_shiftDown &= ~4;
    m_kbData[7] |= 1;
  }
  else
  {
    int32_t sym = (keysym.sym >= EXTENDED_SYM_START) ? (keysym.sym + 0x80 - EXTENDED_SYM_START) : keysym.sym;
    int mod = m_shiftDown ? 1 : 0;
    if (sym >= KB_SYM_COUNT)
    {
      cerr << "warning: unknown keyboard sym code 0x" << hex << keysym.sym << endl;
    }
    else
    {
      const uint8_t *scanInfo = g_symToCode[sym][mod];
      if ((scanInfo[0] == 0x00) && (scanInfo[1] == 0))
      {
        cerr << "warning: unmapped keyboard sym code 0x" << hex << keysym.sym << endl;
      }

      // handle codes that need to be unshifted
      else if (scanInfo[0] & 0x80)
      {
        memset(m_kbData, 0x00, sizeof(m_kbData));
        m_kbData[scanInfo[0] & 7] |= scanInfo[1];
        m_shiftDown = 0;
      }

      // handle codes that need to be shifted
      else if (scanInfo[0] & 0x40)
      {
        memset(m_kbData, 0x00, sizeof(m_kbData));
        m_kbData[7] |= 1;
        m_kbData[scanInfo[0] & 7] |= scanInfo[1];
        m_shiftDown = 4;
      }

      // handle codes that have the correct shift sense
      else
      {
        m_kbData[scanInfo[0] & 7] |= scanInfo[1];
        m_shiftDown &= ~4;
      }
    }
  }
}

void TRS80Emulator::OnKeyUp(SDL_Keysym &keysym)
{
  if (m_shiftDown & 4)
  {
    memset(m_kbData, 0x00, sizeof(m_kbData));
    m_shiftDown &= 0;
  }

  if (keysym.sym == SDLK_LSHIFT)
  {
    m_shiftDown &= !1;
    m_kbData[7] &= !1;
  }
  else if (keysym.sym == SDLK_RSHIFT)
  {
    m_shiftDown &= !2;
    m_kbData[7] &= !1;
  }
  else
  {
    int mod = m_shiftDown ? 1 : 0;
    int32_t sym = (keysym.sym >= EXTENDED_SYM_START) ? (keysym.sym + 0x80 - EXTENDED_SYM_START) : keysym.sym;
    if (sym < KB_SYM_COUNT)
    {
      const uint8_t *scanInfo = g_symToCode[sym][mod];
      m_kbData[scanInfo[0] & 7] &= !scanInfo[1];
    }
  }
}

/////////////////////////////////////////////////////////////

typedef uint8_t (TRS80Emulator::*ReadMemoryFn)(uint16_t);
typedef void (TRS80Emulator::*WriteMemoryFn)(uint16_t, uint8_t);

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadROM(uint16_t addr)
{
  return m_rom[addr];
}

uint8_t TRS80Emulator::ReadRAM(uint16_t addr)
{
  return m_ram[addr - RAM_MEM_ADDR];
}

void TRS80Emulator::WriteRAM(uint16_t addr, uint8_t val)
{
  m_ram[addr - RAM_MEM_ADDR] = val;
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadVideo(uint16_t addr)
{
  return m_videoRAM[addr - VIDEO_MEM_ADDR];
}

void TRS80Emulator::WriteVideo(uint16_t addr, uint8_t val)
{
  uint16_t offset = addr - VIDEO_MEM_ADDR;

  if (val < 0x20)
    val += 0x40;

  if (m_videoRAM[offset] != val)
  {
    m_videoRAM[offset] = val;
    WriteVideoChar(offset, val);
  }
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadKeyboard(uint16_t addr)
{
  uint16_t mask = 1;
  uint8_t value = 0x00;
  for (int i = 0; i < 8; ++i)
  {
    if (addr & mask)
      value |= m_kbData[i];
    mask = mask << 1;
  }
  return value;
}

/////////////////////////////////////////////////////////////

void TRS80Emulator::WritePrinter(uint16_t addr, uint8_t val)
{
  cerr << "PRINTER: " << val << setw(2) << hex << std::setfill('0') << (int)val << ' ' << (isgraph(val) ? (char)val : '.') << endl;
}

uint8_t TRS80Emulator::ReadPrinter(uint16_t addr)
{
  //  BUSY          = 0x80
  // *OUT_OF_PAPPER = 0x40
  // *UNIT_SELECT   = 0x20
  // *FAULT         = 0x10

  uint8_t val = 0x30;
  cerr << "PRINTER READ: " << setw(2) << hex << std::setfill('0') << (int)val << endl;

  return val;
}

/////////////////////////////////////////////////////////////

void TRS80Emulator::FDCInterrupt()
{
  //cerr << "FDC INTERRUPT" << endl;
  m_fdcPending = true;
  Interrupt();
}

void TRS80Emulator::InitFDC()
{
  m_drvSel = -1;
}

void TRS80Emulator::WriteFDC(uint16_t addr, uint8_t val)
{
  m_fdc.Write(addr, val);
}

uint8_t TRS80Emulator::ReadFDC(uint16_t addr)
{
  if (addr == 0x37ec)
    m_fdcPending = false;
  return m_fdc.Read(addr);
}

void TRS80Emulator::WriteDrvSel(uint16_t, uint8_t val)
{
  m_drvSel = val;
  int sel = -1;
  switch (val)
  {
  case 1:
    sel = 0;
    break;
  case 2:
    sel = 1;
    break;
  case 4:
    sel = 2;
    break;
  case 8:
    sel = 3;
    break;
  }
  m_fdc.SelectDrive(sel);
}

uint8_t TRS80Emulator::ReadDrvSel(uint16_t)
{
  return m_drvSel;
}

uint8_t TRS80Emulator::ReadInterrupt(uint16_t)
{
  // 0x80 = RTC interrupt
  // 0x80 = FDC interrupt

  uint8_t value = 0x00;
  //cerr << "INTERRUPT CLEAR" << endl;

  if (m_fdcPending) {
    value |= 0x40;
  }

  if (m_rtcPending) {
    value |= 0x80;
    m_rtcPending = false;
  }

  return value;
}

/////////////////////////////////////////////////////////////

static WriteMemoryFn g_trs80WritePrinterFDC[16] = {
    &TRS80Emulator::WriteLog,     // 0x37e0
    &TRS80Emulator::WriteDrvSel,  // 0x37e1
    &TRS80Emulator::WriteLog,     // 0x37e2
    &TRS80Emulator::WriteLog,     // 0x37e3
    &TRS80Emulator::WriteLog,     // 0x37e4
    &TRS80Emulator::WriteLog,     // 0x37e5
    &TRS80Emulator::WriteLog,     // 0x37e6
    &TRS80Emulator::WriteLog,     // 0x37e7
    &TRS80Emulator::WritePrinter, // 0x37e8
    &TRS80Emulator::WriteLog,     // 0x37e9
    &TRS80Emulator::WriteLog,     // 0x37ea
    &TRS80Emulator::WriteLog,     // 0x37eb
    &TRS80Emulator::WriteFDC,     // 0x37ec
    &TRS80Emulator::WriteFDC,     // 0x37ed
    &TRS80Emulator::WriteFDC,     // 0x37ee
    &TRS80Emulator::WriteFDC      // 0x37ef
};

static ReadMemoryFn g_trs80ReadPrinterFDC[16] = {
    &TRS80Emulator::ReadInterrupt, // 0x37e0
    &TRS80Emulator::ReadDrvSel,    // 0x37e1
    &TRS80Emulator::ReadLog,       // 0x37e2
    &TRS80Emulator::ReadLog,       // 0x37e3
    &TRS80Emulator::ReadLog,       // 0x37e4
    &TRS80Emulator::ReadLog,       // 0x37e5
    &TRS80Emulator::ReadLog,       // 0x37e6
    &TRS80Emulator::ReadLog,       // 0x37e7
    &TRS80Emulator::ReadPrinter,   // 0x37e8
    &TRS80Emulator::ReadLog,       // 0x37e9
    &TRS80Emulator::ReadLog,       // 0x37ea
    &TRS80Emulator::ReadLog,       // 0x37eb
    &TRS80Emulator::ReadFDC,       // 0x37ec
    &TRS80Emulator::ReadFDC,       // 0x37ed
    &TRS80Emulator::ReadFDC,       // 0x37ee
    &TRS80Emulator::ReadFDC        // 0x37ef
};

void TRS80Emulator::WritePrinterFDC(uint16_t addr, uint8_t val)
{
  std::invoke(g_trs80WritePrinterFDC[addr & 0x000f], *this, addr, val);
}

uint8_t TRS80Emulator::ReadPrinterFDC(uint16_t addr)
{
  return std::invoke(g_trs80ReadPrinterFDC[addr & 0x000f], *this, addr);
}

static WriteMemoryFn g_trs80WriteMemIO[16] = {
    &TRS80Emulator::WriteLog,        // 0x3700 to 0x370f
    &TRS80Emulator::WriteLog,        // 0x3710 to 0x371f
    &TRS80Emulator::WriteLog,        // 0x3720 to 0x372f
    &TRS80Emulator::WriteLog,        // 0x3730 to 0x373f
    &TRS80Emulator::WriteLog,        // 0x3740 to 0x374f
    &TRS80Emulator::WriteLog,        // 0x3750 to 0x375f
    &TRS80Emulator::WriteLog,        // 0x3760 to 0x376f
    &TRS80Emulator::WriteLog,        // 0x3770 to 0x377f
    &TRS80Emulator::WriteLog,        // 0x3780 to 0x378f
    &TRS80Emulator::WriteLog,        // 0x3790 to 0x379f
    &TRS80Emulator::WriteLog,        // 0x37a0 to 0x37af
    &TRS80Emulator::WriteLog,        // 0x37b0 to 0x37bf
    &TRS80Emulator::WriteLog,        // 0x37c0 to 0x37cf
    &TRS80Emulator::WriteLog,        // 0x37d0 to 0x37df
    &TRS80Emulator::WritePrinterFDC, // 0x37e0 to 0x37ef
    &TRS80Emulator::WriteLog,        // 0x37f0 to 0x37ff
};

static ReadMemoryFn g_trs80ReadMemIO[16] = {
    &TRS80Emulator::ReadLog,        // 0x3700 to 0x370f
    &TRS80Emulator::ReadLog,        // 0x3710 to 0x371f
    &TRS80Emulator::ReadLog,        // 0x3720 to 0x372f
    &TRS80Emulator::ReadLog,        // 0x3730 to 0x373f
    &TRS80Emulator::ReadLog,        // 0x3740 to 0x374f
    &TRS80Emulator::ReadLog,        // 0x3750 to 0x375f
    &TRS80Emulator::ReadLog,        // 0x3760 to 0x376f
    &TRS80Emulator::ReadLog,        // 0x3770 to 0x377f
    &TRS80Emulator::ReadLog,        // 0x3780 to 0x378f
    &TRS80Emulator::ReadLog,        // 0x3790 to 0x379f
    &TRS80Emulator::ReadLog,        // 0x37a0 to 0x37af
    &TRS80Emulator::ReadLog,        // 0x37b0 to 0x37bf
    &TRS80Emulator::ReadLog,        // 0x37c0 to 0x37cf
    &TRS80Emulator::ReadLog,        // 0x37d0 to 0x37df
    &TRS80Emulator::ReadPrinterFDC, // 0x37e0 to 0x37ef
    &TRS80Emulator::ReadLog,        // 0x37f0 to 0x37ff
};

void TRS80Emulator::WriteMemIO(uint16_t addr, uint8_t val)
{
  std::invoke(g_trs80WriteMemIO[(addr & 0x00f0) >> 4], *this, addr, val);
}

uint8_t TRS80Emulator::ReadMemIO(uint16_t addr)
{
  return std::invoke(g_trs80ReadMemIO[(addr & 0x00f0) >> 4], *this, addr);
}

static WriteMemoryFn g_trs80WriteIO[16] = {
    &TRS80Emulator::WriteLog,   // 0x3000 to 0x30ff
    &TRS80Emulator::WriteLog,   // 0x3100 to 0x31ff
    &TRS80Emulator::WriteLog,   // 0x3200 to 0x32ff
    &TRS80Emulator::WriteLog,   // 0x3300 to 0x33ff
    &TRS80Emulator::WriteLog,   // 0x3400 to 0x34ff
    &TRS80Emulator::WriteLog,   // 0x3500 to 0x35ff
    &TRS80Emulator::WriteLog,   // 0x3600 to 0x36ff
    &TRS80Emulator::WriteMemIO, // 0x3700 to 0x37ff
    &TRS80Emulator::WriteLog,   // 0x3800 to 0x38ff
    &TRS80Emulator::WriteLog,   // 0x3900 to 0x39ff
    &TRS80Emulator::WriteLog,   // 0x3a00 to 0x3aff
    &TRS80Emulator::WriteLog,   // 0x3b00 to 0x3bff
    &TRS80Emulator::WriteVideo, // 0x3c00 to 0x3cff
    &TRS80Emulator::WriteVideo, // 0x3d00 to 0x3dff
    &TRS80Emulator::WriteVideo, // 0x3e00 to 0x3eff
    &TRS80Emulator::WriteVideo, // 0x3f00 to 0x3fff
};

static ReadMemoryFn g_trs80ReadIO[16] = {
    &TRS80Emulator::ReadLog,      // 0x3000 to 0x30ff
    &TRS80Emulator::ReadLog,      // 0x3100 to 0x31ff
    &TRS80Emulator::ReadLog,      // 0x3200 to 0x32ff
    &TRS80Emulator::ReadLog,      // 0x3300 to 0x33ff
    &TRS80Emulator::ReadLog,      // 0x3400 to 0x34ff
    &TRS80Emulator::ReadLog,      // 0x3500 to 0x35ff
    &TRS80Emulator::ReadLog,      // 0x3600 to 0x36ff
    &TRS80Emulator::ReadMemIO,    // 0x3700 to 0x37ff
    &TRS80Emulator::ReadKeyboard, // 0x3800 to 0x38ff
    &TRS80Emulator::ReadKeyboard, // 0x3900 to 0x39ff
    &TRS80Emulator::ReadKeyboard, // 0x3a00 to 0x3aff
    &TRS80Emulator::ReadKeyboard, // 0x3b00 to 0x3bff
    &TRS80Emulator::ReadVideo,    // 0x3c00 to 0x3cff
    &TRS80Emulator::ReadVideo,    // 0x3d00 to 0x3dff
    &TRS80Emulator::ReadVideo,    // 0x3e00 to 0x3eff
    &TRS80Emulator::ReadVideo,    // 0x3f00 to 0x3fff
};

void TRS80Emulator::WriteIO(uint16_t addr, uint8_t val)
{
  return std::invoke(g_trs80WriteIO[(addr & 0x0f00) >> 8], *this, addr, val);
}

uint8_t TRS80Emulator::ReadIO(uint16_t addr)
{
  return std::invoke(g_trs80ReadIO[(addr & 0x0f00) >> 8], *this, addr);
}

static ReadMemoryFn g_trs80ReadMemory[16] = {
    &TRS80Emulator::ReadROM, // 0x0000 to 0x0fff
    &TRS80Emulator::ReadROM, // 0x1000 to 0x1fff
    &TRS80Emulator::ReadROM, // 0x2000 to 0x2fff
    &TRS80Emulator::ReadIO,  // 0x3000 to 0x3fff
    &TRS80Emulator::ReadRAM, // 0x4000 to 0x4fff
    &TRS80Emulator::ReadRAM, // 0x5000 to 0x5fff
    &TRS80Emulator::ReadRAM, // 0x6000 to 0x6fff
    &TRS80Emulator::ReadRAM, // 0x7000 to 0x7fff
    &TRS80Emulator::ReadRAM, // 0x8000 to 0x8fff
    &TRS80Emulator::ReadRAM, // 0x9000 to 0x9fff
    &TRS80Emulator::ReadRAM, // 0xa000 to 0xafff
    &TRS80Emulator::ReadRAM, // 0xb000 to 0xbfff
    &TRS80Emulator::ReadRAM, // 0xc000 to 0xcfff
    &TRS80Emulator::ReadRAM, // 0xd000 to 0xdfff
    &TRS80Emulator::ReadRAM, // 0xe000 to 0xefff
    &TRS80Emulator::ReadRAM, // 0xf000 to 0xffff
};

static WriteMemoryFn g_trs80WriteMemory[16] = {
    &TRS80Emulator::WriteNull, // 0x0000 to 0x0fff
    &TRS80Emulator::WriteNull, // 0x1000 to 0x1fff
    &TRS80Emulator::WriteNull, // 0x2000 to 0x2fff
    &TRS80Emulator::WriteIO,   // 0x3000 to 0x3fff
    &TRS80Emulator::WriteRAM,  // 0x4000 to 0x4fff
    &TRS80Emulator::WriteRAM,  // 0x5000 to 0x5fff
    &TRS80Emulator::WriteRAM,  // 0x6000 to 0x6fff
    &TRS80Emulator::WriteRAM,  // 0x7000 to 0x7fff
    &TRS80Emulator::WriteRAM,  // 0x8000 to 0x8fff
    &TRS80Emulator::WriteRAM,  // 0x9000 to 0x9fff
    &TRS80Emulator::WriteRAM,  // 0xa000 to 0xafff
    &TRS80Emulator::WriteRAM,  // 0xb000 to 0xbfff
    &TRS80Emulator::WriteRAM,  // 0xc000 to 0xcfff
    &TRS80Emulator::WriteRAM,  // 0xd000 to 0xdfff
    &TRS80Emulator::WriteRAM,  // 0xe000 to 0xefff
    &TRS80Emulator::WriteRAM,  // 0xf000 to 0xffff
};


void TRS80Emulator::WrZ80(register uint16_t addr, register uint8_t val)
{
  return std::invoke(g_trs80WriteMemory[(addr & 0xf000) >> 12], *this, addr, val);
}

uint8_t TRS80Emulator::RdZ80(register uint16_t addr)
{
  return std::invoke(g_trs80ReadMemory[(addr & 0xf000) >> 12], *this, addr);
}

/////////////////////////////////////////////////////////////


extern "C" {
#include "nfd.h"
};

/////////////////////////////////////////////////////////////

void TRS80Emulator::WriteFF(register uint16_t, register uint8_t val)
{
  // detect changes in cassette motor
  bool cassOn = (val & 0x04) != 0;
  if (cassOn != m_cassetteMotor) {
    m_cassetteMotor = cassOn;
    if (!cassOn) {
      cerr << "CASS: motor off" << endl;
      m_cassetteTrigger = false;
    }
    else {
      cerr << "CASS: motor on" << endl;
      m_cassetteTrigger = true;
    }
  }

  // detect changes in cassette output when trigger is set
  int cassOut = (val & 0x3);
  if (cassOn && m_cassetteTrigger && (cassOut != 0)) {
    cerr << "CASS: writing to cassette" << endl;
    m_cassetteTrigger = false;
    nfdchar_t * outPath = NULL;
    nfdresult_t result = NFD_SaveDialog(NULL, NULL, &outPath);
  }
}

uint8_t TRS80Emulator::ReadFF(register uint16_t)
{
  // if a read is done when the trigger is active, we are reading a cassette
  if (m_cassetteTrigger) {
    cerr << "CASS: reading from cassette" << endl;
    m_cassetteTrigger = false;
    nfdchar_t * outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);
  }
  return 0;
}

/////////////////////////////////////////////////////////////

static WriteMemoryFn g_trs80WriteFx[16] = {
    &TRS80Emulator::WriteNull, // 0x00 to 0x0f
    &TRS80Emulator::WriteNull, // 0x10 to 0x1f
    &TRS80Emulator::WriteNull, // 0x20 to 0x2f
    &TRS80Emulator::WriteNull, // 0x30 to 0x3f
    &TRS80Emulator::WriteNull, // 0x40 to 0x4f
    &TRS80Emulator::WriteNull, // 0x50 to 0x5f
    &TRS80Emulator::WriteNull, // 0x60 to 0x6f
    &TRS80Emulator::WriteNull, // 0x70 to 0x7f
    &TRS80Emulator::WriteNull, // 0x80 to 0x8f
    &TRS80Emulator::WriteNull, // 0x90 to 0x9f
    &TRS80Emulator::WriteNull, // 0xa0 to 0xaf
    &TRS80Emulator::WriteNull, // 0xb0 to 0xbf
    &TRS80Emulator::WriteNull, // 0xc0 to 0xcf
    &TRS80Emulator::WriteNull, // 0xd0 to 0xdf
    &TRS80Emulator::WriteNull, // 0xe0 to 0xef
    &TRS80Emulator::WriteFF    // 0xf0 to 0xff
};

static ReadMemoryFn g_trs80ReadFx[16] = {
    &TRS80Emulator::ReadNull, // 0x00 to 0x0f
    &TRS80Emulator::ReadNull, // 0x10 to 0x1f
    &TRS80Emulator::ReadNull, // 0x20 to 0x2f
    &TRS80Emulator::ReadNull, // 0x30 to 0x3f
    &TRS80Emulator::ReadNull, // 0x40 to 0x4f
    &TRS80Emulator::ReadNull, // 0x50 to 0x5f
    &TRS80Emulator::ReadNull, // 0x60 to 0x6f
    &TRS80Emulator::ReadNull, // 0x70 to 0x7f
    &TRS80Emulator::ReadNull, // 0x80 to 0x8f
    &TRS80Emulator::ReadNull, // 0x90 to 0x9f
    &TRS80Emulator::ReadNull, // 0xa0 to 0xaf
    &TRS80Emulator::ReadNull, // 0xb0 to 0xbf
    &TRS80Emulator::ReadNull, // 0xc0 to 0xcf
    &TRS80Emulator::ReadNull, // 0xd0 to 0xdf
    &TRS80Emulator::ReadNull, // 0xe0 to 0xef
    &TRS80Emulator::ReadFF    // 0xf0 to 0xff
};

void TRS80Emulator::WriteFx(register uint16_t port, register uint8_t val)
{
  std::invoke(g_trs80WriteFx[port & 0x000f], *this, port, val);
}

uint8_t TRS80Emulator::ReadFx(register uint16_t port)
{
  return std::invoke(g_trs80ReadFx[port & 0x000f], *this, port);
}

/////////////////////////////////////////////////////////////

static WriteMemoryFn g_trs80WritePort[16] = {
    &TRS80Emulator::WriteNull, // 0x00 to 0x0f
    &TRS80Emulator::WriteNull, // 0x10 to 0x1f
    &TRS80Emulator::WriteNull, // 0x20 to 0x2f
    &TRS80Emulator::WriteNull, // 0x30 to 0x3f
    &TRS80Emulator::WriteNull, // 0x40 to 0x4f
    &TRS80Emulator::WriteNull, // 0x50 to 0x5f
    &TRS80Emulator::WriteNull, // 0x60 to 0x6f
    &TRS80Emulator::WriteNull, // 0x70 to 0x7f
    &TRS80Emulator::WriteNull, // 0x80 to 0x8f
    &TRS80Emulator::WriteNull, // 0x90 to 0x9f
    &TRS80Emulator::WriteNull, // 0xa0 to 0xaf
    &TRS80Emulator::WriteNull, // 0xb0 to 0xbf
    &TRS80Emulator::WriteNull, // 0xc0 to 0xcf
    &TRS80Emulator::WriteNull, // 0xd0 to 0xdf
    &TRS80Emulator::WriteNull, // 0xe0 to 0xef
    &TRS80Emulator::WriteFx    // 0xf0 to 0xff
};

static ReadMemoryFn g_trs80ReadPort[16] = {
    &TRS80Emulator::ReadNull, // 0x00 to 0x0f
    &TRS80Emulator::ReadNull, // 0x10 to 0x1f
    &TRS80Emulator::ReadNull, // 0x20 to 0x2f
    &TRS80Emulator::ReadNull, // 0x30 to 0x3f
    &TRS80Emulator::ReadNull, // 0x40 to 0x4f
    &TRS80Emulator::ReadNull, // 0x50 to 0x5f
    &TRS80Emulator::ReadNull, // 0x60 to 0x6f
    &TRS80Emulator::ReadNull, // 0x70 to 0x7f
    &TRS80Emulator::ReadNull, // 0x80 to 0x8f
    &TRS80Emulator::ReadNull, // 0x90 to 0x9f
    &TRS80Emulator::ReadNull, // 0xa0 to 0xaf
    &TRS80Emulator::ReadNull, // 0xb0 to 0xbf
    &TRS80Emulator::ReadNull, // 0xc0 to 0xcf
    &TRS80Emulator::ReadNull, // 0xd0 to 0xdf
    &TRS80Emulator::ReadNull, // 0xe0 to 0xef
    &TRS80Emulator::ReadFx    // 0xf0 to 0xff
};

void TRS80Emulator::OutZ80(register uint16_t port, register uint8_t val)
{
  std::invoke(g_trs80WritePort[(port & 0x0f0) >> 4], *this, port, val);
}

uint8_t TRS80Emulator::InZ80(register uint16_t port)
{
  return std::invoke(g_trs80ReadPort[(port & 0x0f0) >> 4], *this, port);
}
