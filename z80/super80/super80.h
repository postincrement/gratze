#ifndef SUPER80_H_
#define SUPER80_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"

class Super80_Emulator : public Z80Emulator
{
  public:
    Super80_Emulator();

    void Init();

    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);
    uint8_t OnReadKeyboard();

    void OnKeyDown(const SDL_Keysym & keysym);
    void OnKeyUp(const SDL_Keysym & keysym);

    void SetVideoPage(uint8_t value);
    void CopyToVideo();

    virtual void WriteMemory(uint16_t, uint8_t data) override;

  protected:  
    Z80PIO m_pio;
    KeyboardScanner m_kbScanner;

    uint16_t m_videoPage;
    uint16_t m_videoStartAddr;
    uint16_t m_videoEndAddr;

    bool m_led;
    bool m_videoOn;
    uint8_t m_portF0;
    uint8_t m_options;
};

#endif // DG680_H_