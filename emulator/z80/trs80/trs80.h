#ifndef TRS80_H_
#define TRS80_H_

#include "common/config.h"
#include "z80/z80emulator.h"
#include "devices/fdc.h"
#include "common/cassette.h"
#include "devices/keyscan.h"
#include "video/virtual_screen.h"
#include "video/chargen_mcm6674.h"
#include "src/options.h"

#define   MODEL1_CLOCK_SPEED      1.774
#define   MODEL3_CLOCK_SPEED      2.048
#define   MODEL4_CLOCK_SPEED      4.000

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

#define   MODEL1_SCREEN_WIDTH_CHARS      64
#define   MODEL1_SCREEN_HEIGHT_CHARS     16

#define   MODEL3_SCREEN_WIDTH_CHARS      64
#define   MODEL3_SCREEN_HEIGHT_CHARS     16

#define   MODEL4_SCREEN_WIDTH_CHARS      64
#define   MODEL4_SCREEN_HEIGHT_CHARS     16

#define   MODEL1_SCREEN_WIDTH_PIXELS     (MODEL1_SCREEN_WIDTH_CHARS*MODEL1_FONT_WIDTH)
#define   MODEL1_SCREEN_HEIGHT_PIXELS    (MODEL1_SCREEN_HEIGHT_CHARS*MODEL1_FONT_HEIGHT)

#define   MODEL3_SCREEN_WIDTH_PIXELS     (MODEL3_SCREEN_WIDTH_CHARS*MODEL3_FONT_WIDTH)
#define   MODEL3_SCREEN_HEIGHT_PIXELS    (MODEL3_SCREEN_HEIGHT_CHARS*MODEL3_FONT_HEIGHT)

#define   MODEL4_SCREEN_WIDTH_PIXELS     (MODEL4_SCREEN_WIDTH_CHARS*MODEL4_FONT_WIDTH)
#define   MODEL4_SCREEN_HEIGHT_PIXELS    (MODEL4_SCREEN_HEIGHT_CHARS*MODEL4_FONT_HEIGHT)

class TRS80Video : public MonoMemoryMappedScreen
{
  public:
    TRS80Video(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);
    virtual void WriteMemoryAtAddress(int addr, uint8_t ch) override;
    void Set32Col(bool val);

  protected:
    bool m_32Col;  
};

class TRS80Emulator : public Z80Emulator
{
  public:
    TRS80Emulator(const EmulatorInfo * info);

    void Instantiate() override;

    // overrides from Emulator
    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadNull(uint16_t) const override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    // new functions
    void WriteVideoChar(unsigned int offset, uint8_t ch);

    uint8_t ReadKeyboard(uint16_t addr) const;

    uint8_t ReadPrinter(uint16_t addr) const;
    void WritePrinter(uint16_t addr, uint8_t val);

    void WriteDrvSel(uint16_t, uint8_t val);
    uint8_t ReadDrvSel(uint16_t) const;

    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly) override;;
    void InitFDC();
    uint8_t ReadFDC(uint16_t addr) const;
    void WriteFDC(uint16_t addr, uint8_t val);
    void FDCInterrupt(bool v);
    void RTCInterrupt();

    uint8_t ReadInterrupt(uint16_t) const;

    void WriteFF(register uint16_t, register uint8_t val);
    uint8_t ReadFF(register uint16_t);

    void WriteFx(register uint16_t, register uint8_t val);
    uint8_t ReadFx(register uint16_t);

    static bool CreatePixelFont(const Config::Font & fontInfo, std::vector<uint8_t> & fontData);

  protected:  
    bool m_fdcEnabled = false;
    mutable int m_fdcPending = false;
    uint8_t m_drvSel;
    mutable std::unique_ptr<WD_FDC> m_fdc;

    bool m_rtcEnabled = false;
    mutable bool m_rtcPending = false;
    std::chrono::system_clock::time_point m_rtcTimer;

    bool m_cassette2 = false;
    bool m_cassetteMotor = false;
    bool m_cassetteTrigger = false;
    std::unique_ptr<VirtualCassetteFile> m_cassette;

    bool m_32Col = false;
};

#endif // TRS80_H_
