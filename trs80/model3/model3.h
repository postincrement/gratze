#ifndef MODEL3_H_
#define MODEL3_H_

#include "trs80.h"

class Model3_Emulator : public TRS80Emulator
{
  public:
    Model3_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual int GetDefaultRAMSize_k() const override;
};

#endif // MODEL3_H_

