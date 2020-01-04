#ifndef DG680_H_
#define DG680_H_

#include "config.h"
#include "cpu/z80emulator.h"
#include "options.h"
#include "devices/z80pio.h"

class DG680_Emulator : public Z80Emulator
{
  public:
    DG680_Emulator();

    void Init();

    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);

    void OnKeyDown(const SDL_Keysym & keysym);

  protected:  
    Z80PIO m_pio;
};

#endif // DG680_H_