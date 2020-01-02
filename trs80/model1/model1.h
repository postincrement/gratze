#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80/trs80.h"

class Model1_Emulator : public TRS80Emulator
{
  public:
    Model1_Emulator(EmulatorInfo * info);

    virtual bool Open(const Options & options) override;

    virtual uint8_t ReadIOMemory(int id, uint16_t addr) override;
    virtual void WriteIOMemory(int id, uint16_t addr, uint8_t val) override;

  protected:
    bool m_withEI;    
};

class Model1Level1_Emulator : public Model1_Emulator
{
  public:
    Model1Level1_Emulator();
};

class Model1Level2_Emulator : public Model1_Emulator
{
  public:
    Model1Level2_Emulator();
};
#endif // MODEL1_H_