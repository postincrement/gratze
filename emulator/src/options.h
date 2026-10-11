#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "common/cmdargs.h"

struct Options
{
  CommandLineArgs m_args;

  // arguments after options
  std::vector<std::string> m_arg;

  // override default type
  std::string m_typeName;

  // use SDL
  bool m_useSDL = false;

  // true if to use game kb mapping, if available
  bool m_gameKb = false;

  // override default ROM path
  std::string m_romFn;

  // map of disk drive information
  std::map<unsigned, std::string> m_driveFns;

  // CP/M drive map. Each entry is A=dir:path or A=image:file,dpb=name.
  std::vector<std::string> m_cpmDrives;

  // override default RAM size
  unsigned m_ramSize_k = 0;

  // enable expansion interface (for Model I)
  bool m_withEI = false;

  // use TTF font
  std::string m_font;
  unsigned m_fontSize = -1;

  // debug display options
  bool m_readMemory = false;
  bool m_writeMemory = false;
  bool m_readVideo = false;
  bool m_writeVideo = false;
  bool m_readIO = false;
  bool m_writeIO = false;
  bool m_fdcDebug = false;
  bool m_keyboardDebug = false;

  // set breakpoint (not used yet)
  int m_breakpoint = -1;

  // set trace
  bool m_trace = false;

  // scale video. 0 chooses a scale that fits the desktop.
  unsigned m_videoScale = 0;

  // level of vebosity
  unsigned m_verbose = 0;

  // do not limit CPU speed
  bool m_turbo = false;

  // maintain backtrace queue
  unsigned m_traceLength = 0;

  // display the CPU speed
  bool m_displayCPUSpeed = false;

  // log program counter
  bool m_logPC = false;

  // Starnet workstation slot. Negative asks the server to assign one.
  int m_starnetStation = -1;
};

#endif // OPTIONS_H_
