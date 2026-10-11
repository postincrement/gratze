#include "src/options.h"

#include <cctype>
#include <sstream>

bool ParseDriveLetter(const std::string & suffix, unsigned & index, std::string & error)
{
  return CommandLineArgs::ParseDriveSuffix(suffix, index, error);
}

static bool UsesEmulatedBdos(const Options & options)
{
  return options.m_typeName == "cpm80";
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

    if (options.m_shadowBdos) {
      // Boot drive A must be an image: guest BIOS needs reserved tracks.
      // Interceptor NewBDOS only serves directory mounts on B–P.
      if (mount.m_kind == DriveMount::Kind::eDir) {
        if (index == 0) {
          error = "drive a: boot drive cannot be a host directory when shadowBdos is enabled "
                  "(no reserved tracks for boot sectors); use an image on a: and dir on b:-p:";
          return false;
        }
        options.m_cpmDrives.push_back(std::string(1, letter) + "=dir:" + mount.m_path);
      }
      else if (!options.m_driveFns.count(index)) {
        options.m_driveFns[index] = mount.m_path;
      }
    }
    else if (UsesEmulatedBdos(options)) {
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
              + ": directory mounts require shadowBdos or cpm80";
        return false;
      }
      // Config fills gaps; explicit --drive* applied afterward may overwrite.
      if (!options.m_driveFns.count(index))
        options.m_driveFns[index] = mount.m_path;
    }
  }

  if (options.m_shadowBdos) {
    auto boot = options.m_drives.find(0);
    bool bootIsImage = (boot != options.m_drives.end()
                        && boot->second.m_kind == DriveMount::Kind::eImage)
                    || options.m_driveFns.count(0);
    if (!bootIsImage) {
      error = "shadowBdos requires boot drive a: to be a disk image "
              "(host directories have no reserved tracks for boot sectors)";
      return false;
    }
  }

  return true;
}
