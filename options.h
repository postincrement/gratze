#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <map>
#include <string>

struct Options
{
  std::string m_typeName;
  std::string m_romFn;
  std::map<int, std::string> m_driveFns;
  int m_ramSize_k;
  int m_breakpoint = -1;
};

#endif // OPTIONS_H_