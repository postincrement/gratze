#include <sstream>
#include <iomanip>

#include "assembler.h"

#include "s2650.h"

using namespace std;

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
  }

  return true;
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
  cout << endl; 
  cout << setw(5) << dec << m_lineNumber << "     " << m_line << endl;
  cout << "          ";
  for (int i = 0; i < m_pos-1; ++i)
    cout << ' ';
  cout << '^' << endl;  
  cout << "sym:'" << m_symbol << "', op:'" << m_op << "', value:'" << m_value << "'" << endl;
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

void Assembler::AssignSymbol(const std::string & sym, unsigned val)
{
  if (m_symbols.count(sym) != 0) {
    if (m_symbols[sym] == val)
      return;
    return;  
  }

  m_symbols[sym] = val;
}

//////////////////////////////////////////////////////

std::string Assembler::GetListing()
{
  std::stringstream strm;
  int opcodeCols = 4;

  for (int i = 1; i <= m_lineNumber; ++i) {

    // print line number
    strm << setw(5) << setfill(' ') << dec << i;

    // print empty line if source line was empty
    auto r = m_listings.find(i);
    if (r == m_listings.end()) {
      strm << "\n";
      continue;
    }

    // print address (which could be a symbol value)
    strm << " " << setw(4) << setfill('0') << hex << r->second.m_addr;

    // if no opcodes, print line only
    if ((r->second.m_ops.size() == 0)) {
      strm << "               " << m_lines[i-1] << endl;
      continue;
    }

    strm << " ";

    // print opcodes
    auto & listing = r->second;
    bool first = true;
    int count = 0;
    while (count < r->second.m_ops.size()) {
      if (!first) {
        strm << "\n           ";
      }
      int col = r->second.m_ops.size() - count;
      if (col > 4)
        col = 4;
      int j;
      for (j = 0; j < col; ++j)
        strm << hex << setw(2) << setfill('0') << (int)listing.m_ops[j] << " ";
      for (;j < 4; ++j)
        strm << "   ";
      count += col;  
      if (first) {
        strm << "  ";
        strm << m_lines[i-1];
        first = false;
      }
    }
    if (first) {
      strm << "  ";
      strm << m_lines[i-1] << endl;
    }
    else {
      strm << endl;
    }
  }

  for (auto & r : m_symbols)
    strm << r.first << " = 0x" << hex << r.second << endl;

  return strm.str();
}


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

  cout << dec << data.size() << " bytes written to " << m_binaryFn << endl; 
  file.close();
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

  S2650Assembler assembler;

  if (!assembler.Open(argv[1])) {
    cerr << "error: " << assembler.GetError() << endl;
    return -1;
  }

  bool result = assembler.Parse();
  cout << assembler.GetLineCount() << " lines parsed" << endl;
  if (result) {
    assembler.WriteListing();
    assembler.WriteBinary();
  }
}

