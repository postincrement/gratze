#ifndef MICROBEE_H_
#define MICROBEE_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"
#include "devices/synertek6545.h"

class Microbee_Emulator : public Z80Emulator
{
  public:
    Microbee_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);

    bool OnKeyboardScan(bool doUpdate, uint16_t & addr);

    void OnSetScreenSize(int cols, int rows);
    void OnSetVideoStartAddress(uint16_t addr);
    void OnSetCursorAddress(uint16_t addr);
    void OnSetCursorShape(uint8_t start, uint8_t end, int blinkRate);

    bool ScanKeyboard(uint16_t & addr);

  protected:  
    KeyboardScanner m_keyboard;
    Z80PIO m_pio;
    Synertek6545 m_crtc;
    uint16_t m_prevKeyboardCode;
};

#endif // DG680_H_