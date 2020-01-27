#ifndef DISASSEMBLER_H_
#define DISASSEMBLER_H_

#include <set>

#include "cmdargs.h"
#include "assembler.h"

class Disassembler  : public XSsembler
{
  public:
    Disassembler(const FormatInfo & format);

    virtual bool Open(const CommandLineArgs & args, const std::string & fn);
    virtual bool Run();

    void Matrix();

    typedef std::array<std::string, 4> DisasmInfo;   // symbol, mnem, arg, comments

    std::string CreateSymbol(unsigned addr);
  
    virtual const OpCodeInfo * GetOpcodes() const = 0;
    virtual bool DecodeInstruction(DisasmInfo & disasm, int & len, unsigned address, const std::vector<uint8_t> & image, unsigned addr, bool & isTerm) = 0;

    typedef std::map<unsigned, std::string> ReverseSymbolTable;

  protected:
    std::vector<uint8_t> m_image;  
    unsigned m_origin;
    std::set<unsigned> m_forceDb;
    ReverseSymbolTable m_reverseSymbols;
};

#endif // DISASSEMBLER_H_
