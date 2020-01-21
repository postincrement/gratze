#ifndef S2650_H_
#define S2650_H_

#include "assembler.h"

//////////////////////////////////////////////////////////////////
enum class S2650Mode {
  eZ,    // register addressing
  eI,    // immediate addressing
  eR,    // relative addressing
  eA,    // absolute addressing (non-branch)
  eB,    // absolute addressing (branch)
  eE,    // miscellaneous instructions

  eZa,    // register addressing using arg
  eIn,    // immediate addressing with no arg
};

class S2650Assembler : public Assembler
{
  public:
    virtual bool ParseLine(const std::string & line_) override;
};

#endif // S2650_H_
