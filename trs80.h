#ifndef TRS80_H_
#define TRS80_H_

#include "emulator.h"

#include <set>

class TRS80Emulator : public Emulator
{
  public:
    TRS80Emulator();

    // overrides from Emulator
    virtual bool Open(int argc, char *argv[]) override;
    virtual bool Start(uint16_t addr) override;

    virtual uint8_t RdZ80(register uint16_t addr) override;
    virtual void WrZ80(register uint16_t addr,register uint8_t value) override;

    virtual uint8_t InZ80(register uint16_t port) override;
    virtual void OutZ80(register uint16_t port, register uint8_t value) override;

    virtual void OnKeyDown(SDL_Keysym & keysym) override;
    virtual void OnKeyUp(SDL_Keysym & keysym) override;

    // new functions
    uint8_t ReadRAM(uint16_t);
    void WriteRAM(uint16_t, uint8_t);

    uint8_t ReadROM(uint16_t);

    uint8_t ReadVideo(uint16_t);
    void WriteVideo(uint16_t, uint8_t);

    uint8_t ReadIO(uint16_t addr);
    void WriteIO(uint16_t addr, uint8_t val);

    uint8_t ReadKeyboard(uint16_t addr);

  protected:  
    std::unique_ptr<MemoryMappedVideo::Font> m_font;

    uint8_t m_rom[12*1024];
    uint8_t m_ram[48*1024];
    uint8_t m_videoRAM[1*1024];
    uint8_t m_kbData[8];
    uint8_t m_shiftDown;
    std::set<uint8_t> m_ignoreUps;
};

#endif // TRS80_H_
