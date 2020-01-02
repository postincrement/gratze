#ifndef MODEL4_H_
#define MODEL4_H_

#include "trs80/trs80.h"

class Model4_Emulator : public TRS80Emulator
{
  public:
    Model4_Emulator();
    
    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;
};

#endif // MODEL4_H_