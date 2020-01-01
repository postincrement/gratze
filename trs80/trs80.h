#ifndef TRS80_H_
#define TRS80_H_

#include "config.h"
#include "cpu/z80emulator.h"
#include "magmedia/fdc.h"
#include "magmedia/cassette.h"
#include "options.h"

#define   MODEL1_ROM_START_ADDR    0x0000
#define   MODEL1_L1_ROM_END_ADDR   0x0fff
#define   MODEL1_L2_ROM_END_ADDR   0x2fff
#define   MODEL3_ROM_END_ADDR      0x37ff

#define   MODEL1_MEMIO_START_ADDR  0x3000
#define   MODEL1_MEMIO_END_ADDR    0x37ff

#define   MODEL1_KB_START_ADDR     0x3800
#define   MODEL1_KB_END_ADDR       0x3bff

#define   MODEL1_VIDEO_START_ADDR  0x3c00
#define   MODEL1_VIDEO_END_ADDR    0x3fff

#define   MODEL1_RAM_START_ADDR    0x4000
#define   MODEL1_RAM_END_ADDR      0xffff

#define   MODEL1_FONT_HEIGHT   12    // must be divisble by 3
#define   MODEL1_FONT_WIDTH    6     // must be divisible by 2

#define   MODEL3_FONT_HEIGHT   12    // must be divisble by 3
#define   MODEL3_FONT_WIDTH    8     // must be divisible by 2

#define   MODEL4_FONT_HEIGHT   12    // must be divisble by 3
#define   MODEL4_FONT_WIDTH    8     // must be divisible by 2

#define   MODEL1_SCREEN_WIDTH     (64*MODEL1_FONT_WIDTH)
#define   MODEL1_SCREEN_HEIGHT    (16*MODEL1_FONT_HEIGHT)

#define   MODEL3_SCREEN_WIDTH     (64*MODEL3_FONT_WIDTH)
#define   MODEL3_SCREEN_HEIGHT    (16*MODEL3_FONT_HEIGHT)

#define   MODEL4_SCREEN_WIDTH     (64*MODEL4_FONT_WIDTH)
#define   MODEL4_SCREEN_HEIGHT    (16*MODEL4_FONT_HEIGHT)

class TRS80Emulator : public Z80Emulator
{
  public:
    TRS80Emulator();

    // overrides from Emulator
    virtual bool Open(const Options & options) override;
    virtual bool Start(int addr = -1) override;

    virtual uint8_t ReadNull(uint16_t) override;

    virtual uint8_t InZ80(register uint16_t port) override;
    virtual void OutZ80(register uint16_t port, register uint8_t value) override;

    virtual void OnKeyDown(SDL_Keysym & keysym) override;
    virtual void OnKeyUp(SDL_Keysym & keysym) override;

    virtual bool Poll() override;

    // new functions
    uint8_t ReadIO(uint16_t addr);
    void WriteIO(uint16_t addr, uint8_t val);

    uint8_t ReadKeyboard(uint16_t addr);

    uint8_t ReadMemIO(uint16_t addr);
    void WriteMemIO(uint16_t addr, uint8_t val);

    uint8_t ReadPrinter(uint16_t addr);
    void WritePrinter(uint16_t addr, uint8_t val);

    void WriteDrvSel(uint16_t, uint8_t val);
    uint8_t ReadDrvSel(uint16_t);

    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly) override;;
    void InitFDC();
    uint8_t ReadFDC(uint16_t addr);
    void WriteFDC(uint16_t addr, uint8_t val);
    void FDCInterrupt();

    uint8_t ReadInterrupt(uint16_t);

    void WriteFF(register uint16_t, register uint8_t val);
    uint8_t ReadFF(register uint16_t);

    void WriteFx(register uint16_t, register uint8_t val);
    uint8_t ReadFx(register uint16_t);

  protected:  
    void CreateFontData(int width, int height, uint8_t * fontData);

    uint8_t m_kbData[8];
    uint8_t m_shiftDown;
    std::vector<uint8_t> m_data;

    bool m_fdcEnabled;
    bool m_fdcPending;
    uint8_t m_drvSel;
    std::unique_ptr<WD_FDC> m_fdc;

    bool m_rtcEnabled;
    bool m_rtcPending;
    std::chrono::system_clock::time_point m_rtcTimer;

    bool m_cassette2;
    bool m_cassetteMotor;
    bool m_cassetteTrigger;
    std::unique_ptr<VirtualCassetteFile> m_cassette;
};

#endif // TRS80_H_
