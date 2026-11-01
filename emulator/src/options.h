#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "common/cmdargs.h"

// One mounted drive slot. Index 0 = A … 15 = P.
struct DriveMount
{
  enum class Kind {
    eImage,  // floppy/CPM disk image file
    eDir     // host directory (cpm80)
  };

  Kind m_kind = Kind::eImage;
  std::string m_path;
  std::string m_dpb;    // optional CP/M DPB name for image mounts
  std::string m_label;  // optional status-bar text (defaults to file/dir basename)
};

struct Options
{
  CommandLineArgs m_args;

  // arguments after options (and after a leading config name, if any)
  std::vector<std::string> m_arg;

  // override default type
  std::string m_typeName;

  // use SDL
  bool m_useSDL = false;

  // true if to use game kb mapping, if available
  bool m_gameKb = false;

  // override default ROM path
  std::string m_romFn;

  // Lettered drives A–P (keys 0–15). Used by FDC emulators and filled from
  // env configs / --drivea. Image paths go here for Microbee/TRS-80.
  std::map<unsigned, DriveMount> m_drives;

  // Legacy FDC path map (A=0). Filled from m_drives images and --drive*.
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

  // Path of the environment config that was loaded, if any.
  std::string m_envConfigPath;
};

// Parse a drive suffix ("a"/"A" or "0") into 0–15. Returns false on error.
bool ParseDriveLetter(const std::string & suffix, unsigned & index, std::string & error);

// Convert m_drives into m_driveFns / m_cpmDrives for the selected emulator type.
bool MaterializeDrives(Options & options, std::string & error);

#endif // OPTIONS_H_
