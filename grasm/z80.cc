#include <iostream>
#include <fstream>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

#include "z80.h"

extern XSsembler::OpCodeInfo g_z80_opcodes[];

using namespace std;

struct XSsembler::FormatInfo g_z80Format = {
  5,    // line number width
  5,    // number of bytes in listing file
  7,    // number of chars for symbol in listing file (if generated)
  10,   // number of chars for mnemonic column in listing file (if generated)
  10    // number of chars for argument column in listing file (if generated)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Z80Assembler::Z80Assembler()
  : Assembler(g_z80Format)
{
}

bool Z80Assembler::IsCommentStart(const char * str, size_t col)
{
  if (*str == 0)
    return false;

  return (*str == ';'); 
}

bool Z80Assembler::ParseLine()
{
  ListingInfo & listing = m_listings[m_lineNumber];

  m_symbol  = GetNextWord();    // label or symbol
  m_op      = GetNextWord();    
  m_value   = GetRestOfLine();

  // ignore comment lines
  if (m_symbol.empty() && m_op.empty())
    return true;

  std::string op(m_op);
  std::string value(m_value);

  for (auto & r : op)    r = tolower(r);

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

  if (m_op.empty())
    return true;

  // DEFB pseudo op
  if (op == "defb") {
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
  // DEFS pseudo-op
  if (op == "defs") {
    unsigned intVal;
    if (!ParseExpr(intVal, m_value, 0, m_error)) {
      return ParseError(m_error);
    }
    listing.m_spaceLen = intVal;
    m_address += intVal;
    return true;
  }


  // look for mnemonic
  OpCodeInfo * info = g_z80_opcodes;
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

  return true;
}


////////////////////////////////////////////////////////////////////////////////////

Z80Disassembler::Z80Disassembler()
  : Disassembler(g_z80Format)
{
}

const XSsembler::OpCodeInfo * Z80Disassembler::GetOpcodes() const
{
  return g_z80_opcodes;
}

bool Z80Disassembler::DecodeInstruction(DisasmInfo & disasm, int & len, unsigned address, const std::vector<uint8_t> & image, unsigned offs, bool & isTerm)
{
  len = 0;
  uint8_t opcode = image[offs];
  const OpCodeInfo * info = nullptr;

  const OpCodeInfo * ptr = g_z80_opcodes;
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

  return true;
}