#include <iostream>
#include <fstream>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

using namespace std;

struct OpCodeInfo
{
  const char * m_mnemonic;
  int          m_mode;   // 0 = R0, 1 = immediate, 2 = rel, 3 = abs  
  uint8_t      m_opcode;
  int          m_opLen;
};

static OpCodeInfo g_opcodes[] = {
  { "lodz", 0, 0x00, 1 },
  { "lodi", 1, 0x04, 2 },
  { "lodr", 2, 0x08, 2 },
  { "loda", 3, 0x0c, 3 },

  { "strz", 0, 0xc0, 1 },
  { "stri", 1, 0xc4, 2 },
  { "strr", 2, 0xc8, 2 },
  { "stra", 3, 0xcc, 3 },

  { "addz", 0, 0x80, 1 },
  { "addi", 1, 0x84, 2 },
  { "addr", 2, 0x88, 2 },
  { "adda", 3, 0xcc, 3 },

  { "subz", 0, 0xa0, 1 },
  { "subi", 1, 0xa4, 2 },
  { "subr", 2, 0xa8, 2 },
  { "suba", 3, 0xac, 3 },

  { "dar",  0, 0x94, 1 },

  { "andz", 0, 0x40, 1 },
  { "andi", 1, 0x44, 2 },
  { "andr", 2, 0x48, 2 },
  { "anda", 3, 0x4c, 3 },

  { "iorz", 0, 0x60, 1 },
  { "iori", 1, 0x64, 2 },
  { "iorr", 2, 0x68, 2 },
  { "iora", 3, 0x6c, 3 },

  { "eorz", 0, 0x20, 1 },
  { "eori", 1, 0x24, 2 },
  { "eorr", 2, 0x28, 2 },
  { "eora", 3, 0x2c, 3 },
  
  { "comz", 0, 0xe0, 1 },
  { "comi", 1, 0xe4, 2 },
  { "comr", 2, 0xe8, 2 },
  { "coma", 3, 0xec, 3 },

  { "rrr",  9, 0x50, 1 },
  { "rrl",  9, 0xd0, 1 },

  { "bctr", 4 + 2, 0x18, 2 },
  { "bcta", 4 + 3, 0x1c, 3 },

  { "bcfr", 4 + 2, 0x98, 2 },
  { "bcfa", 4 + 3, 0x9c, 3 },

  { "brnr", 4 + 2, 0x58, 2 },
  { "brna", 4 + 3, 0x5c, 3 },

  { "birr", 4 + 2, 0xd8, 2 },
  { "bira", 4 + 3, 0xdc, 3 },

  { "bdrr", 4 + 2, 0xf8, 2 },
  { "bdra", 4 + 3, 0xfc, 3 },

  { "zbrr", 2, 0x9b, 2 },
  { "bxa",  3, 0x9f, 3 },

  { "bstr", 4 + 2, 0x38, 2 },
  { "bsta", 4 + 3, 0x3c, 3 },

  { "bsfr", 4 + 2, 0xb8, 2 },
  { "bsfa", 4 + 3, 0xbc, 3 },

  { "bsnr", 4 + 2, 0x78, 2 },
  { "bsna", 4 + 3, 0x7c, 3 },

  { "zbsr", 2, 0xbb, 2 },
  { "bsxa", 3, 0xbf, 3 },

  { "retc", 4 + 0, 0x14, 1 },
  { "rete", 4 + 0, 0x34, 1 },

  { "wrtd", -1,    0xf0, 1 },
  { "redd", 8,     0x70, 2 },
  { "wrtc", -1,    0xb0, 1 },
  { "redc", 8,     0x30, 2 },

  { "wrtc", 1,     0xd4, 2 },
  { "rede", 1,     0x54, 2 },

  { "halt", -1,     0x40, 1 },
  { "nop",  -1,     0xc0, 1 },

  { "tmi",  1,     0xf4, 1 },

  { "lpsu", -1,     0x92, 1 },
  { "lpsl", -1,     0x93, 1 },

  { "spsu", -1,     0x12, 1 },
  { "spsl", -1,     0x13, 1 },

  { "cpsu", 8,     0x78, 2 },
  { "cpsl", 8,     0x79, 2 },

  { "ppsu", 8,     0x76, 2 },
  { "ppsl", 8,     0x77, 2 },

  { "tpsu", 8,     0xb4, 2 },
  { "tpsl", 8,     0xb5, 2 },

  { 0 }
};


static std::string TrimRight(const std::string & str_)
{
  std::string str(str_);

  while ((str.length() > 0) && ::isspace(str[str.length()-1]))
    str = str.substr(0, str.length()-1);

  return str;
}

static std::string TrimLeft(const std::string & str_)
{
  std::string str(str_);

  while ((str.length() > 0) && ::isspace(str[0]))
    str = str.substr(1);

  return str;  
}

static std::string Trim(const std::string & str)
{
  return TrimLeft(TrimRight(str));
}

//////////////////////////////////////////////////////////////////

class Assembler
{
  public:
    Assembler();

    bool Open(const std::string & fn);
    bool Parse();

    bool ParseLine(const std::string & line_);

    int GetLineCount() const
    { return m_lineNumber; }

    std::string GetNextWord();

    void AssignSymbol(const std::string & sym, unsigned val);
    void AssignSymbol(const std::string & sym, const std::string & val);

    std::string GetListing();
    std::string GetBinary();

    std::string GetError();
    void ParseError(const std::string & str, const std::string & arg = "");

    bool WriteBinary();
    bool WriteListing();

  protected:   
    int m_lineNumber;
    unsigned m_origin;
    unsigned m_address;
    std::string m_trimmedLine;
    std::string m_line;
    std::string m_error;

    struct ListingInfo
    {
      ListingInfo(unsigned addr = 0)
        : m_opLen(0)
        , m_addr(addr)
      { }    

      unsigned m_addr;
      int      m_opLen;
      uint8_t  m_ops[3];
    };

    std::vector<std::string> m_lines;
    std::map<int, ListingInfo> m_listings;
    std::map<std::string, unsigned> m_labels;

    std::string m_sourceDir;
    std::string m_basename;

    std::string m_sourceFn;
    std::string m_binaryFn;

    ifstream m_file;

    std::string m_symbol;
    std::string m_op;
    std::string m_value;
};

Assembler::Assembler()
{}

bool Assembler::Open(const std::string & fn)
{
  m_sourceFn = fn;

  // extract dir and basename from source
  size_t pos = m_sourceFn.rfind('/');
  if (pos != std::string::npos) {
    m_sourceDir = m_sourceFn.substr(0, pos); 
    m_basename  = m_sourceFn.substr(pos);
  }
  else {
    m_sourceDir = "./"; 
    m_basename  = m_sourceFn;
  }  

  pos = m_basename.rfind('.');
  if (pos != std::string::npos) {
    m_basename = m_basename.substr(0, pos);
  }

  m_binaryFn = m_sourceDir + m_basename + ".bin";

  // open the source file
  m_file.open(m_sourceFn.c_str());
  if (!m_file.is_open()) {
    std::stringstream strm;
    strm << "cannot open '" << m_sourceFn << "'";
    return false;
  }

  // initialize the parser
  m_lineNumber = 0;
  m_origin     = 0x0000;
  m_address    = 0x0000;

  return true;
}

std::string Assembler::GetError()
{
  return m_error;
}

bool Assembler::Parse()
{
  std::string line;
  while (getline(m_file, line)) {
    ParseLine(line);
  }
  m_file.close();
  return true;
}

std::string Assembler::GetNextWord()
{
  std::string word;

  if (isspace(m_line[0])) {
    m_line = TrimLeft(m_line);
  }
  else {
    int pos = 0;
    while ((pos < m_line.length()) && !isspace(m_line[pos]))
      ++pos;
    word = m_line.substr(0, pos);
    m_line = TrimLeft(m_line.substr(pos));
  }
  return word;
}

unsigned ParseExpr(const std::string & str_)
{
  unsigned val = 0;

  std::string str(str_);
  for (auto & r : str) r = tolower(r);

  if (str.length() == 0)
    return val;

  char * ptr;
  if (str[str.length()-1] == 'h') {
    val = strtoul(str.c_str(), &ptr, 16);
  }
  else { 
    val = strtoul(str.c_str(), &ptr, 10);
  }

  return val;
}

bool ParseReg(uint8_t & reg, const std::string & arg)
{
  reg = 0;
  if (arg == "r0")
    ;
  else if (arg == "r1")
    reg = 0x01;
  else if (arg == "r2")
    reg = 0x02;
  else if (arg == "r3")
    reg = 0x03;
  else
    return false;

  return true;  
}

bool ParseCond(uint8_t & reg, const std::string & arg)
{
  reg = 0;
  if (ParseReg(reg, arg))
    return true;

  if (arg == "un")
    ;
  else if (arg == "z")
    ;
  else if (arg == "n")
    ;
  else if (arg == "p")
    ;
  else if (arg == "eq")
    ;
  else if (arg == "lt")
    reg = 0x01;
  else if (arg == "gt")
    reg = 0x02;
//  else if (arg == "r3")
//    reg = 0x03;
  else
    return false;
  return true;  
}

void Assembler::ParseError(const std::string & str, const std::string & arg)
{
  cerr << "error : " << str;
  if (!arg.empty()) {
    cerr << " '" << arg << "'";
  }
  cout << endl; 
  cout << "sym:'" << m_symbol << "', op:'" << m_op << "', value:'" << m_value << "'" << endl;
  cout << setw(5) << dec << m_lineNumber << "     " << m_trimmedLine << endl;
}

bool Assembler::ParseLine(const std::string & line_)
{
  ++m_lineNumber;

  // save lines for creating the listing file
  m_line = TrimRight(line_);
  m_lines.push_back(m_line);

  // ignore blank lines
  m_trimmedLine = TrimLeft(m_line);
  if (m_trimmedLine.length() == 0)
    return true;

  // ignore comment lines
  if ((m_trimmedLine[0] == '*') || (m_trimmedLine[0] == ';'))
    return true;

  ListingInfo & listing = m_listings[m_lineNumber];

  // symbol and op
  m_symbol  = GetNextWord();
  m_op      = GetNextWord();
  m_value   = GetNextWord();

  std::string op(m_op);
  std::string value(m_value);

  for (auto & r : op) r = tolower(r);
  for (auto & r : value) r = tolower(r);

  if (!m_symbol.empty() && op.empty()) {
    AssignSymbol(m_symbol, m_address);
    listing.m_addr = m_address;
  }
  else if (op == "equ") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr = intVal;
    AssignSymbol(m_symbol, intVal);
  }
  else if (op == "org") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr = intVal;
    cout << "set address to " << intVal << endl;
  }
  else if (op == "dw") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr   = m_address;
    listing.m_ops[0] = intVal >> 8;
    listing.m_ops[1] = intVal & 0xff;
    listing.m_opLen  = 2;
    m_address += 2;
  }
  else if (op == "db") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr   = m_address;
    listing.m_ops[0] = intVal & 0xff;
    listing.m_opLen  = 1;
    m_address += 1;
  }
  else if (op == "ds") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr   = m_address;
    m_address += intVal;
  }
  else {
    std::string arg;
    size_t pos = op.find(',');
    if (pos != string::npos) {
      arg = op.substr(pos+1);
      op  = op.substr(0,pos);
    }

    OpCodeInfo * info = g_opcodes;
    while (info->m_mnemonic != 0) {
      if (op == info->m_mnemonic)
        break;
      ++info;  
    }
    if (info->m_mnemonic == 0) {
      ParseError("unknown mnemonic", m_op);
    }
    else {
      listing.m_addr   = m_address;
      listing.m_ops[0] = info->m_opcode;
      listing.m_opLen  = info->m_opLen;

      switch (info->m_mode) {
        case -1: // no arg
          break;
        case 0: // r0
          {
            uint8_t reg;
            if (!ParseReg(reg, value)) {
              ParseError("mode 0 - unknown reg", m_value);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 1: // immediate
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseReg(reg, arg)) {
              ParseError("mode 1 - unknown reg", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 2: // relative
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseReg(reg, arg)) {
              ParseError("mode 2 - unknown reg", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 3: // absolute
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseReg(reg, arg)) {
              ParseError("mode 3 - unknown reg", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 4+0: // r0
          {
            uint8_t reg;
            if (!ParseCond(reg, arg)) {
              ParseError("mode 4 - unknown reg", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 4+2: // cond + relative
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseCond(reg, arg)) {
              ParseError("mode 6 - unknown cond", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 4+3: // cond absolute
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseCond(reg, arg)) {
              ParseError("mode 7 - unknown cond", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        case 8: // immediate
          {
            listing.m_ops[1] = ParseExpr(value);
          }
          break;
        case 9: // r0
          {
            uint8_t reg;
            if (!ParseReg(reg, arg)) {
              ParseError("mode 9 - unknown reg", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;
        default:
          ParseError("unknown mnemonic", op);
          break;
      }
      m_address += info->m_opLen;
    }
  }

  return true;
}

void Assembler::AssignSymbol(const std::string & sym, unsigned val)
{
  //cout << "symbol: '" << sym << "' = 0x" << hex << val << endl;
}

void Assembler::AssignSymbol(const std::string & sym, const std::string & val)
{
  //cout << "symbol: '" << sym << "' = '" << val << "'" << endl;
}

//////////////////////////////////////////////////////

bool Assembler::WriteListing()
{
  std::string listFn = m_sourceDir + m_basename + ".lst";

  // write to file
  ofstream file(listFn.c_str());
  if (!file.is_open()) {
    std::stringstream strm;
    strm << "cannot create '" << listFn << "'";
    return false;
  }

  std::string listing = GetListing();
  file.write((const char *)&listing[0], listing.length());

  cout << dec << listing.size() << " bytes written to " << listFn << endl; 
  file.close();

  return true;
}

bool Assembler::WriteBinary()
{
  std::vector<uint8_t> data;

  // get binary data
  for (auto & r : m_listings) {
    for (int i = 0; i < r.second.m_opLen; ++i) {
      data.push_back(r.second.m_ops[i]);
    }
  }

  // write to file
  ofstream file(m_binaryFn.c_str(), std::ofstream::binary);
  if (!file.is_open()) {
    std::stringstream strm;
    strm << "cannot create '" << m_binaryFn << "'";
    return false;
  }

  file.write((const char *)&data[0], data.size());

  cout << dec << data.size() << " bytes written to " << m_binaryFn << endl; 
  file.close();
}

std::string Assembler::GetListing()
{
  std::stringstream strm;

  for (int i = 1; i <= m_lineNumber; ++i) {

    // lllll aaaa 11 22 33      line.....

    strm << setw(5) << setfill('0') << dec << i << " ";
    auto r = m_listings.find(i);
    if (r == m_listings.end()) {
           // "aaaa 11 22 33      "
      strm << "                      ";
    }
    else {
      auto & listing = r->second;
      strm << hex << setw(4) << setfill('0') << listing.m_addr;
      strm << " ";
      int j;
      for (j = 0; j < listing.m_opLen; ++j)
        strm << hex << setw(2) << setfill('0') << (int)listing.m_ops[j] << " ";
      for (;j < 3; ++j)
        strm << "   ";
    }
    strm << "  ";
    strm << m_lines[i-1] << endl;
  }

  return strm.str();
}

//////////////////////////////////////////////////////

void Usage()
{
  cout << "usage: asm2650 fn" << endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    Usage();
    return 0;
  }

  Assembler assembler;

  if (!assembler.Open(argv[1])) {
    cerr << "error: " << assembler.GetError() << endl;
    return -1;
  }

  assembler.Parse();

  cout << assembler.GetLineCount() << " lines parsed" << endl;

  assembler.WriteListing();
  assembler.WriteBinary();
}

