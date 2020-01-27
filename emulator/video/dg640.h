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
  INFO_SCREEN_MEMORY_MAPPED_FIXED("dg640", \
                           addr, addr + DG640_VIDEO_RAM_SIZE_K * 1024 - 1, \
                           DG640_SCREEN_COLS, DG640_SCREEN_ROWS, \
                           DG640_FONT_WIDTH, DG640_FONT_HEIGHT, \
                           DG640_VIRTUAL_FONT_CHARS, \
                           &g_charGen_MotorolaMCM6574, \
                           &DG640::CreatePixelFont), \
  INFO_MONITOR(12.0, 4.0, 3.0, ePAL)

class DG640 : public SingleColourMemoryMappedScreen
{
  public:
    DG640(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    // override to ensure correct operation of attribute memory
    virtual void WriteMemoryAtAddress(int addr, uint8_t ch) override;
    virtual uint8_t ReadMemoryAtAddress(int addr) const override;

    // override to allow graphics chars to use fonr
    FontChar GetCharAtLoc(int addr) const override;

    // create font with graphics chars
    static bool CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData);
};


#endif // DG640_H_