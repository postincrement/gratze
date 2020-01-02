#ifndef MODEL3_H_
#define MODEL3_H_

#include "trs80/trs80.h"
#include "trs80/model1/model1.h"

class Model3_Emulator : public Model1_Emulator
{
  public:
    Model3_Emulator();
    
    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;
};

#endif // MODEL3_H_

