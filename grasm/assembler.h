#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

#include "common/cmdargs.h"
#include "xssembler.h"

#define DEFAULT_PAGE_LENGTH   52

class Assembler : public XSsembler
{
  public:
    struct Token
    {
      enum class Type
      {
        eEOF,
        eInteger,
        eOp,
        eString,
        eSymbol,
        eCurrentAddr
      };

      Type m_type;
      std::string m_lexeme;
      unsigned m_value;

      Type Parse(const std::string & str, size_t & pos);
    };

    Assembler(const FormatInfo & format);

    virtual bool Open(const CommandLineArgs & args, const std::string & fn);
    virtual bool Parse();

    virtual bool ParseLine() = 0;
    virtual bool IsCommentStart(const char * str, size_t col) = 0;

    virtual int GetLineCount() const
    { return m_lineNumber; }

    virtual std::string GetNextWord();
    virtual std::string GetRestOfLine();

    virtual std::string GetListing();

    virtual std::string GetError();
    virtual bool ParseError(const std::string & str, const std::string & arg = "");

    virtual bool WriteBinary();
    virtual bool WriteListing();

    static std::string TrimRight(const std::string & str);
    static std::string TrimLeft(const std::string & str);
    static std::string Trim(const std::string & str);


    std::string GetPageHeader(int & row, int & page);
    bool ParseExpr(unsigned int & val, const std::string & str, size_t pos, std::string & error);
    bool ParseByteExpr(uint8_t & val, const std::string & str, size_t pos, std::string & error);
    bool ResolveIntegerValue(unsigned & val, const Token & token, unsigned int currentAddr, Assembler::SymbolTable & symbolTable);

  protected:   
    int m_lineNumber;
    unsigned m_address;
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
