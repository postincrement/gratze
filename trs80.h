#ifndef TRS80_H_
#define TRS80_H_

#include "emulator.h"
#include "fdc.h"
#include "options.h"

class TRS80Emulator : public Emulator
{
  public:
    TRS80Emulator();

    // overrides from Emulator
    virtual bool Open(const Options & options) override;
    virtual bool Start(uint16_t addr) override;

    virtual uint8_t RdZ80(register uint16_t addr) override;
    virtual void WrZ80(register uint16_t addr,register uint8_t value) override;

    virtual uint8_t InZ80(register uint16_t port) override;
    virtual void OutZ80(register uint16_t port, register uint8_t value) override;

    virtual void OnKeyDown(SDL_Keysym & keysym) override;
    virtual void OnKeyUp(SDL_Keysym & keysym) override;

    virtual void Poll() override;

    // new functions
    uint8_t ReadRAM(uint16_t);
    void WriteRAM(uint16_t, uint8_t);

    uint8_t ReadROM(uint16_t);

    uint8_t ReadVideo(uint16_t);
    void WriteVideo(uint16_t, uint8_t);

    uint8_t ReadIO(uint16_t addr);
    void WriteIO(uint16_t addr, uint8_t val);

    uint8_t ReadKeyboard(uint16_t addr);

    uint8_t ReadMemIO(uint16_t addr);
    void WriteMemIO(uint16_t addr, uint8_t val);

    void WritePrinterFDC(uint16_t addr, uint8_t val);
    uint8_t ReadPrinterFDC(uint16_t addr);

    uint8_t ReadPrinter(uint16_t addr);
    void WritePrinter(uint16_t addr, uint8_t val);

    void WriteDrvSel(uint16_t, uint8_t val);
    uint8_t ReadDrvSel(uint16_t);

    void InitFDC();
    uint8_t ReadFDC(uint16_t addr);
    void WriteFDC(uint16_t addr, uint8_t val);
    void FDCInterrupt();

    uint8_t ReadInterrupt(uint16_t);

  protected:  
    std::unique_ptr<MemoryMappedVideo::Font> m_font;
    std::vector<uint8_t> m_fontData;

    uint8_t m_rom[12*1024];
    uint8_t m_ram[48*1024];
    uint8_t m_videoRAM[1*1024];
    uint8_t m_kbData[8];
    uint8_t m_shiftDown;
    std::vector<uint8_t> m_data;

    bool m_fdcPending;
    bool m_rtcPending;
    std::chrono::system_clock::time_point m_rtcTimer;
};

#endif // TRS80_H_
