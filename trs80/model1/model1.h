#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80.h"

class Model1_Emulator : public TRS80Emulator
{
  public:
    Model1_Emulator();

    virtual void WrZ80(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t RdZ80(register uint16_t addr) override;
};

class Model1Level1_Emulator : public Model1_Emulator
{
  public:
    Model1Level1_Emulator();

    virtual std::string GetTitle() const  override;

    virtual int GetDefaultRAMSize_k() const override;
};

class Model1Level2_Emulator : public Model1_Emulator
{
  public:
    Model1Level2_Emulator();

    virtual std::string GetTitle() const override;

    virtual int GetDefaultRAMSize_k() const override;
};
#endif // MODEL1_H_