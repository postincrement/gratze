#ifndef XSSEMBLER_H_
#define XSSEMBLER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

#include "common/cmdargs.h"

#define DEFAULT_PAGE_LENGTH   52

class XSsembler
{
  public:
    struct FormatInfo
    {
      int m_lineNumberWidth;  // width of line number
      int m_opcodeByteCount;  // number of bytes in listing file
      int m_symColWidth;      // number of chars for symbol in listing file
      int m_mnemColWidth;     // number of chars for mnemonic column in listing file
      int m_argColWidth;      // number of chars for argument column in listing file
    };

    XSsembler(const FormatInfo & format);

    struct OpCodeInfo
    {
      const char *  m_mnemonic = nullptr;
      int           m_mode = 0;     
      uint8_t       m_opcode = 0;
      uint8_t       m_mask = 0;
      int           m_opLen = 0;
      void *        m_bitInfo = nullptr;
    };

    struct SymbolInfo
    {
      SymbolInfo() = default;

      SymbolInfo(unsigned value)
        : m_value(value)
      {}

      unsigned m_value = 0;
      bool m_used = false;
    };

    enum {
      eModeMask = 0x0ff,
      eFunc     = 0x400,  // use function for matrix
      eTerm     = 0x800   // terminating instruction (ret, jmp etc)
    };

    virtual void AssignSymbol(const std::string & sym, unsigned val);

    typedef std::map<std::string, SymbolInfo> SymbolTable;

  protected:
    FormatInfo m_format;  
    int m_pass;
    SymbolTable m_symbols;
};

class Assembler;
class Disassembler;

struct ProcessorType
{
  virtual Assembler * CreateAssembler() = 0;  
  virtual Disassembler * CreateDisassembler() = 0;  
};



#endif // XSSEMBLER_H_