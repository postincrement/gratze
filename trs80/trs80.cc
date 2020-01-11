#include <functional>
#include <iostream>
#include <iomanip>
#include <memory.h>

#include "config.h"
#include "misc.h"
#include "trs80.h"

#include "model1/model1.h"

/*
  Port FF write
  -------------
    bit 0,1   cassette voltage level
    bit 2     cassette motor on/off
    bit 3     0 = 64 char, 1 = 32 char
    bit 4-7   unused

  Port FF read
  -------------
    bit 0,6   unused
    bit 7     cassette read
*/


using namespace std;

#define RTC_INTERVAL_MS 40

#define RESET_SYM   SDLK_F1

#define EXTENDED_SYM_START 0x4000004f

/////////////////////////////////////////////////////////////

TRS80Emulator::TRS80Emulator(EmulatorInfo * info)
  : Z80Emulator(info)
{
  m_rtcEnabled = false;
  m_fdcEnabled = false;
}

void TRS80Emulator::Init()
{  
  VirtualScreen::AddType<TRS80Video>("trs80");
}

bool TRS80Emulator::Open(const Options &options)
{
  m_rtcTimer = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
  m_rtcPending = false;
  m_fdcPending = 0;

  m_cassetteMotor    = false;
  m_cassetteTrigger  = false;
  m_cassette2        = false;
  m_32Col            = false;

  cout << "info: FDC is " << (m_fdcEnabled ? "en" : "dis") << "abled" << endl; 
  if (m_fdcEnabled) {
    m_fdc.reset(new WD_FD1771());
    m_fdc->SetInterruptHandler(std::bind(&TRS80Emulator::FDCInterrupt, this));
  }

  cout << "info: RTC is " << (m_rtcEnabled ? "en" : "dis") << "abled" << endl; 

  return Emulator::Open(options);
}

void TRS80Emulator::Reset(int addr)
{
  // clear keyboard
  memset(m_kbData, 0x00, sizeof(m_kbData));
  m_shiftDown = 0;

  // init floppy drive 
  InitFDC();  

  return Z80Emulator::Reset(addr);
}

bool TRS80Emulator::CreatePixelFont(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  // set graphics
  uint8_t maskRight = (1 << (fontInfo.m_width / 2)) - 1;
  uint8_t maskLeft  = maskRight << (fontInfo.m_width / 2);

  for (uint8_t i = 0; i < 64; ++i) {
    uint8_t * dst = &fontData[(128 + i) * fontInfo.m_height];
    uint8_t val = i;
    for (int y = 0; y < 3; ++y) {
      *dst = 0;
      if (val & 1)
        *dst |= maskRight;
      if (val & 2)
        *dst |= maskLeft;
      for (int z = 1; z < fontInfo.m_height / 3; ++z)
        dst[z] = dst[0];  
      dst += fontInfo.m_height / 3;
      val = val >> 2;  
    }
  }
  memcpy(&fontData[(128 + 64) * fontInfo.m_height], &fontData[128 * fontInfo.m_height], 64 * fontInfo.m_height);
  return true;
}

bool TRS80Emulator::Poll()
{
  auto now = std::chrono::system_clock::now();
  if (now > m_rtcTimer) {
    if (!m_rtcPending) {
      m_rtcTimer = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
      m_rtcPending = true;
      Interrupt();
    }
  }

  return Z80Emulator::Poll();
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadNull(uint16_t)
{
  return 0xff;
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

void TRS80Emulator::OnKeyDown(const SDL_Keysym & keysym)
{
  if (keysym.sym == RESET_SYM)
    NMI();

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
    if (sym >= KB_SYM_COUNT) {
      Z80Emulator::OnKeyDown(keysym);
    }
    else
    {
      const uint8_t *scanInfo = g_symToCode[sym][mod];
      if ((scanInfo[0] == 0x00) && (scanInfo[1] == 0))
      {
        cerr << "warning: unmapped keyboard sym code " << HEXFORMAT0x2(keysym.sym) << endl;
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

void TRS80Emulator::OnKeyUp(const SDL_Keysym & keysym)
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
    if (sym < KB_SYM_COUNT) {
      const uint8_t *scanInfo = g_symToCode[sym][mod];
      m_kbData[scanInfo[0] & 7] &= !scanInfo[1];
    }
    else 
      Z80Emulator::OnKeyUp(keysym);
  }
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadKeyboard(uint16_t addr)
{
  uint16_t mask = 1;
  uint8_t value = 0x00;
  for (int i = 0; i < 8; ++i) {
    if (addr & mask)
      value |= m_kbData[i];
    mask = mask << 1;
  }
  return value;
}

/////////////////////////////////////////////////////////////

void TRS80Emulator::WritePrinter(uint16_t addr, uint8_t val)
{
  cerr << "PRINTER: " << HEXFORMAT0x2(val) << ' ' << (isgraph(val) ? (char)val : '.') << endl;
}

uint8_t TRS80Emulator::ReadPrinter(uint16_t addr)
{
  //  BUSY          = 0x80
  // *OUT_OF_PAPPER = 0x40
  // *UNIT_SELECT   = 0x20
  // *FAULT         = 0x10

  uint8_t val = 0x30;
  cerr << "PRINTER READ: " << HEXFORMAT0x2(val) << endl;

  return val;
}

/////////////////////////////////////////////////////////////

bool TRS80Emulator::MountDrive(int driveNum, VirtualDrive *drive, bool readOnly)
{
  if (!m_fdcEnabled || !m_fdc)
    return false;

  return m_fdc->MountDrive(driveNum, drive, readOnly);
}

/////////////////////////////////////////////////////////////////////////////////////

void TRS80Emulator::FDCInterrupt()
{
  cerr << "FDC: interrupt" << endl;
  m_fdcPending = 3;
  Interrupt();
}

void TRS80Emulator::InitFDC()
{
  if (!m_fdcEnabled || !m_fdc)
    return;

  m_fdc->Reset();
  m_drvSel = -1;
}

void TRS80Emulator::WriteFDC(uint16_t addr, uint8_t val)
{
  if (m_fdcEnabled && m_fdc)
    m_fdc->Write(addr, val);
}

uint8_t TRS80Emulator::ReadFDC(uint16_t addr)
{
  if (!m_fdcEnabled)
    return ReadNull(addr);

  m_fdcPending &= 2;

  return m_fdc->Read(addr);
}

void TRS80Emulator::WriteDrvSel(uint16_t, uint8_t val)
{
  if (!m_fdcEnabled || !m_fdc)
    return;

  m_drvSel = val;
  int sel = -1;
  switch (val) {
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
  m_fdc->SelectDrive(sel);
}

uint8_t TRS80Emulator::ReadDrvSel(uint16_t addr)
{
  if (!m_fdcEnabled || !m_fdc)
    return ReadNull(addr);

  return m_drvSel;
}

uint8_t TRS80Emulator::ReadInterrupt(uint16_t addr)
{
  if (!m_fdcEnabled && !m_rtcEnabled)
    return ReadNull(addr);

  // 0x80 = RTC interrupt
  // 0x80 = FDC interrupt

  uint8_t value = 0x00;
  //cerr << "INTERRUPT CLEAR" << endl;

  if (m_fdcPending != 0) {
    m_fdcPending &= 1;
    value |= 0x40;
    //SetTrace(1);
  }

  if (m_rtcPending) {
    value |= 0x80;
    m_rtcPending = false;
  }

  return value;
}

/////////////////////////////////////////////////////////////

extern "C" {
#include "nfd.h"
};

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:
      return ReadFF(port & 0xff);
  }
  cerr << "trs80: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void TRS80Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      return WriteFF(port & 0xff, data);
  }
  cerr << "trs680: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}


void TRS80Emulator::WriteFF(register uint16_t, register uint8_t val)
{
  // detect changes in cassette motor
  bool cassOn = (val & 0x04) != 0;
  if (cassOn != m_cassetteMotor) {
    m_cassetteMotor = cassOn;

    if (cassOn) {
      cerr << "CASS: motor on" << endl;
      m_cassetteTrigger = true;
    }
    else {
      cerr << "CASS: motor off" << endl;
      m_cassetteTrigger = false;
      if (m_cassette && !m_cassette->IsReading()) {
        // get name from data, if we can
        std::string filename = m_cassette->GetFilename();
        cerr << "CASS: filename is '" << filename << "'" << endl;

        nfd_SaveDialogExt extInfo;
        memset(&extInfo, 0, sizeof(extInfo));
        extInfo.filterList      = "cas;cpt;wav";
        extInfo.title           = "Save cassette image";
        extInfo.defaultFilename = (filename.length() > 0) ? filename.c_str() : NULL;

        nfdchar_t * outPath = NULL;

        if (NFD_SaveDialogExt(&extInfo, &outPath) == NFD_OKAY) {
          m_cassette->WriteClose(outPath);
        }
        free(outPath);
      }
      m_cassette.reset();
    }
  }

  // detect changes in cassette output when trigger is set
  int cassOut = (val & 0x3);
  if (cassOn) {

    //cerr << "CASS: write data " << HEXFORMAT0x2(cassOut) << ' ' << m_cassetteTrigger << endl;

    // open for writing if this is the first time
    if (m_cassetteTrigger && (cassOut != 0)) {
      cerr << "CASS: writing to cassette" << endl;
      m_cassette.reset(new VirtualCassetteFile());
      m_cassette->WriteOpen();
      m_cassetteTrigger = false;
    }

    // if writing, peek inside the CPU to get the byte data
    if (m_cassette) {
      uint16_t pc = m_cpu.PC.W;
      if (pc == 0x228) {
        uint16_t sp = m_cpu.SP.W;
        uint16_t c1 = ReadMemoryWord(sp - 0);
        uint16_t c2 = ReadMemoryWord(sp + 2);
        if ((c1 == 0x01df) && (c2 == 0x026e) && (m_cpu.BC.B.l == 0x8)) {
          m_cassette->WriteByte(m_cpu.DE.B.h);
        }
      }
    }
  }

  // detect changes in 32/64 columns mode
  bool new32Col = val & 0x08;
  if (new32Col != m_32Col) {
    m_32Col = new32Col;
    cout << "trs80: 32 column mode turned " << (m_32Col ? "on" : "off") << endl;
    ((TRS80Video *)m_video.get())->Set32Col(m_32Col);
  }
}

uint8_t TRS80Emulator::ReadFF(register uint16_t)
{
  // if a read is done when the trigger is active, we are reading a cassette
  if (m_cassetteTrigger) {
    cerr << "CASS: reading from cassette" << endl;
    m_cassetteTrigger = false;

    nfd_OpenDialogExt extInfo;
    memset(&extInfo, 0, sizeof(extInfo));
    extInfo.filterList      = "cas;cpt;wav";
    extInfo.title           = "Open cassette image";

    nfdchar_t * outPath = NULL;
    if (NFD_OpenDialogExt(&extInfo, &outPath) == NFD_OKAY) {
      m_cassette.reset(new VirtualCassetteFile());
      m_cassette->ReadOpen(outPath);
    } 
  }

  // peek inside the CPU
  uint16_t pc = m_cpu.PC.W;

  // always set clock bit
  if (pc == 0x245) {
    return 0x80;
  }

  // look for data bits
  else if (pc == 0x0255) {

    /*
        code:

        0253   db ff      in a,(0ffh) 
        0255   47         ld b,a     
        0256   f1         pop af     
        0257   cb 10      rl b        
        0259   17         rla        
    */

    uint16_t sp = m_cpu.SP.W;
    uint16_t c4 = ReadMemoryWord(sp + 4);

    // sync byte
    if (c4 == 0x029b) {

       /*
       stack looks like:
       SP + 00 : 0x??      F saved
       SP + 01 : 0x??      A saved
       SP + 02 : 0x41e8    BC saved
       SP + 04 : 0x029b    return address
       */
      //DumpStack(5);
      uint8_t data = m_cassette->ReadByte();
      //cerr << "CASS: Read sync " << HEXFORMAT0x2(data) << endl;
      WriteMemory(sp + 1, data >> 1); // get A ready to accept new bit 0 
      //if (data == 0xa5)
      //  SetTrace(true);
      return (data << 7);       // shift bit 0 into bit 7
    }
    else {
       /*
       stack looks like:
       SP + 00 : 0x??      F saved
       SP + 01 : 0x??      A saved
       SP + 02 : 0x??      C saved
       SP + 03 : 0x??      B saved
       SP + 04 : 0x029b    return address
       */
      //DumpStack(5);
      uint8_t data;
      if (ReadMemory(sp + 3) == 1) {
        data = m_cassette->ReadByte();
        //cerr << "CASS: Read data " << HEXFORMAT0x2(data) << endl;
      }
      WriteMemory(sp + 1, data >> 1); // get A ready to accept new bit 0 
      //SetTrace(true);
      return (data << 7);       // shift bit 0 into bit 7
    }
  }
  else {
    DumpStack(5);
  }

  return 0;
}

/////////////////////////////////////////////////////////////

TRS80Video::TRS80Video(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : SingleColourMemoryMappedVideo(mainWindow, emulator, options, info)
{
  m_32Col = false;
}

void TRS80Video::Set32Col(bool val)
{ 
  if (val != m_32Col) {
    m_32Col = val;
    SetColScale(m_32Col ? 2 : 1);
    RefreshScreen();
  }
}

void TRS80Video::WriteMemoryAtAddress(int addr, uint8_t ch)
{
  if (ch < 0x20)
    ch += 0x40;
    
  SingleColourMemoryMappedVideo::WriteMemoryAtAddress(addr, ch);
}

FontChar TRS80Video::GetCharAtPos(int x, int y)
{
  int pos = (y * m_cols) + (x * (m_32Col ? 2 : 1));
  return m_memory[pos & m_visibleMask];
}
