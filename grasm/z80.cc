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

  if ((col == 0) && (*str == '*'))
    return true;

  return (*str == ';'); 
}

bool Z80Assembler::ParseLine()
{
  ListingInfo & listing = m_listings[m_lineNumber];

  //m_address += info->m_opLen;

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