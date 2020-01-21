#include <iostream>
#include <fstream>
#include <ctype.h>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

#include "s2650.h"

extern OpCodeInfo g_2650_opcodes[];

using namespace std;

static unsigned ParseExpr(const std::string & str_)
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

static bool ParseReg(uint8_t & reg, const std::string & arg)
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
  else if (arg == "un")
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
  else
    return false;

  return true;  
}

bool S2650Assembler::ParseLine(const std::string & line_)
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
    cout << "set address to 0x" << setw(4) << setfill('0') << hex << intVal << endl;
  }
  else if (op == "dw") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr   = m_address;
    int len = 2;
    listing.m_ops.resize(len);
    listing.m_ops[0] = intVal >> 8;
    listing.m_ops[1] = intVal & 0xff;
    m_address += len;
  }
  else if (op == "db") {
    unsigned intVal = ParseExpr(value);
    listing.m_addr   = m_address;
    int len = 1;
    listing.m_ops.resize(len);
    listing.m_ops[0] = intVal & 0xff;
    m_address += len;
  }
  else if (op == "ds") {
    unsigned intVal = ParseExpr(value);
    int len = intVal;
    listing.m_ops.resize(len);
    listing.m_addr   = m_address;
    m_address += len;
  }
  else {
    std::string arg;
    size_t pos = op.find(',');
    if (pos != string::npos) {
      arg = op.substr(pos+1);
      op  = op.substr(0,pos);
    }

    OpCodeInfo * info = g_2650_opcodes;
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
      int len = info->m_opLen;
      listing.m_ops.resize(len);
      listing.m_ops[0] = info->m_opcode;

      switch ((S2650Mode)info->m_mode) {
        case S2650Mode::eZ: // r0
          {
            uint8_t reg;
            if (!value.empty()) {
              if (!ParseReg(reg, value)) {
                ParseError("mode 0 - unknown reg", m_value);
              }    
            }
            else if (!ParseReg(reg, arg)) {
              ParseError("mode 0 - unknown reg", arg);
            }    
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;

        case S2650Mode::eI: // immediate
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

        case S2650Mode::eIn: // immediate with no arg
          {
            listing.m_ops[1] = ParseExpr(value);
          }
          break;

        case S2650Mode::eR: // relative
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

        case S2650Mode::eA: // absolute
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

        case S2650Mode::eB: // cond absolute
          {
            listing.m_ops[1] = ParseExpr(value);
            uint8_t reg;
            if (!ParseReg(reg, arg)) {
              ParseError("mode 7 - unknown cond", arg);
            }
            else {
              listing.m_ops[0] |= reg;
            }
          }
          break;

        case S2650Mode::eE: // misc
          {
          }
          break;
      }
      m_address += info->m_opLen;
    }
  }

  return true;
}
