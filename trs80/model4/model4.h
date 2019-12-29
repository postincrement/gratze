#ifndef MODEL4_H_
#define MODEL4_H_

#include "trs80.h"

class Model4_Emulator : public TRS80Emulator
{
  public:
    Model4_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual int GetDefaultMemorySize_k() const override;
};

#endif // MODEL4_H_