#ifndef DG680_H_
#define DG680_H_

#include "config.h"
#include "cpu/z80emulator.h"
#include "options.h"

class DG680_Emulator : public Z80Emulator
{
  public:
    DG680_Emulator();

    virtual bool Open(const Options & options) override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

  protected:
    uint8_t m_dgosRAM[2048];  
};

#endif // DG680_H_