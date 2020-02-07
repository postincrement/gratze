#ifndef S2650_H_
#define S2650_H_

#include "assembler.h"
#include "disassembler.h"

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

  eTerm = XSsembler::eTerm   // terminating instruction
};

class S2650Assembler : public Assembler
{
  public:
    S2650Assembler();
    virtual bool ParseLine() override;
    virtual bool IsCommentStart(const char * str, size_t col) override;

  protected:
    bool ParseIndexExpr(uint8_t & reg, unsigned & addr, const std::string & arg, const std::string & str);
};

class S2650Disassembler : public Disassembler
{
  public:
    S2650Disassembler();
    virtual const OpCodeInfo * GetOpcodes() const override;
    virtual bool DecodeInstruction(DisasmInfo & disasm, int & len, unsigned address, const std::vector<uint8_t> & image, unsigned addr, bool & isTerm) override;
};

struct S2650Processor : public ProcessorType
{
  virtual Assembler * CreateAssembler() override
  {
    return new S2650Assembler();
  }

  virtual Disassembler * CreateDisassembler() override
  {
    return new S2650Disassembler();
  }
};

#endif // S2650_H_
