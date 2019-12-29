#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80.h"

class Model1Level1_Emulator : public TRS80Emulator
{
  public:
    Model1Level1_Emulator();

    virtual std::string GetTitle() const  override;

    virtual int GetDefaultMemorySize_k() const override;
};

class Model1Level2_Emulator : public TRS80Emulator
{
  public:
    Model1Level2_Emulator();

    virtual std::string GetTitle() const override;

    virtual int GetDefaultMemorySize_k() const override;
};
#endif // MODEL1_H_