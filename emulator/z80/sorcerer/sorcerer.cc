

#include <iostream>
#include <iomanip>

#include "common/misc.h"
#include "z80/sorcerer/sorcerer.h"
#include "video/virtual_screen.h"
#include "z80/sorcerer/chargen_sorcerer.h"


using namespace std;

#define   SORCERER_RAM_START_ADDR       0x0000
#define   SORCERER_RAM_END_ADDR         0xbfff

#define   SORCERER_ROMPAC_START_ADDR    0xc000
#define   SORCERER_ROMPAC_END_ADDR      0xdfff

#define   SORCERER_MONITOR_START_ADDR   0xe000
#define   SORCERER_MONITOR_END_ADDR     0xefff

#define   SORCERER_VSCRATCH_START_ADDR  0xf000
#define   SORCERER_VSCRATCH_END_ADDR    0xf07f

#define   SORCERER_VIDEO_START_ADDR     0xf080
#define   SORCERER_VIDEO_END_ADDR       0xf7ff

#define   SORCERER_ASCII_ROM_START_ADDR 0xf800
#define   SORCERER_ASCII_ROM_END_ADDR   0xfbff

#define   SORCERER_GRAPHICS_START_ADDR  0xfc00
#define   SORCERER_GRAPHIC_END_ADDR     0xfdff

#define   SORCERER_USER_GRAPHICS_START_ADDR  0xfe00
#define   SORCERER_USER_GRAPHIC_END_ADDR     0xffff

#define   SORCERER_VIDEO_ROWS        30
#define   SORCERER_VIDEO_COLS        64

#define   SORCERER_VIDEO_CHARS       (SORCERER_VIDEO_ROWS * SORCERER_VIDEO_COLS)

#define   SORCERER_FONT_WIDTH        8
#define   SORCERER_FONT_HEIGHT       8

extern unsigned char g_sorcerer_Monitor_ROM[4096];
//extern unsigned char g_super80_U33_ROM[4096];
//extern unsigned char g_super80_U42_ROM[4096];

//extern unsigned char g_2513ROM[2048];


INFO_START(sorcerer)
{
  INFO_CPU(2, SORCERER_MONITOR_START_ADDR),

  INFO_MAIN_RAM(SORCERER_RAM_START_ADDR, 16, 8, SORCERER_RAM_END_ADDR / 1024),

  INFO_RAM(SORCERER_VSCRATCH_START_ADDR,      SORCERER_VSCRATCH_END_ADDR),
  INFO_RAM(SORCERER_GRAPHICS_START_ADDR,      SORCERER_GRAPHIC_END_ADDR),
  INFO_RAM(SORCERER_USER_GRAPHICS_START_ADDR, SORCERER_USER_GRAPHIC_END_ADDR),

  INFO_SCREEN_MEMORY_MAPPED_FIXED("sorcerer", \
                        SORCERER_VIDEO_START_ADDR, SORCERER_VIDEO_END_ADDR, \
                        SORCERER_VIDEO_COLS, SORCERER_VIDEO_ROWS, \
                        SORCERER_FONT_WIDTH, SORCERER_FONT_HEIGHT, \
                        128, \
                        &g_charGen_Sorcerer, \
                        nullptr),

  INFO_MONITOR(6.0, 4.0, 3.0, ePAL),

  INFO_IO_PORT_RW(0xfe, 0xfe, 1),    // port

  INFO_ROM(SORCERER_MONITOR_START_ADDR,        g_sorcerer_Monitor_ROM),
  INFO_ROM_DATA(SORCERER_ASCII_ROM_START_ADDR, SORCERER_ASCII_ROM_END_ADDR, g_charGen_Sorcerer.m_data),
}
INFO_END(sorcerer);

static EmulatorInfo g_emulatorInfo =
{
  "sorcerer",                 // command line option
  "sorcerer",                 // short name
  "Exidy Sorcerer",           // long name

  INFO_INSERT(sorcerer)
};

//////////////////////////////////////////////////////////////////////////////////

using SorcererVideo = SingleColourMemoryMappedScreen;

void Sorcerer_Emulator::Instantiate()
{
  MemoryMappedScreen::AddType<SorcererVideo>("sorcerer");
}

Sorcerer_Emulator::Sorcerer_Emulator()
  : Z80Emulator(&g_emulatorInfo)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Sorcerer_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  return true;
}

void Sorcerer_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);
  //SetVideoPage(0);
  //m_pio.Reset();
  //m_pio.SetData(1, 0xff); // keyboard has pull ups
}

uint8_t Sorcerer_Emulator::OnReadKeyboard()
{
//  uint16_t kbIn  = m_pio.GetData(0) ^ 0xff;
//  uint8_t kbOut = m_keyboard->Read(kbIn);
  //uint8_t kbOut = 0x00;

  //if ((kbIn != 0x00) && (kbOut != 0x00))
  //  cout << "kb: read in " << HEXFORMAT0x2(kbIn) << " returned " << HEXFORMAT0x2(kbOut) << endl;

  return 0xff;
}

uint8_t Sorcerer_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port)
{
  switch (info.m_id) {
    case 1:                            // 0xfe 
      return 0x3f;
  /*
    case 2:                            // 0xf1  video page
      return m_videoPage;
    case 3:                            // 0xf2  input
       return m_options << 4;
    case 4:                          // 0xf0 not used - output
      return 0xff;
  */
  }
  cerr << "sorcerer: read port " << HEXFORMAT0x2(port) << endl;
  return 0xff;
}

void Sorcerer_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
  switch (info.m_id) {
    case 1:
      //
      return;
/*
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
  */
  }
  cerr << "sorcerer: write port " << HEXFORMAT0x2(port) << " " << HEXFORMAT0x2(data) << endl;
}



