#ifndef DG680_H_
#define DG680_H_

#include "common/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keypar.h"

class DG680_Emulator : public Z80Emulator
{
  public:
    DG680_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);

  protected:  
    Z80PIO m_pio;
};

#endif // DG680_H_