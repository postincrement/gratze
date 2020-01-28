#ifndef Z80_H_
#define Z80_H_

#include "assembler.h"
#include "disassembler.h"

//////////////////////////////////////////////////////////////////

enum class Z80Mode {
  eD,    // direct register addressing
  eRR,   // register to register move
  eDW,   // direct word register addressing
  eR,    // relative addressing
  eO,    // other

  eFunc = XSsembler::eFunc,  // use function for matrix
  eTerm = XSsembler::eTerm   // terminating instruction
};

class Z80Assembler : public Assembler
{
  public:
    Z80Assembler();
    virtual bool ParseLine() override;
    virtual bool IsCommentStart(const char * str, size_t col) override;
};

class Z80Disassembler : public Disassembler
{
  public:
    Z80Disassembler();
    virtual const OpCodeInfo * GetOpcodes() const override;
    virtual bool DecodeInstruction(DisasmInfo & disasm, int & len, unsigned address, const std::vector<uint8_t> & image, unsigned addr, bool & isTerm) override;
};

struct Z80Processor : public ProcessorType
{
  virtual Assembler * CreateAssembler() override
  {
    return new Z80Assembler();
  }

  virtual Disassembler * CreateDisassembler() override
  {
    return new Z80Disassembler();
  }
};

#endif // Z80_H_
