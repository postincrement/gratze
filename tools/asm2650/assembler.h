#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>

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

    virtual bool Open(const std::string & fn);
    virtual bool Parse();

    virtual bool ParseLine(const std::string & line_) = 0;

    virtual int GetLineCount() const
    { return m_lineNumber; }

    virtual std::string GetNextWord();

    virtual void AssignSymbol(const std::string & sym, unsigned val);
    virtual void AssignSymbol(const std::string & sym, const std::string & val);

    virtual std::string GetListing();

    virtual std::string GetError();
    virtual void ParseError(const std::string & str, const std::string & arg = "");

    virtual bool WriteBinary();
    virtual bool WriteListing();

    static std::string TrimRight(const std::string & str);
    static std::string TrimLeft(const std::string & str);
    static std::string Trim(const std::string & str);

  protected:   
    int m_lineNumber;
    unsigned m_address;

    std::string m_trimmedLine;
    std::string m_line;
    std::string m_error;

    struct ListingInfo
    {
      ListingInfo(unsigned addr = 0)
        : m_addr(addr)
      { }    

      unsigned m_addr;
      std::vector<uint8_t> m_ops;
    };

    std::vector<std::string> m_lines;
    std::map<int, ListingInfo> m_listings;
    std::map<std::string, unsigned> m_labels;

    std::string m_sourceDir;
    std::string m_basename;

    std::string m_sourceFn;
    std::string m_binaryFn;

    std::ifstream m_file;

    std::string m_symbol;
    std::string m_op;
    std::string m_value;
};

#endif // ASSEMBLER_H_
