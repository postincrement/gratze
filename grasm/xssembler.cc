#include <sstream>
#include <iomanip>

#include "xssembler.h"
#include "common/cmdargs.h"


using namespace std;

XSsembler::XSsembler(const FormatInfo & format)
  : m_format(format)
{
}

void XSsembler::AssignSymbol(const std::string & sym, unsigned val)
{
  if (m_symbols.count(sym) != 0) {
    if (m_symbols[sym].m_value == val)
      return;
    // possible warning about dupicate symbol  
    return;  
  }

  m_symbols.insert(SymbolTable::value_type(sym, SymbolInfo(val)));
}