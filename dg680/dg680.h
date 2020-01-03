#ifndef DG680_H_
#define DG680_H_

#include "config.h"
#include "cpu/z80emulator.h"
#include "options.h"

class DG680_Emulator : public Z80Emulator
{
  public:
    void Init();
    DG680_Emulator();

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    void OnPIOInterrupt(uint8_t vector);

    void OnKeyDown(const SDL_Keysym & keysym);
};

#endif // DG680_H_