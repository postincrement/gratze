#ifndef S2650_H_
#define S2650_H_

#include "assembler.h"

//////////////////////////////////////////////////////////////////

class S2650Assembler : public Assembler
{
  public:
    virtual bool ParseLine(const std::string & line_) override;
};

#endif // S2650_H_
