#ifndef MISC_H_
#define MISC_H_

//////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>

#define   HEXFORMAT2(val) std::hex << std::setw(2) << std::setfill('0') << ((unsigned int)(val) & 0xff) << std::dec
#define   HEXFORMAT4(val) std::hex << std::setw(4) << std::setfill('0') << ((unsigned int)(val) & 0xffff) << std::dec
#define   HEXFORMAT8(val) std::hex << std::setw(8) << std::setfill('0') << ((unsigned int)(val)) << std::dec

#define   HEXFORMAT0x2(val) "0x" << HEXFORMAT2(val)
#define   HEXFORMAT0x4(val) "0x" << HEXFORMAT4(val)
#define   HEXFORMAT0x8(val) "0x" << HEXFORMAT8(val)

#define   FIXEDFORMAT(prec, val)      std::fixed << std::setprecision(prec) << (val)
#define   FIXEDFORMAT3(val)           FIXEDFORMAT(3,val)

template<class Type>
inline void VECTOR_FILL(Type & v, const typename Type::value_type & val) { v.assign(v.size(), val); }

template<class Type>
inline void VECTOR_ZERO(Type & v) { VECTOR_FILL<Type>(v, 0); }

std::string DumpMemory(const uint8_t * memory, int memoryLen);

//////////////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <array>
#include <vector>
#include <string>

namespace ColumnFormatter 
{

template<int Cols>
using Columns = std::array<std::vector<std::string>, Cols>;

template <int Cols>
std::string Print(const Columns<Cols> & columns, const std::array<std::string, Cols> & seps)
{
  std::array<int, Cols> width;
  std::array<int, Cols> height;

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
    for (col = 0; col < Cols; ++col) {
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

template <int Cols>
std::string Print(const Columns<Cols> & columns)
{
  std::array<std::string, Cols> seps;
  bool first = true;
  for (int i = 0; i < Cols; ++i) {
    if (i != 0)
      seps[i] = "  ";
  }
  return Print<Cols>(columns, seps);
}

template <int Cols>
std::string Print(const std::string & sep, const Columns<Cols> & columns)
{
  std::array<std::string, Cols> seps;
  for (int i = 0; i < Cols; ++i) {
    if (i != 0)
      seps[i] = sep;
  }
  return Print<Cols>(columns, seps);
}

template <int Cols>
std::string Print(const Columns<Cols> & columns, char sep)
{
  std::array<std::string, Cols> seps;
  for (int i = 0; i < Cols; ++i) {
    if (i != 0)
      seps[i] = std::string(sep, 1);
  }
  return Print<Cols>(columns, seps);
}

template <int Cols>
std::string Print(const Columns<Cols> & columns, char * sepStrings[Cols])
{
  std::array<std::string, Cols> seps;
  for (int i = 0; i < Cols; ++i)
    seps[i] = std::string(sepStrings[i]);
  return Print<Cols>(columns, seps);
}

} // namespace ColumnFormatter

#endif // MISC_H_