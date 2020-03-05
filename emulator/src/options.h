#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <map>
#include <string>

#include "common/cmdargs.h"

struct Options
{
  CommandLineArgs m_args;

  // override default type
  std::string m_typeName;

  // override default ROM path
  std::string m_romFn;

  // map of disk drive information
  std::map<unsigned, std::string> m_driveFns;

  // override default RAM size
  unsigned m_ramSize_k = 0;

  // enable expansion interface (for Model I)
  bool m_withEI = false;

  // use TTF font
  std::string m_font;
  unsigned m_fontSize = -1;

  bool m_readMemory = false;
  bool m_writeMemory = false;
  bool m_readVideo = false;
  bool m_writeVideo = false;

  // set breakpoint (not used yet)
  int m_breakpoint = -1;

  // scale video
  int m_videoScale = 1;

  // level of vebosity
  unsigned m_verbose = 0;

  // true if to display keyboard debugging
  bool m_keyboardDebug = false;

  // do not limit CPU speed
  bool m_turbo = false;
};

#endif // OPTIONS_H_
