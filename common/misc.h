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

std::string DumpMemory(uint16_t addr, const uint8_t * memory, int memoryLen);
std::string DumpMemory(const uint8_t * memory, int memoryLen);

//////////////////////////////////////////////////////////////////////////////////////////

#include <sstream>
#include <array>
#include <vector>
#include <string>

namespace ColumnFormatter 
{

typedef std::vector<std::string> StringVector;

struct Columns : public std::vector<StringVector> 
{
  Columns(size_t colCount);

  StringVector & operator[](size_t offs)
  {
    if (offs >= size()) {
      resize(offs+1);
    }
    return std::vector<StringVector>::operator[](offs);  
  }
  const StringVector & operator[](size_t offs) const
  {
    if (offs >= size()) {
      m_dummy.resize(0);
      return m_dummy;
    }
    return std::vector<StringVector>::operator[](offs);  
  }
  static StringVector m_dummy;
};

std::string Print(const Columns & columns, const StringVector & seps);

std::string Print(const Columns & columns);

std::string Print(int cols, const std::string & sep, const Columns & columns);

std::string Print(const Columns & columns, char sep);

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

//////////////////////////////////////////////////////////////////

class Filename : public std::string
{
  public:
    Filename();
    Filename(const std::string & str);

    Filename & operator =(const std::string & str);

    std::string GetDir() const;
    std::string GetFilename() const;
    std::string GetBasename() const;
    std::string GetExtension() const;
};

#endif // MISC_H_