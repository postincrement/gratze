#ifndef GRZ_H
#define GRZ_H

#include <string>
#include <sstream>
#include <iomanip>

#include "cereal.h"

struct GRZ
{
  unsigned m_value;

  template<class Archive>
  void load(Archive & ar)
  {
    std::string addrStr;     // memory address to load at
    std::string execStr;     // memory address to exec at
    std::string offsStr;     // position within file to start
    std::string lengthStr;   // length from offset to read;

    ar(
      cereal::make_nvp("fn",              m_filename),
      cereal::make_nvp("addr",            addrStr)
    );
    cereal::make_optional_nvp(ar, "exec",   execStr, ""),
    cereal::make_optional_nvp(ar, "offs",   offsStr, ""),
    cereal::make_optional_nvp(ar, "length", lengthStr, ""),

    m_addr   = FromHex(addrStr);
    m_exec   = FromHex(execStr,   m_hasExec);
    m_offs   = FromHex(offsStr,   m_hasOffs);
    m_length = FromHex(lengthStr, m_hasLength);
  }

  template<class Archive>
  void save(Archive & ar) const
  {
    std::string addrStr = ToHex(m_addr);

    std::string execStr;
    if (m_hasExec)
      execStr = ToHex(m_exec);

    std::string offsStr;
    if (m_hasOffs)
      offsStr = ToHex(m_offs);

    std::string lengthStr;
    if (m_hasLength)
      lengthStr = ToHex(m_length);

    ar(
      cereal::make_nvp("fn",     m_filename),
      cereal::make_nvp("addr",   addrStr),
      cereal::make_nvp("exec",   execStr),
      cereal::make_nvp("offs",   offsStr),
      cereal::make_nvp("length", lengthStr)
    );
  }

  std::string ToHex(unsigned val) const
  {
    std::stringstream strm;
    strm << "0x" << std::setw(4) << std::setfill('0') << std::hex << val;
    return strm.str();
  }

  unsigned FromHex(const std::string & val) const
  {
    bool has;
    return FromHex(val, has);
  }

  unsigned FromHex(const std::string & str_, bool & has) const
  {
    has = false;
    if (str_.length() < 3)
      return 0;
    if (str_.length() < 2)
      return 0;
    std::string str(str_);
    for (auto & r : str) r = tolower(r);
    if ((str[0] != '0') && (str[1] != 'x'))
      return 0;
    char * ptr;
    unsigned v = strtoul(&str[2], &ptr, 16);
    has = (ptr != NULL);
    return v;  
  }

  std::string m_filename; // filename to load
  unsigned m_addr;        // memory address to load at

  unsigned m_exec;        // memory address to exec at
  bool m_hasExec = false;

  unsigned m_offs;        // position within file to start
  bool m_hasOffs = false;

  unsigned m_length;      // length from offset to read;
  bool m_hasLength = false;

  protected:
};

#endif // GRZ_H