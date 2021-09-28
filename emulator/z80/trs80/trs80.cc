#include <functional>
#include <iostream>
#include <iomanip>
#include <memory.h>

#include "common/config.h"
#include "common/misc.h"
#include "z80/trs80/trs80.h"

#include "z80/trs80/model1/model1.h"

extern "C" {
#include "nfd.h"
};

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


const ScannedKeyboard::ScanCode keys[8*8] = {
  { "@" },      { "A" },      { "B" },      { "C" },    { "D"},     { "E" } ,   { "F" } ,     { "G" } ,
  { "H" },      { "I" },      { "J" },      { "K" },    { "L"},     { "M" } ,   { "N" } ,     { "O" } ,
  { "P" },      { "Q" },      { "R" },      { "S" },    { "T"},     { "U" } ,   { "V" } ,     { "W" } ,
  { "X" },      { "Y" },      { "Z" },      {  0 },     { 0 },      { 0 } ,     { 0 } ,       { 0 } ,
  { "0" },      { "1" },      { "2" },      { "3" },    { "4"},     { "5" } ,   { "6" } ,     { "7" } ,
  { "8" },      { "9" },      { ":" },      { ";" },    { ","},     { "-" } ,   { "." } ,     { "/" } ,
  { "Return" }, { "Clear" },  { "Break" },  { "Up" },   { "Down"},  { "Left" }, { "Right" },  { " " } ,
  { "Shift" },  { 0 },        { 0 },        { 0 },      { 0 },      { 0 },      { 0 },        { 0 }
};

const ScannedKeyboard::ScanCode shiftedkeys[8*8] = {
  { "@" },      { "a" },      { "b" },      { "c" },    { "d"},     { "e" } ,   { "f" } ,     { "g" } ,
  { "h" },      { "i" },      { "j" },      { "k" },    { "l"},     { "m" } ,   { "n" } ,     { "o" } ,
  { "p" },      { "q" },      { "r" },      { "s" },    { "t"},     { "u" } ,   { "v" } ,     { "w" } ,
  { "x" },      { "y" },      { "z" },      {  0 },     { 0 },      { 0 } ,     { 0 } ,       { 0 } ,
  { "0" },      { "!" },      { "\"" },     { "#" },    { "$"},     { "%" } ,   { "&" } ,     { "/" } ,
  { "(" },      { ")" },      { "*" },      { "+" },    { "<"},     { "=" } ,   { ">" } ,     { "?" } ,
  { "Return" }, { "Clear" },  { "Break" },  { "Up" },   { "Down"},  { "Left" }, { "Right" },  { " " } ,
  { "Shift" },  { 0 },        { 0 },        { 0 },      { 0 },      { 0 },      { 0 },        { 0 }
};

static ScannedKeyboard::ScanLayout g_trs80Keys = {
  8, 8,
  NULL,
  keys,
  shiftedkeys,
  {
    { "Backspace", "Left" },
  }
};


/////////////////////////////////////////////////////////////

void TRS80Emulator::Instantiate()
{
  MemoryMappedScreen::AddType<TRS80Video>("trs80");
}

TRS80Emulator::TRS80Emulator(const EmulatorInfo * info)
  : Z80Emulator(info)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool TRS80Emulator::Open(const Options & options)
{
  cout << "info: FDC is " << (m_fdcEnabled ? "en" : "dis") << "abled" << endl;
  if (m_fdcEnabled) {
    m_fdc.reset(new WD_FD1771(options.m_fdcDebug));
    using namespace placeholders;
    m_fdc->SetInterruptHandler(std::bind(&TRS80Emulator::FDCInterrupt, this, _1));
  }

  if (!Z80Emulator::Open(options))
    return false;

  ScannedKeyboard * kb = new ScannedKeyboard();
  SetKeyboard(kb);
  kb->Compile(g_trs80Keys);

  m_rtcTimer   = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
  m_rtcPending = false;
  m_fdcPending = 0;

  m_cassetteMotor    = false;
  m_cassetteTrigger  = false;
  m_cassette2        = false;
  m_32Col            = false;

  cout << "info: RTC is " << (m_rtcEnabled ? "en" : "dis") << "abled" << endl;

  return true;
}

void TRS80Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);

  using namespace std::placeholders;
  if (m_rtcEnabled) {
    AddRealTimePollDef(0.04, std::bind(&TRS80Emulator::RTCInterrupt, this));
  }
  // init floppy drive
  InitFDC();
}

bool TRS80Emulator::CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData)
{
  // basic font data already in memory

  // set graphics from 0x80 to 0xbf
  uint8_t maskRight = (1 << (fontInfo.m_width / 2)) - 1;
  uint8_t maskLeft  = maskRight << (fontInfo.m_width / 2);

  for (uint8_t i = 0; i < 64; ++i) {
    uint8_t * dst = &fontData[(128 + i) * fontInfo.m_height];
    uint8_t val = i;
    for (int y = 0; y < 3; ++y) {
      *dst = 0;
      if (val & 1)
        *dst |= maskLeft;
      if (val & 2)
        *dst |= maskRight;
      for (int z = 1; z < (fontInfo.m_height / 3); ++z)
        dst[z] = dst[0];
      dst += fontInfo.m_height / 3;
      val = val >> 2;
    }
  }

  // duplicate graphics data from 0xc0 to 0xff
  memcpy(&fontData[(128 + 64) * fontInfo.m_height], &fontData[128 * fontInfo.m_height], 64 * fontInfo.m_height);

  return true;
}

void TRS80Emulator::RTCInterrupt()
{
  auto now = std::chrono::system_clock::now();
  if (now > m_rtcTimer) {
    if (!m_rtcPending) {
      m_rtcTimer = std::chrono::system_clock::now() + std::chrono::milliseconds(RTC_INTERVAL_MS);
      m_rtcPending = true;
      Interrupt();
    }
  }
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadNull(uint16_t) const
{
  return 0xff;
}

/////////////////////////////////////////////////////////////

uint8_t TRS80Emulator::ReadKeyboard(uint16_t addr) const
{
  return m_keyboard->Read(addr);
}

/////////////////////////////////////////////////////////////

void TRS80Emulator::WritePrinter(uint16_t addr, uint8_t val)
{
  cerr << "PRINTER: " << HEXFORMAT0x2(val) << ' ' << (isgraph(val) ? (char)val : '.') << endl;
}

uint8_t TRS80Emulator::ReadPrinter(uint16_t addr) const
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
  if (!m_fdcEnabled || !m_fdc) {
    cerr << "error: cannot mount drives when FDC is disabled" << endl;
    return false;
  }

  return m_fdc->MountDrive(driveNum, drive, readOnly);
}

/////////////////////////////////////////////////////////////////////////////////////

void TRS80Emulator::FDCInterrupt(bool v)
{
  if (m_options.m_fdcDebug)
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

uint8_t TRS80Emulator::ReadFDC(uint16_t addr) const
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

uint8_t TRS80Emulator::ReadDrvSel(uint16_t addr) const
{
  if (!m_fdcEnabled || !m_fdc)
    return ReadNull(addr);

  return m_drvSel;
}

uint8_t TRS80Emulator::ReadInterrupt(uint16_t addr) const
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
/*
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
*/
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
    ((TRS80Video *)m_memMapScreen.get())->Set32Col(m_32Col);
  }
}

uint8_t TRS80Emulator::ReadFF(register uint16_t)
{
  // if a read is done when the trigger is active, we are reading a cassette
  if (m_cassetteTrigger) {
    cerr << "CASS: reading from cassette" << endl;
    m_cassetteTrigger = false;
/*
    nfd_OpenDialogExt extInfo;
    memset(&extInfo, 0, sizeof(extInfo));
    extInfo.filterList      = "cas;cpt;wav";
    extInfo.title           = "Open cassette image";

    nfdchar_t * outPath = NULL;
    if (NFD_OpenDialogExt(&extInfo, &outPath) == NFD_OKAY) {
      m_cassette.reset(new VirtualCassetteFile());
      m_cassette->ReadOpen(outPath);
    }
*/
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

TRS80Video::TRS80Video(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : MonoMemoryMappedScreen(mainWindow, options, info)
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

  MonoMemoryMappedScreen::WriteMemoryAtAddress(addr, ch);
}
