#include <sstream>

#include "misc.h"

using namespace std;

ColumnFormatter::StringVector ColumnFormatter::Columns::m_dummy;



std::string DumpMemory(const uint8_t * memory, int memoryLen)
{
  stringstream strm;

  int cols = 16;
  int p = 0;
  while (p < memoryLen) {
    strm << HEXFORMAT0x4(p) << "  ";
    int len = std::min(memoryLen-p, cols);
    int i;
    for (i = 0; i < len; ++i)
      strm << " " << HEXFORMAT2(memory[p + i]);
    while (i++ < cols)
      strm << "   ";
    strm << "   ";
    for (i = 0; i < len; ++i)
      strm << (isgraph(memory[p + i]) ? (char)memory[p + i] : '.');
    strm << endl;
    p += len;
  }

  return strm.str();
}


namespace ColumnFormatter 
{

Columns::Columns(size_t colCount)
{
  resize(colCount);
}

std::string Print(const Columns & columns, const StringVector & seps)
{
  std::vector<int> width(columns.size());
  std::vector<int> height(columns.size());

  int maxHeight = 0;
  int col = 0;
  for (auto & r : columns) {
    height[col] = r.size();
    maxHeight = std::max(maxHeight, height[col]);
    width[col] = 0;
    for (auto & s : r) {
      width[col] = std::max((int)width[col], (int)s.length());
    }
    ++col;  
  }

  std::stringstream strm;
  int row;
  for (row = 0; row < maxHeight; ++row) {
    for (col = 0; col < columns.size(); ++col) {
      if (row < columns[col].size()) {
        strm << seps[col];
        strm << columns[col][row];
        strm << std::string(width[col] - columns[col][row].length(), ' ');
      }
    }
    strm << "\n";
  }

  return strm.str();
}

std::string Print(const Columns & columns)
{
  StringVector seps(columns.size());
  bool first = true;
  for (int i = 0; i < columns.size(); ++i) {
    if (i != 0)
      seps[i] = "  ";
  }
  return Print(columns, seps);
}

std::string Print(int cols, const std::string & sep, const Columns & columns)
{
  StringVector seps(columns.size());
  for (int i = 0; i < columns.size(); ++i) {
    if (i != 0)
      seps[i] = sep;
  }
  return Print(columns, seps);
}

std::string Print(const Columns & columns, char sep)
{
  std::string strSep;
  if (isprint(sep))
    strSep += sep;

  StringVector seps(columns.size());
  for (int i = 0; i < columns.size(); ++i) {
    if (i != 0)
      seps[i] = strSep;
  }
  return Print(columns, seps);
}

/*
template <int Cols>
std::string Print(const Columns & columns, char * sepStrings[Cols])
{
  std::array<std::string, Cols> seps;
  for (int i = 0; i < Cols; ++i)
    seps[i] = std::string(sepStrings[i]);
  return Print<Cols>(columns, seps);
}
*/

} // namespace ColumnFormatter

//////////////////////////////////////////////////////////////////////

Filename::Filename()
{ }

Filename::Filename(const std::string & str)
  : std::string(str)
{ }

Filename & Filename::operator =(const std::string & str)
{ this->std::string::operator=(str); return *this; }

std::string Filename::GetDir() const
{ 
  size_t pos = find_last_of('/');
  if (pos == std::string::npos)
    return "";
  return substr(0, pos);
}

std::string Filename::GetFilename() const
{ 
  std::string fn;
  size_t pos = find_last_of('/');
  if (pos == std::string::npos)
    fn = *this;
  else
    fn = substr(pos+1);
  return fn;
}

std::string Filename::GetBasename() const
{ 
  std::string base = GetFilename();      
  size_t pos = base.find_last_of('.');      
  if (pos != std::string::npos)
    base = base.substr(0, pos);
  return base;
}

std::string Filename::GetExtension() const
{ 
  std::string ext = GetFilename();      
  size_t pos = ext.find_last_of('.');      
  if (pos == std::string::npos)
    return "";
  return ext.substr(pos);
}
