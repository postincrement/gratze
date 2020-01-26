#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

#include "cmdargs.h"

#define DEFAULT_PAGE_LENGTH   52

struct OpCodeInfo
{
  const char * m_mnemonic;
  int          m_mode;   // 0 = R0, 1 = immediate, 2 = rel, 3 = abs  
  uint8_t      m_opcode;
  int          m_opLen;
};

class Assembler
{
  public:
    Assembler();

    virtual bool Open(const CommandLineArgs & args, const std::string & fn);
    virtual bool Parse();

    virtual bool ParseLine() = 0;
    virtual bool IsCommentStart(const char * str, size_t col) = 0;

    virtual int GetLineCount() const
    { return m_lineNumber; }

    virtual std::string GetNextWord();
    virtual std::string GetRestOfLine();

    virtual void AssignSymbol(const std::string & sym, unsigned val);

    virtual std::string GetListing();

    virtual std::string GetError();
    virtual bool ParseError(const std::string & str, const std::string & arg = "");

    virtual bool WriteBinary();
    virtual bool WriteListing();

    static std::string TrimRight(const std::string & str);
    static std::string TrimLeft(const std::string & str);
    static std::string Trim(const std::string & str);

    struct SymbolInfo
    {
      SymbolInfo() = default;

      SymbolInfo(unsigned value)
        : m_value(value)
      {}

      unsigned m_value = 0;
      bool m_used = false;
    };

    std::string GetPageHeader(int & row, int & page);

    typedef std::map<std::string, SymbolInfo> SymbolTable;

  protected:   
    int m_lineNumber;
    unsigned m_address;
    int m_pass;
    int m_errorCount;

    std::string m_line;
    size_t m_pos;
    std::string m_error;

    struct ListingInfo
    {
      ListingInfo(unsigned addr = 0)
        : m_addr(addr)
        , m_spaceLen(0)
        , m_equ(false)
      { }    

      unsigned m_addr;
      std::vector<uint8_t> m_ops;
      int m_spaceLen;
      bool m_equ;
    };

    std::vector<std::string> m_lines;
    std::map<int, ListingInfo> m_listings;

    SymbolTable m_symbols;
    bool m_includeSymbols = false;

    std::string m_sourceDir;
    std::string m_basename;

    std::string m_sourceFn;
    std::string m_binaryFn;
    std::string m_listingFn;
    std::string m_symFn;
    unsigned m_pageLength = DEFAULT_PAGE_LENGTH;

    std::ifstream m_file;

    std::string m_symbol;
    std::string m_op;
    std::string m_value;
};

#endif // ASSEMBLER_H_
