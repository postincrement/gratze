#include "src/options.h"

#include <cctype>
#include <sstream>

bool ParseDriveLetter(const std::string & suffix, unsigned & index, std::string & error)
{
  return CommandLineArgs::ParseDriveSuffix(suffix, index, error);
}

static bool IsCpm80Type(const std::string & typeName)
{
  return typeName == "cpm80";
}

bool MaterializeDrives(Options & options, std::string & error)
{
  // m_drives from env config / structured CLI wins over empty legacy maps.
  for (const auto & entry : options.m_drives) {
    unsigned index = entry.first;
    if (index > 15) {
      error = "drive index out of range";
      return false;
    }
    char letter = (char)('A' + index);
    const DriveMount & mount = entry.second;

    if (IsCpm80Type(options.m_typeName)) {
      std::string spec;
      if (mount.m_kind == DriveMount::Kind::eDir) {
        spec = std::string(1, letter) + "=dir:" + mount.m_path;
      }
      else {
        spec = std::string(1, letter) + "=image:" + mount.m_path;
        if (!mount.m_dpb.empty())
          spec += ",dpb=" + mount.m_dpb;
      }
      options.m_cpmDrives.push_back(spec);
    }
    else {
      if (mount.m_kind == DriveMount::Kind::eDir) {
        error = std::string("drive ") + (char)tolower(letter)
              + ": directory mounts are only supported for cpm80";
        return false;
      }
      // Config fills gaps; explicit --drive* applied afterward may overwrite.
      if (!options.m_driveFns.count(index))
        options.m_driveFns[index] = mount.m_path;
    }
  }

  return true;
}
