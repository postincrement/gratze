#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <map>
#include <string>

struct Options
{
  // override default type
  std::string m_typeName;

  // override default ROM path
  std::string m_romFn;

  // map of disk drive information
  std::map<int, std::string> m_driveFns;

  // override default RAM size
  int m_ramSize_k = -1;

  // enable expansion interface (for Model I)
  bool m_withEI = false;

  // set breakpoint (not used yet)
  int m_breakpoint = -1;
};

#endif // OPTIONS_H_