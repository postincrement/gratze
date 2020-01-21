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

  // initialize the parser
  m_lineNumber = 0;
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

  S2650Assembler assembler;

  if (!assembler.Open(argv[1])) {
    cerr << "error: " << assembler.GetError() << endl;
    return -1;
  }

  assembler.Parse();

  cout << assembler.GetLineCount() << " lines parsed" << endl;

  assembler.WriteListing();
  assembler.WriteBinary();
}

