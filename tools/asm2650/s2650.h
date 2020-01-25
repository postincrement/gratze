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
    virtual bool ParseLine() override;
    virtual bool IsCommentStart(const char * str, size_t col) override;

  protected:
    bool ParseExpr(unsigned int & val, const std::string & str, size_t pos, std::string & error);
    bool ParseByteExpr(uint8_t & val, const std::string & str, size_t pos, std::string & error);
};

#endif // S2650_H_
