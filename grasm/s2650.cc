#include <iostream>
#include <fstream>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

#include "s2650.h"

extern XSsembler::OpCodeInfo g_2650_opcodes[];

using namespace std;

struct XSsembler::FormatInfo g_2650Format = {
  5,    // line number width
  5,    // number of bytes in listing file
  7,    // number of chars for symbol in listing file (if generated)
  10,   // number of chars for mnemonic column in listing file (if generated)
  13    // number of chars for argument column in listing file (if generated)
};

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


Token::Type Token::Parse(const std::string & str, size_t & pos)
{
  m_type = Type::eEOF;

  // ignore spaces
  while ((pos < str.length()) && isspace(str[pos]))
    ++pos;

  if (pos >= str.length()) {
    return m_type;
  }

  char ch = str[pos++];

  // character string
  if (ch == '\'') {
    size_t start = pos;
    while (pos < str.length()) {
      char ch = str[pos++];
      if (ch == '\'')
        break;
      m_lexeme += ch;
    }
    if (m_lexeme.length() == 1) {
      m_value = (int)m_lexeme[0];
      m_type  = Type::eInteger;
    }
    else {
      m_type = Token::Type::eString;
    }
  }

  // operator
  else if ((ch == '+') || (ch == '-')) {
    m_type = Token::Type::eOp;
    m_lexeme = ch;
  }

  // operator
  else if (ch == '$') {
    m_type = Token::Type::eCurrentAddr;
    m_lexeme = ch;
  }

  // numeric/hex constant
  else if (isdigit(ch)) {
    m_lexeme = ch;
    bool decimal = true;
    size_t start = pos;
    while (pos < str.length()) {
      char ch = tolower(str[pos++]);
      if (isdigit(ch)) {
        m_lexeme += ch;
      }
      else if (ch >= 'a' && (ch <= 'f')) {
        m_lexeme += ch;
        decimal = false;
      }
      else if (ch == 'h') {
        m_lexeme += ch;
        decimal = false;
        break;
      }
      else
        break;
    }
    char * ptr;
    m_value = strtoul(m_lexeme.c_str(), &ptr, decimal ? 10 : 16);
    if (ptr == NULL) {
      cerr << "error: cannot parse expression '" << str << "'" << endl;
    }
    else {
      //cout << "'" << m_lexeme << "' converted to 0x" << hex << m_value << endl;
      m_type = Type::eInteger;
    }
  }

  // symbol
  else if (isalpha(ch)) {
    m_lexeme = ch;
    size_t start = pos;
    while ((pos < str.length()) && isalnum(str[pos])) {
      m_lexeme += str[pos++];
    }
    m_type = Token::Type::eSymbol;
  }

  return m_type;;
};

static bool ParseReg(uint8_t & reg, const std::string & arg_)
{
  std::string arg(arg_);
  for (auto & r : arg) r = tolower(r);

  reg = 0;
  if (arg == "r0")
    reg =  0x00;
  else if (arg == "r1")
    reg = 0x01;
  else if (arg == "r2")
    reg = 0x02;
  else if (arg == "r3")
    reg = 0x03;

  else if (arg == "un")
    reg = 0x03;
  else if (arg == "z")
    reg = 0;
  else if (arg == "n")
    reg = 2;
  else if (arg == "p")
    reg = 1;

  else if (arg == "eq")
    reg = 0;
  else if (arg == "lt")
    reg = 0x02;
  else if (arg == "gt")
    reg = 0x01;

  else
    return false;

  return true;  
}

static bool ResolveIntegerValue(unsigned & val, const Token & token, unsigned int currentAddr, Assembler::SymbolTable & symbolTable)
{
  switch (token.m_type) {
    case Token::Type::eEOF:
    case Token::Type::eOp:
    case Token::Type::eString:
      return false;

    case Token::Type::eInteger:
      val = token.m_value;
      break;

    case Token::Type::eCurrentAddr:
      val = currentAddr;
      break;

    case Token::Type::eSymbol:
      {
        std::string str(token.m_lexeme);
        if (symbolTable.count(str) == 0)
          return false;
        val = symbolTable[str].m_value;
        symbolTable[str].m_used = true;
      }
      break;
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

S2650Assembler::S2650Assembler()
  : Assembler(g_2650Format)
{
}

bool S2650Assembler::ParseExpr(unsigned int & val, const std::string & str, size_t pos, std::string & error)
{
  val = 0;
  stringstream strm;

  // get first token and resolve
  Token token1;
  if (token1.Parse(str, pos) == Token::Type::eEOF) {
    return true;
  }

  unsigned lhs;
  if (!ResolveIntegerValue(lhs, token1, m_address, m_symbols)) {
    error = "cannot resolve expected integer value '" + token1.m_lexeme + "'";
    return (m_pass == 1);
  }

  // get op - if none, then return lhs
  Token token2;
  if (token2.Parse(str, pos) == Token::Type::eEOF) {
    val = lhs;
    return true;
  }
  if (token2.m_type != Token::Type::eOp) {
    stringstream strm;
    strm << "expected operator, got '" << token2.m_lexeme << "'";
    error = strm.str();
    return false;
  }

  // get rhs
  unsigned rhs;
  if (!ParseExpr(rhs, str, pos, error)) {
    error = "cannot resolve expected integer value '" + token1.m_lexeme + "'";
    return (m_pass == 1);
  }

  switch (token2.m_lexeme[0]) {
    case '+':
      val = lhs + rhs;
      break;
    case '-':
      val = lhs - rhs;
      break;
  }  

  //cout << "expr: " << token1.m_lexeme << "(0x" << hex << lhs << ") " << token2.m_lexeme << " 0x" << hex << rhs << " = 0x" << hex << val << endl;

  return true;
}

bool S2650Assembler::ParseByteExpr(uint8_t & val, const std::string & str, size_t pos, std::string & error)
{
  unsigned intVal;
  if (!ParseExpr(intVal, str, pos, error))
    return false;

  if (intVal > 0xff) {
    stringstream strm;
    strm << "expression value 0x" << hex << intVal << " too large for destination"; 
    error = strm.str();
    return false;
  }

  val = intVal & 0xff;
  return true;
}

bool S2650Assembler::ParseIndexExpr(uint8_t & reg, unsigned & addr, const std::string & arg, const std::string & str_)
{
  if (!ParseReg(reg, arg)) {
    return ParseError("unknown reg", arg);
  }

  std::string str(str_);

  bool indirect = (str[0] == '*');
  if (indirect) {
    str = str.substr(1);
  }

  reg = 0;
  addr = 0;

  size_t pos = str.find(',');

  // non-indexed
  if (pos == std::string::npos) {
    if (!ParseExpr(addr, str, 0, m_error))
      return ParseError(m_error);
    addr &= 0x1fff;  

    if (!ParseReg(reg, arg)) {
      return ParseError("unknown reg", str);
    }
  }
  else {
    // all index operations require R0 as argument
    if (arg != "r0") {
      return ParseError("index mode requires R0 as destination");
    }

    // get base address
    std::string value = Trim(str.substr(0, pos));
    ++pos;
    str = str.substr(pos);
    if (!ParseExpr(addr, value, 0, m_error)) {
      return ParseError(m_error);
    }

    addr &= 0x1fff;  

    // see if auto inc/dec  
    std::string regName;
    size_t pos = str.find(',');
    if (pos == std::string::npos) {
      regName = str;
      addr |= 0x6000;
    }
    else {
      regName = str.substr(0, pos);
      str = Trim(str.substr(pos+1));
      if (str[0] == '-')
        addr |= 0x4000;
      else if (str[0] == '+')
        addr |= 0x2000;
      else {
        stringstream strm;
        strm << "unknown index direction '" << str << "'";
        return ParseError(strm.str());
      }
    }

    // parse index reg
    regName = Trim(regName);
    if (!ParseReg(reg, regName)) {
      return ParseError("unknown reg", regName);
    }
  }

  addr |= (indirect ? 0x8000 : 0);

  return true;
}


bool S2650Assembler::IsCommentStart(const char * str, size_t col)
{
  if (*str == 0)
    return false;

  if ((col == 0) && (*str == '*'))
    return true;

  return (*str == ';'); 
}

bool S2650Assembler::ParseLine()
{
  ListingInfo & listing = m_listings[m_lineNumber];

  // symbol and op
  m_symbol  = GetNextWord();    // label or symbol
  m_op      = GetNextWord();    
  m_value   = GetRestOfLine();

  // ignore comment lines
  if (m_symbol.empty() && m_op.empty())
    return true;

  std::string op(m_op);
  std::string value(m_value);

  for (auto & r : op)    r = tolower(r);
//  for (auto & r : value) r = tolower(r);

  // EQU pseudo-op
  if (op == "equ") {
    unsigned intVal;
    if (!ParseExpr(intVal, m_value, 0, m_error)) {
      return ParseError(m_error);
    }
    listing.m_addr = intVal;
    AssignSymbol(m_symbol, intVal);
    listing.m_equ = true;
    return true;
  }

  // org pseudo-op
  if (op == "org") {
    unsigned intVal;
    if (!ParseExpr(intVal, m_value, 0, m_error))
      return ParseError(m_error);

    listing.m_addr = intVal;
    m_address      = intVal;
    return true;
  }

  // if symbol specified, defines symbol as current address
  if (!m_symbol.empty()) {
    AssignSymbol(m_symbol, m_address);
  }

  listing.m_addr = m_address;

  if (op.empty())
    return true;

  // DW pseudo-op
  if (op == "dw") {
    unsigned intVal;
    if (!ParseExpr(intVal, m_value, 0, m_error)) {
      return ParseError(m_error);
    }
    listing.m_ops.resize(2);
    listing.m_ops[0] = intVal >> 8;
    listing.m_ops[1] = intVal & 0xff;
    m_address += 2;
    return true;
  }

  // DB pseudo op
  if (op == "db") {
    Token token;
    size_t pos = 0;
    if (token.Parse(m_value, pos) == Token::Type::eString) {
      listing.m_ops.resize(token.m_lexeme.length());
      int i = 0;
      for (auto & r : token.m_lexeme) {
        listing.m_ops[i] = r;
        m_address++;
        ++i;
      }
      return true;
    }

    listing.m_ops.resize(1);
    if (ParseByteExpr(listing.m_ops[0], m_value, 0, m_error)) {
      m_address++;
      return true;
    }
    else {
      return ParseError(m_error);
    }
  }

  // DS pseudo-op
  if (op == "ds") {
    unsigned intVal;
    if (!ParseExpr(intVal, m_value, 0, m_error)) {
      return ParseError(m_error);
    }
    listing.m_spaceLen = intVal;
    m_address += intVal;
    return true;
  }

  // extract mnemonic arg
  std::string arg;
  size_t pos = op.find(',');
  if (pos != string::npos) {
    arg = op.substr(pos+1);
    op  = op.substr(0,pos);
  }

  // look for mnemonic
  OpCodeInfo * info = g_2650_opcodes;
  while (info->m_mnemonic != 0) {
    if (op == info->m_mnemonic)
      break;
    ++info;  
  }
  if (info->m_mnemonic == 0) {
    return ParseError("unknown mnemonic", m_op);
  }

  // copy opcode to listing
  int len = info->m_opLen;
  listing.m_ops.resize(len);
  listing.m_ops[0] = info->m_opcode;

  uint8_t reg = 0;
  bool indirect = false;
  unsigned addr = 0;
  signed offs = 0;
  stringstream strm;

  switch ((S2650Mode)(info->m_mode & XSsembler::eModeMask)) {
    case S2650Mode::eZ: // r0
      if (!value.empty()) {
        if (!ParseReg(reg, value)) {
          return ParseError("unknown reg", m_value);
        }    
      }
      else { 
        if (!ParseReg(reg, arg)) {
          return ParseError("unknown reg", arg);
        }
      }
      listing.m_ops[0] = (listing.m_ops[0] & 0xfc) | reg;
      break;

    case S2650Mode::eI: // immediate
      if (!ParseByteExpr(listing.m_ops[1], value, 0, m_error)) {
        return ParseError(m_error);
      }
      if (!ParseReg(reg, arg)) {
        return ParseError("unknown reg", arg);
      }
      listing.m_ops[0] = (listing.m_ops[0] & 0xfc) | reg;;
      break;

    case S2650Mode::eIn: // immediate with no arg
      if (!ParseByteExpr(listing.m_ops[1], value, 0, m_error)) {
        return ParseError(m_error);
      }
      break;

    case S2650Mode::eR: // relative
      indirect = (value[0] == '*');
      if (indirect) {
        value = value.substr(1);
      }
      if (!ParseExpr(addr, value, 0, m_error)) {
        return ParseError(m_error);
      }
      if (!ParseReg(reg, arg)) {
        return ParseError("unknown reg", arg);
      }
      offs = addr - (m_address + 2);
      if ((m_pass == 2) && ((offs < -64) || (offs > 63))) {
        strm << "relative offset " << (int)offs << " is too large";
        return ParseError(strm.str());
      }
      listing.m_ops[0] = (listing.m_ops[0] & 0xfc) | reg;;
      listing.m_ops[1] = (offs & 0x7f) | (indirect ? 0x80 : 0x00);
      break;

    case S2650Mode::eA: // absolute
      if (!ParseIndexExpr(reg, addr, arg, value))
        return false;

      listing.m_ops[0] |= reg;
      listing.m_ops[1] = addr >> 8;
      listing.m_ops[2] = addr & 0xff;
      break;

    case S2650Mode::eB: // cond absolute
      if (!ParseReg(reg, arg)) {
        return ParseError("unknown reg", arg);
      }
      indirect = (value[0] == '*');
      if (indirect) {
        value = value.substr(1);
      }
      if (!ParseExpr(addr, value, 0, m_error)) {
        return ParseError(m_error);
      }
      addr &= 0x7fff;
      listing.m_ops[0] = (listing.m_ops[0] & 0xfc) | reg;;
      listing.m_ops[1] = (addr >> 8) | (indirect ? 0x80 : 0x00);
      listing.m_ops[2] = (addr & 0xff);
      break;

    case S2650Mode::eE: // misc
      break;
  }

  m_address += info->m_opLen;

  return true;
}


////////////////////////////////////////////////////////////////////////////////////

S2650Disassembler::S2650Disassembler()
  : Disassembler(g_2650Format)
{
}

std::string DecodeReg(uint8_t reg, const XSsembler::OpCodeInfo * info)
{
  if (info->m_bitInfo == 0)
    return std::string("???");

  if (info->m_mask == 0xff) {
    return (const char *)info->m_bitInfo;
  }

  const char ** names = (const char **)info->m_bitInfo;
  return names[reg & 3];
}

const XSsembler::OpCodeInfo * S2650Disassembler::GetOpcodes() const
{
  return g_2650_opcodes;
}

bool S2650Disassembler::DecodeInstruction(DisasmInfo & disasm, int & len, unsigned address, const std::vector<uint8_t> & image, unsigned offs, bool & isTerm)
{
  len = 0;
  uint8_t opcode = image[offs];
  const OpCodeInfo * info = nullptr;

  const OpCodeInfo * ptr = g_2650_opcodes;
  while (ptr->m_mnemonic != 0) {
    if (
          ((opcode & ptr->m_mask) == ptr->m_opcode) &&
          ((info == nullptr) || (ptr->m_mask > info->m_mask))
        )
        info = ptr;
     ptr++;     
  }

  if (info == nullptr)
    return false;

  unsigned addr;
  signed relOffs;
  uint8_t v;
  bool indirect;
  std::string symbol;
  int mode;
  len  = info->m_opLen;
  isTerm = (info->m_mode & XSsembler::eTerm) != 0;
  stringstream strm;
  disasm[1] = info->m_mnemonic;
  switch ((S2650Mode)(info->m_mode & XSsembler::eModeMask)) {

    case S2650Mode::eZ:    // register addressing
      disasm[2] = DecodeReg(opcode, info);
      break;

    case S2650Mode::eI:    // immediate addressing
      disasm[1] += "," + DecodeReg(opcode, info);
      strm << "0" << setw(2) << setfill('0') << hex << (int)m_image[offs+1] << "h";
      disasm[2] = strm.str();
      break; 

    case S2650Mode::eR:    // relative addressing
      indirect = (m_image[offs+1] & 0x80) != 0;
      v = m_image[offs+1] & 0x7f;
      if (v > 63)
        relOffs = v - 128;
      else
        relOffs = v;
//      cout << "rel val 0x" << hex << (int)(m_image[offs+1] & 0x7f) << " -> " << dec << (int)relOffs << endl;
      addr = address + 2 + relOffs;
      addr &= 0x7fff;
      disasm[1] += "," + DecodeReg(opcode, info);
      disasm[2] = (indirect ? "*" : "") + CreateSymbol(addr);
      break;

    case S2650Mode::eA:    // absolute addressing (non-branch)
      indirect = (m_image[offs+1] & 0x80) != 0;
      mode = (m_image[offs+1] >> 5) & 0x3;
      addr = (m_image[offs+1] << 8) + m_image[offs+2];
      addr &= 0x1fff;
      disasm[2] = (indirect ? "*" : "") + CreateSymbol(addr);
      if (mode == 0) {
        disasm[1] += "," + DecodeReg(opcode, info);
      }
      else {
        disasm[1] += ",r0";
        disasm[2] += "," + DecodeReg(opcode, info);
        if (mode == 1) {
          disasm[2] += ",+";
        }
        else if (mode == 2) {
          disasm[2] += ",-";
        }
      }
      break;

    case S2650Mode::eB:    // absolute addressing (branch)
      indirect = (m_image[offs+1] & 0x80) != 0;
      addr = (m_image[offs+1] << 8) + m_image[offs+2];
      addr &= 0x7fff;
      disasm[1] += "," + DecodeReg(opcode, info);
      disasm[2] = (indirect ? "*" : "") + CreateSymbol(addr);
      break;

    case S2650Mode::eZa:   // register addressing using arg
      disasm[2] = DecodeReg(opcode, info);
      break;

    case S2650Mode::eIn:   // immediate addressing with no arg
      strm << "0" << setw(2) << setfill('0') << hex << (int)m_image[offs+1] << "h";
      disasm[2] = strm.str();
      break;

    case S2650Mode::eE:    // miscellaneous instructions
      break;
  }
  return true;
}