#ifndef DG640_H_
#define DG640_H_

#include "virtual_screen.h"
#include "chargen_mcm6574.h"

#define   DG640_VIDEO_RAM_SIZE_K    2

#define   DG640_FONT_WIDTH          8
#define   DG640_FONT_HEIGHT         16

#define   DG640_SCREEN_COLS         64
#define   DG640_SCREEN_ROWS         16

#define   DG640_SCREEN_WIDTH_PIXELS     (DG640_SCREEN_COLS * DG640_FONT_WIDTH)
#define   DG640_SCREEN_HEIGHT_PIXELS    (DG640_SCREEN_ROWS * DG640_FONT_HEIGHT)

#define   DG640_VIRTUAL_FONT_CHARS  (128*4)       // chars, inverted chars, graphics

extern void CreateDG640PixelData(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

#define DG640_VIDEO_DRIVER(addr) \
  INFO_VIDEO_MEMORY_MAPPED("dg640", \
                           addr, addr + DG640_VIDEO_RAM_SIZE_K * 1024 - 1, \
                           DG640_SCREEN_COLS, DG640_SCREEN_ROWS, \
                           DG640_FONT_WIDTH, DG640_FONT_HEIGHT, \
                           DG640_VIRTUAL_FONT_CHARS, \
                           &g_charGen_MotorolaMCM6574, \
                           &DG640::CreatePixelFont), \
  INFO_MONITOR(12.0, 4.0, 3.0, ePAL)

class DG640 : public SingleColourMemoryMappedVideo
{
  public:
    DG640(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);

    virtual void WriteMemoryAtAddress(int addr, uint8_t ch) override;
    virtual uint8_t ReadMemoryAtAddress(int addr) const override;

    static bool CreatePixelFont(const Options & options, const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

    FontChar GetCharAtAddress(int addr) const override;
};


#endif // DG640_H_