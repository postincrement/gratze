

#include <iostream>
#include <iomanip>

#include "src/misc.h"
#include "z80/super80/super80.h"
#include "video/virtual_screen.h"
#include "video/chargen_2513.h"
#include "devices/keyscan.h"


using namespace std;

#define   SUPER80_RAM_START_ADDR        0x0000
#define   SUPER80_RAM_END_ADDR          0x7fff

#define   SUPER80_ROM_U26_START_ADDR    0xc000
#define   SUPER80_ROM_U26_END_ADDR      0xcfff

#define   SUPER80_ROM_U33_START_ADDR    0xd000
#define   SUPER80_ROM_U33_END_ADDR      0xdfff

#define   SUPER80_ROM_U42_START_ADDR    0xe000
#define   SUPER80_ROM_U42_END_ADDR      0xefff

#define   SUPER80_VIDEO_ROWS        16
#define   SUPER80_VIDEO_COLS        32

#define   SUPER80_VIDEO_CHARS       (SUPER80_VIDEO_ROWS * SUPER80_VIDEO_COLS)

#define   SUPER80_FONT_WIDTH        6   // actually 5, but leave a space
#define   SUPER80_FONT_HEIGHT       8

extern unsigned char g_super80_U26_ROM[4096];
extern unsigned char g_super80_U33_ROM[4096];
extern unsigned char g_super80_U42_ROM[4096];

extern unsigned char g_2513ROM[2048];


static EmulatorInfo g_emulatorInfo = 
{
  "super80",                      // command line option
  "Super-80",                     // short name
  "Dick Smith Super80",           // long name

  {
    INFO_CPU(2, SUPER80_ROM_U26_START_ADDR),

    INFO_MAIN_RAM(SUPER80_RAM_START_ADDR, 48, 8, SUPER80_ROM_U26_START_ADDR / 1024),


    //INFO_VIDEO_MEMORY_MAPPED("super80", 0xf000, 0xf1ff,
    INFO_VIDEO_EXTERNAL("super80", SUPER80_VIDEO_CHARS, \
                         SUPER80_VIDEO_COLS, SUPER80_VIDEO_ROWS, \
                         SUPER80_FONT_WIDTH, SUPER80_FONT_HEIGHT, \
                         64, &g_charGen_Signetics2513, nullptr),

    INFO_MONITOR(6.0, 4.0, 3.0, ePAL),        

    INFO_IO_PORT_RW(0xf8, 0xfb, 1),    // PIO
    INFO_IO_PORT_RW(0xf1, 0xf1, 2),    // video page
    INFO_IO_PORT_RW(0xf2, 0xf2, 3),    // input
    INFO_IO_PORT_RW(0xf0, 0xf0, 4),    // output

    INFO_ROM(SUPER80_ROM_U26_START_ADDR, g_super80_U26_ROM),
    //INFO_ROM(SUPER80_ROM_U33_START_ADDR, g_super80_U33_ROM),
    //INFO_ROM(SUPER80_ROM_U42_START_ADDR, g_super80_U42_ROM),

    INFO_END()
  }
};

//////////////////////////////////////////////////////////////////////////////////

const KeyboardScanner::ScanCode keys[8*8] = {
  { "@" }, { "H" }, { "P" },  {  "X" }, { "1"},  {     "9" } ,  {         " " } , {     "REP" } ,
  { "A" }, { "I" }, { "Q" },  {  "Y" }, { "2"},  {     ":" } ,  { "Backspace" } , {   "Shift" } ,
  { "B" }, { "J" }, { "R" },  {  "Z" }, { "3"},  {     ";" } ,  {       "Tab" } , {        0  } ,
  { "C" }, { "K" }, { "S" },  {  "[" }, { "4"},  {     "," } ,  {        "LF" } , { "Control" } ,
  { "D" }, { "L" }, { "T" },  { "\\" }, { "5"},  { "Break" } ,  {    "Return" } , {        0  } ,
  { "E" }, { "M" }, { "U" },  {  "]" }, { "6"},  {     "." } ,  {    "Escape" } , {        0  } ,
  { "F" }, { "N" }, { "V" },  {  "^" }, { "7"},  {     "/" } ,  {    "Delete" } , {        0  } ,
  { "G" }, { "O" }, { "W" },  {  "-" }, { "8"},  {     "0" } ,  {    "Insert" } , {        0  } 
};

const KeyboardScanner::ScanCode shiftedkeys[8*8] = {
  { "`" }, { "h" }, { "p" },  {  "x" }, {  "!"},  {     ")" } ,  {         " " } , {     "REP" } ,
  { "a" }, { "i" }, { "q" },  {  "y" }, { "\""},  {     "*" } ,  { "Backspace" } , {   "Shift" } ,
  { "b" }, { "j" }, { "r" },  {  "z" }, {  "#"},  {     "+" } ,  {       "Tab" } , {        0  } ,
  { "c" }, { "k" }, { "s" },  {  "{" }, {  "$"},  {     "<" } ,  {        "LF" } , { "Control" } ,
  { "d" }, { "l" }, { "t" },  { "\\" }, {  "%"},  { "Break" } ,  {    "Return" } , {        0  } ,
  { "e" }, { "m" }, { "u" },  {  "}" }, {  "&"},  {     ">" } ,  {    "Escape" } , {        0  } ,
  { "f" }, { "n" }, { "v" },  {  "~" }, {  "'"},  {     "/" } ,  {    "Delete" } , {        0  } ,
  { "g" }, { "o" }, { "w" },  {  "=" }, {  "("},  {     "0" } ,  {    "Insert" } , {        0  } 
};

static KeyboardScanner::ScanLayout g_super80Keys = {
  8, 8,
  keys,
  shiftedkeys
};


//////////////////////////////////////////////////////////////////////////////////

class Super80Video : public SingleColourMemoryMappedVideo
{
  public:
    Super80Video(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);
    virtual FontChar GetCharAtPos(int x, int y) override;
};

Super80Video::Super80Video(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : SingleColourMemoryMappedVideo(mainWindow, emulator, options, info)
{
}

FontChar Super80Video::GetCharAtPos(int x, int y)
{
  int pos = y * m_cols + x;

  FontChar ch = m_memory[pos & m_visibleMask];

  // map all chars into 64 chars (0 .. 0x3f)
  ch &= 0x3f;
//  if (ch >= 0x60)
//    return ch - 0x60;
//  else if (ch > 0x40)
//    return ch - 0x40;
  return ch;  
}


//////////////////////////////////////////////////////////////////////////////////

void Super80_Emulator::Instantiate()
{  
  VirtualScreen::AddType<Super80Video>("super80");
}

Super80_Emulator::Super80_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Super80_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  m_videoPage = 0;
  m_videoStartAddr = 0;
  m_videoEndAddr   = SUPER80_VIDEO_CHARS - 1;

  m_led = false;
  m_videoOn = false;
  m_portF0 = 0x00;

  // 0xf  = jumble of chars with super-80 on second line   NO JUMPERS
  // 0xd  = screen full of @                               JUMPER B
  // 0xe  = monitor?                                       JUMPER A   
  m_options = 0xe;
  SetKeyboard(&m_keyboard);
  m_keyboard.Compile(g_super80Keys);

  using namespace std::placeholders;

  // set PIO interrupt handler
  m_pio.SetInterruptHandler(std::bind(&Super80_Emulator::Interrupt, this, _1));

  // set read handler for keyboard
  m_pio.SetReadHandler(1, std::bind(&Super80_Emulator::OnReadKeyboard, this));  

  return true;
}

void Super80_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);
  SetVideoPage(0);
  m_pio.Reset();
  m_pio.SetData(1, 0xff); // keyboard has pull ups
}

uint8_t Super80_Emulator::OnReadKeyboard()
{
  uint8_t kbIn  = m_pio.GetData(0) ^ 0xff;
  uint8_t kbOut = m_keyboard.Read(kbIn);

  //if ((kbIn != 0x00) && (kbOut != 0x00))
  //  cout << "kb: read in " << HEXFORMAT0x2(kbIn) << " returned " << HEXFORMAT0x2(kbOut) << endl;

  return kbOut ^ 0xff;
}

uint8_t Super80_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:                            // 0xf8 - 0xfb PIO
      return m_pio.Read(port & 0x3);   
    case 2:                            // 0xf1  video page
      return m_videoPage;              
    case 3:                            // 0xf2  input
       return m_options << 4;
    //case 4:                          // 0xf0 not used - output
    //  return 0xff;
  }
  cerr << "super80: read port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void Super80_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      m_pio.Write(port & 0x3, data);    // 0xf8 - 0xfb PIO
      return;
    case 2:                             // 0xf1 video page
      SetVideoPage(data);
      return;
    //case 3:                           // 0xf2 not used input
      break;
    case 4:                             // 0xf0 - output
      m_portF0 = data;
      {
        bool newVideoOn = (data & (1 << 2)) != 0;
        if (newVideoOn != m_videoOn) {
          cerr << "super80: video turned " << (newVideoOn ? "on" : "off") << endl;
          m_videoOn = newVideoOn;
          if (m_videoOn)
            CopyToVideo();
        }
      }
      m_led     = (data & (1 << 5)) != 0;   // led
      return;
  }
  cerr << "super80: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}

void Super80_Emulator::SetVideoPage(uint8_t value)
{
  // if value hasn't changed, nothing to do
  if (value == m_videoPage)
    return;

  m_videoPage = value & 0xfe;
  m_videoStartAddr = (m_videoPage << 8);
  m_videoEndAddr   = m_videoStartAddr + SUPER80_VIDEO_CHARS - 1;

  cout << "super80: video page set to " << HEXFORMAT0x4(m_videoStartAddr) << " to " << HEXFORMAT0x4(m_videoEndAddr) << endl;

  // value has changed - copy memory to video
  if (m_videoOn)
    CopyToVideo();
}

void Super80_Emulator::CopyToVideo()
{
  cout << "super80: copy to video from " << HEXFORMAT0x4(m_videoStartAddr) << endl;
  uint16_t addr = m_videoStartAddr;
  for (int i = 0; i < SUPER80_VIDEO_CHARS; ++i) {
    uint8_t data = ReadMemory(addr);
    //cout << "super80: copy to video from " << HEXFORMAT0x4(addr) << " to " << HEXFORMAT0x4(i) << " " << HEXFORMAT0x2(data) << endl;
    m_video->WriteMemoryAtAddress(i, data);
    addr++;
  }
}

void Super80_Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  if (m_videoOn && (addr >= m_videoStartAddr) && (addr <= m_videoEndAddr)) {
    m_video->WriteMemoryAtAddress(addr - m_videoStartAddr, data);
    //cout << "super80: write to video at " << HEXFORMAT0x4(m_videoStartAddr) << " " << HEXFORMAT0x2(data) << endl;
  }
  Z80Emulator::WriteMemory(addr, data);  
}



