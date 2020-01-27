#include <sstream>
#include <iomanip>

#include "assembler.h"
#include "cmdargs.h"

#define GRASM_VERSION   "1.0.0"

#define FORM_FEED       0x0c

using namespace std;

XSsembler::XSsembler(const FormatInfo & format)
  : m_format(format)
{}



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

////////////////////////////////////////////////////////////////////////////////////////////////

Assembler::Assembler(const FormatInfo & format)
  : XSsembler(format)
{}

bool Assembler::Open(const CommandLineArgs & args, const std::string & fn)
{
  m_sourceFn = fn;

  // extract dir and basename from source
  size_t pos = m_sourceFn.rfind("/");
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

  if (!args.GetValue("-O", m_binaryFn))
    m_binaryFn = m_sourceDir + m_basename + ".out";

  if (!args.GetValue("-L", m_listingFn)) {
    if (m_listingFn != "--")
      m_listingFn = m_sourceDir + m_basename + ".lst";
  }

  m_includeSymbols = args.HasArg("-s");
  args.GetValue("-p", m_pageLength);

  // open the source file
  m_file.open(m_sourceFn.c_str());
  if (!m_file.is_open()) {
    std::stringstream strm;
    strm << "cannot open '" << m_sourceFn << "'";
    return false;
  }

  return true;
}

std::string Assembler::GetError()
{
  return m_error;
}

bool Assembler::Parse()
{
  // load the file into a memory vector
  std::string line;
  while (getline(m_file, line)) {
    m_line = TrimRight(line);
    m_lines.push_back(m_line);
  }
  m_file.close();

  // two passes through file, so forward declared symbols 
  // are defined on second pass
  for (m_pass = 1; m_pass <= 2; ++m_pass) {

    // initialize the parser
    m_lineNumber = 0;
    m_address    = 0x0000;
    m_errorCount = 0;

    for (auto & line : m_lines) {
      ++m_lineNumber;
      std::string trimmedLine = TrimLeft(line);
      if (trimmedLine.length() == 0)
        continue;
      m_line = line;
      m_pos = 0;
      if (!ParseLine())
        m_errorCount++;
    }

    // if any errors after pass1, no need to to do pass 2
    if (m_errorCount > 0) {
      cerr << m_errorCount << " errors found" << endl;
      return false;
    }
    else if (m_pass == 2)
      cerr << "no errors found" << endl;
  }

  return true;
}

std::string Assembler::GetRestOfLine()
{
  size_t start = m_pos;

  while ((m_pos < m_line.length()) && !IsCommentStart(&m_line[m_pos], m_pos))
    ++m_pos;

  return Trim(m_line.substr(start, m_pos-start));  
}


std::string Assembler::GetNextWord()
{
  // if position is spaces, consume them and return empty word
  if (isspace(m_line[m_pos])) {
    while ((m_pos < m_line.length()) && isspace(m_line[m_pos])) {
      if (IsCommentStart(&m_line[m_pos], m_pos))
        break;
      ++m_pos;
    }
    return "";  
  }

  // if remainder of line is a comment, return empty word
  if (IsCommentStart(&m_line[m_pos], m_pos))
    return "";

  // collect the word
  std::string word;
  size_t start = m_pos;
  while ((m_pos < m_line.length()) && !isspace(m_line[m_pos])) {
    if (IsCommentStart(&m_line[m_pos], m_pos))
      break;
    ++m_pos;  
  }
  word = m_line.substr(start, m_pos - start);

  // consume trailing spaces
  while ((m_pos < m_line.length()) && isspace(m_line[m_pos])) {
    if (IsCommentStart(&m_line[m_pos], m_pos))
      break;
    ++m_pos;
  }

  return word;
}

bool Assembler::ParseError(const std::string & str, const std::string & arg)
{
  cerr << "error : " << str;
  if (!arg.empty()) {
    cerr << " '" << arg << "'";
  }
  cerr << endl; 
  cerr << setw(5) << dec << m_lineNumber << "     " << m_line << endl;
  cerr << "          ";
  for (int i = 0; i < m_pos-1; ++i)
    cerr << ' ';
  cerr << '^' << endl;  
  //cout << "sym:'" << m_symbol << "', op:'" << m_op << "', value:'" << m_value << "'" << endl;
  return false;
}

std::string Assembler::TrimRight(const std::string & str_)
{
  std::string str(str_);

  while ((str.length() > 0) && ::isspace(str[str.length()-1]))
    str = str.substr(0, str.length()-1);

  return str;
}

std::string Assembler::TrimLeft(const std::string & str_)
{
  std::string str(str_);

  while ((str.length() > 0) && ::isspace(str[0]))
    str = str.substr(1);

  return str;  
}

std::string Assembler::Trim(const std::string & str)
{
  return TrimLeft(TrimRight(str));
}

//////////////////////////////////////////////////////

std::string Assembler::GetPageHeader(int & row, int & page)
{
  std::string str;

  if (m_pageLength > 0) {
    stringstream strm;
    if (row == 0) {
      if (page != 1)
        str = std::string(1, FORM_FEED);
      strm << "GRASM ASSEMBLER v" << GRASM_VERSION;
      while (strm.str().length() < 70)
        strm << " ";
      strm << "PAGE " << (int)page++ << "\n";
      strm << "\n"
          << " LINE ADDR ";
      for (int j = 0; j < m_format.m_opcodeByteCount; ++j)
          strm << "B" << (int)j << " ";
      strm << "   SOURCE\n\n";
      str += strm.str();
    }
    row = (row + 1) % m_pageLength;
  }

  return str;
}

#define SPACE_BETWEEN_OPCODE_AND_LINE   3

std::string Assembler::GetListing()
{
  std::stringstream strm;

  int row = 0;
  int page = 1;

  for (int i = 1; i <= m_lineNumber; ++i) {
    strm << GetPageHeader(row, page);

    // print line number
    strm << setw(m_format.m_lineNumberWidth) << setfill(' ') << dec << i;

    // print empty line if source line was empty
    auto r = m_listings.find(i);
    if (r == m_listings.end()) {
      strm << "\n";
      continue;
    }

    // if no opcodes, print line only
    // else print address and opcodes
    strm << " ";
    if ((r->second.m_ops.size() != 0) || (r->second.m_spaceLen != 0) || r->second.m_equ) {
      strm << setw(4) << setfill('0') << hex << r->second.m_addr << " ";
    }
    else {
      strm << std::string(5, ' ');  // address + space
    }

    if (r->second.m_ops.size() == 0) {
      for (int j = 0; j < m_format.m_opcodeByteCount; ++j)
        strm << std::string(3, ' '); // opcode + space
      strm << std::string(SPACE_BETWEEN_OPCODE_AND_LINE, ' ') << m_lines[i-1] << endl;;
    }
    else {  
      // print opcodes
      auto & listing = r->second;
      bool first = true;
      int count = 0;
      while (count < r->second.m_ops.size()) {
        if (!first) { 
          strm << "\n";
          strm << GetPageHeader(row, page);
          strm << std::string(m_format.m_lineNumberWidth + 1 + 4 + 1, ' ');  // line number, space, address, space
        }
        int col = r->second.m_ops.size() - count;
        if (col > m_format.m_opcodeByteCount)
          col = m_format.m_opcodeByteCount;
        int j;
        for (j = 0; j < col; ++j)
          strm << hex << setw(2) << setfill('0') << (int)listing.m_ops[count+j] << " ";
        for (;j < m_format.m_opcodeByteCount; ++j)
          strm << "   ";
        count += col;  
        if (first) {
          strm << std::string(SPACE_BETWEEN_OPCODE_AND_LINE, ' ') << m_lines[i-1];
          first = false;
        }
      }
      if (first) {
        strm << std::string(SPACE_BETWEEN_OPCODE_AND_LINE, ' ') << m_lines[i-1] << endl;
      }
      else {
        strm << endl;
      }
    }
  }

  if (m_includeSymbols) {
    for (auto & r : m_symbols) {
      strm << GetPageHeader(row, page);
      strm << setw(8) << setfill(' ') << r.first << " = 0x" << setw(4) << setfill('0') << hex << r.second.m_value << (r.second.m_used ? "" : " not used") << endl;
    }
  }

  return strm.str();
}


bool Assembler::WriteListing()
{
  if (m_listingFn == "--") {
    cout << GetListing();
  }
  else {
    ofstream file(m_listingFn.c_str());
    if (!file.is_open()) {
      std::stringstream strm;
      strm << "cannot create '" << m_listingFn << "'";
      return false;
    }

    file << GetListing();
    cerr << "listing written to " << m_listingFn << endl; 
    file.close();
  }

  return true;
}

bool Assembler::WriteBinary()
{
  std::vector<uint8_t> data;

  // get binary data
  for (auto & r : m_listings) {
    for (auto & s : r.second.m_ops) {
      data.push_back(s);
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

  cerr << dec << data.size() << " bytes written to " << m_binaryFn << endl; 
  file.close();
}

//////////////////////////////////////////////////////


