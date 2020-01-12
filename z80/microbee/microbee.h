#ifndef MICROBEE_H_
#define MICROBEE_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"

class Microbee_Emulator : public Z80Emulator
{
  public:
    Microbee_Emulator();

    void Init();

    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);

    uint16_t OnKeyboardScan();

  protected:  
    KeyboardScanner m_keyboard;
    Z80PIO m_pio;
    uint16_t m_prevKeyboardCode;
};

#endif // DG680_H_