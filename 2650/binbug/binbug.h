#ifndef SYSTEM_2650_H
#define SYSTEM_2650_H

#include "cpu/2650emulator.h"

class BINBUG_2650 : public S2650Emulator
{
  public:
    BINBUG_2650();

    virtual bool Open(const Options & options) override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;
};

#endif // SYSTEM_2650_H



