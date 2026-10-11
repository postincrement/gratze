#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "disk/virtual_drive.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "diskdef.h"
#include "common/json_parser.h"

using namespace std;

static std::string ExecutableDir()
{
  char buf[4096];
#if defined(__APPLE__)
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0)
    return {};
#elif defined(__linux__)
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0)
    return {};
  buf[n] = '\0';
#else
  return {};
#endif
  char real[4096];
  if (realpath(buf, real) == nullptr)
    return {};
  std::string path(real);
  auto slash = path.find_last_of('/');
  if (slash == std::string::npos)
    return {};
  path.resize(slash);
  return path;
}

static bool EndsWithJson(const std::string & spec)
{
  if (spec.size() < 5)
    return false;
  std::string tail = spec.substr(spec.size() - 5);
  for (auto & ch : tail)
    ch = (char)tolower((unsigned char)ch);
  return tail == ".json";
}

static std::string ResolveDiskDefPath(const std::string & spec, std::string & error)
{
  bool direct = spec.find('/') != std::string::npos || spec.find('\\') != std::string::npos || EndsWithJson(spec);
  std::vector<std::string> candidates;
  if (direct) {
    candidates.push_back(spec);
  }
  else {
    candidates.push_back("disk/cpmdrive/dpb/" + spec + ".json");
    candidates.push_back("emulator/z80/cpm80/dpb/" + spec + ".json");
    std::string dir = ExecutableDir();
    for (int i = 0; i < 6 && !dir.empty(); ++i) {
      candidates.push_back(dir + "/disk/cpmdrive/dpb/" + spec + ".json");
      candidates.push_back(dir + "/emulator/z80/cpm80/dpb/" + spec + ".json");
      auto slash = dir.find_last_of('/');
      if (slash == std::string::npos)
        break;
      dir.resize(slash);
    }
  }

  for (const auto & candidate : candidates) {
    if (access(candidate.c_str(), R_OK) == 0)
      return candidate;
  }

  error = "unknown disk definition '" + spec + "'";
  return {};
}

static std::vector<int> BuildSkew(int spt, int skew)
{
  std::vector<int> table(spt, 0);
  std::vector<char> used(spt, 0);
  int sector = 0;
  for (int i = 0; i < spt; ++i) {
    while (used[sector])
      sector = (sector + 1) % spt;
    table[i] = sector;
    used[sector] = 1;
    sector = (sector + skew) % spt;
  }
  return table;
}

static bool CpmDiskTracks(const CpmDiskDef & disk, int & tracks);

static bool LoadDiskDefFile(const std::string & path, CpmDiskDef & disk, std::string & error)
{
  ifstream file(path);
  if (!file.is_open()) {
    error = "cannot read disk definition '" + path + "'";
    return false;
  }
  stringstream buffer;
  buffer << file.rdbuf();
  std::string text = buffer.str();

  JsonParser parser(text);
  if (!parser.Eat('{')) {
    error = "disk definition must be a JSON object";
    return false;
  }

  map<string, int> numbers;
  map<string, string> strings;
  vector<int> translate;
  bool hasSkew = false;
  bool hasTranslate = false;

  if (!parser.Eat('}')) {
    for (;;) {
      std::string key;
      if (!parser.String(key)) {
        error = parser.m_error;
        return false;
      }
      if (!parser.Eat(':')) {
        error = "expected ':' after '" + key + "'";
        return false;
      }
      if (numbers.count(key) || strings.count(key) || (key == "translate" && hasTranslate)) {
        error = "duplicate field '" + key + "'";
        return false;
      }

      if (key == "name" || key == "description" || key == "sides") {
        std::string value;
        if (!parser.String(value)) {
          error = parser.m_error;
          return false;
        }
        strings[key] = value;
      }
      else if (key == "translate") {
        if (!parser.Array(translate)) {
          error = parser.m_error;
          return false;
        }
        hasTranslate = true;
      }
      else if (key == "spt" || key == "bsh" || key == "blm" || key == "exm" ||
               key == "dsm" || key == "drm" || key == "al0" || key == "al1" ||
               key == "cks" || key == "off" || key == "skew") {
        int value = 0;
        if (!parser.Number(value)) {
          error = parser.m_error;
          return false;
        }
        numbers[key] = value;
        if (key == "skew")
          hasSkew = true;
      }
      else {
        error = "unknown disk definition field '" + key + "'";
        return false;
      }

      if (parser.Eat('}'))
        break;
      if (!parser.Eat(',')) {
        error = "expected a comma between disk definition fields";
        return false;
      }
    }
  }

  if (hasSkew && hasTranslate) {
    error = "disk definition sets both skew and translate";
    return false;
  }

  auto need = [&](const char * key, int & dest) -> bool {
    auto found = numbers.find(key);
    if (found == numbers.end()) {
      error = std::string("disk definition is missing ") + key;
      return false;
    }
    dest = found->second;
    return true;
  };

  if (!need("spt", disk.m_spt) || !need("bsh", disk.m_bsh) || !need("blm", disk.m_blm) ||
      !need("exm", disk.m_exm) || !need("dsm", disk.m_dsm) || !need("drm", disk.m_drm) ||
      !need("al0", disk.m_al0) || !need("al1", disk.m_al1) || !need("cks", disk.m_cks) ||
      !need("off", disk.m_off))
    return false;

  disk.m_name = strings.count("name") ? strings["name"] : "";
  disk.m_description = strings.count("description") ? strings["description"] : disk.m_name;

  if (!strings.count("sides")) {
    error = "disk definition is missing sides";
    return false;
  }
  std::string sides = strings["sides"];
  for (auto & ch : sides)
    ch = (char)tolower((unsigned char)ch);
  if (sides == "single")
    disk.m_sides = CpmSides::eSingle;
  else if (sides == "cylinder")
    disk.m_sides = CpmSides::eCylinder;
  else if (sides == "inout" || sides == "in-out" || sides == "outback")
    disk.m_sides = CpmSides::eInOut;
  else {
    error = "sides must be single, cylinder, or inout";
    return false;
  }

  if (disk.m_spt < 1 || disk.m_spt > 65535) {
    error = "spt is out of range";
    return false;
  }
  if (disk.m_bsh < 0 || disk.m_bsh > 7 || disk.m_blm != ((1 << disk.m_bsh) - 1)) {
    error = "bsh and blm do not describe a CP/M block size";
    return false;
  }
  if (disk.m_exm < 0 || disk.m_exm > 31 || disk.m_al0 < 0 || disk.m_al0 > 255 || disk.m_al1 < 0 || disk.m_al1 > 255) {
    error = "exm, al0, or al1 is out of range";
    return false;
  }
  if (disk.m_dsm < 0 || disk.m_dsm > 65535 || disk.m_drm < 0 || disk.m_drm > 65535 ||
      disk.m_cks < 0 || disk.m_cks > 65535 || disk.m_off < 0 || disk.m_off > 65535) {
    error = "dsm, drm, cks, or off is out of range";
    return false;
  }

  int64_t dirBytes = (int64_t)(disk.m_drm + 1) * 32;
  int64_t dataBytes = (int64_t)(disk.m_dsm + 1) * (128LL << disk.m_bsh);
  if (dirBytes > dataBytes) {
    error = "directory does not fit in the disk";
    return false;
  }

  int tracks = 0;
  if (!CpmDiskTracks(disk, tracks)) {
    error = "disk geometry is out of range";
    return false;
  }
  if (disk.m_sides != CpmSides::eSingle && (tracks % 2) != 0) {
    error = "a double-sided disk needs an even number of tracks";
    return false;
  }

  if (hasTranslate) {
    int span = (int)translate.size();
    if (span < 1 || span > disk.m_spt || (disk.m_spt % span) != 0) {
      error = "translate length must divide SPT";
      return false;
    }
    vector<char> seen(span, 0);
    for (int sector : translate) {
      if (sector < 1 || sector > span || seen[sector - 1]) {
        error = "translate repeats or omits a sector";
        return false;
      }
      seen[sector - 1] = 1;
    }
    // Sector numbers are 1-based, so sector 10 stays in the first group of a
    // 10-entry table and sector 11 starts the next group.
    disk.m_sectorIds = translate;
    disk.m_translate.resize(disk.m_spt);
    for (int logical = 0; logical < disk.m_spt; ++logical) {
      int sector = logical + 1;
      int index = (sector - 1) % span;
      int base = ((sector - 1) / span) * span;
      disk.m_translate[logical] = base + translate[index] - 1;
    }
  }
  else if (hasSkew) {
    int skew = numbers["skew"];
    if (skew < 1) {
      error = "skew must be a positive sector count";
      return false;
    }
    disk.m_translate = BuildSkew(disk.m_spt, skew);
  }
  else {
    disk.m_translate.resize(disk.m_spt);
    for (int i = 0; i < disk.m_spt; ++i)
      disk.m_translate[i] = i;
  }

  return true;
}

static bool CpmDiskTracks(const CpmDiskDef & disk, int & tracks)
{
  if (disk.m_spt < 1 || disk.m_bsh < 0 || disk.m_bsh > 7)
    return false;
  int64_t recordsPerBlock = 1LL << disk.m_bsh;
  int64_t dataRecords = (int64_t)(disk.m_dsm + 1) * recordsPerBlock;
  int64_t totalRecords = (int64_t)disk.m_off * disk.m_spt + dataRecords;
  int64_t count = (totalRecords + disk.m_spt - 1) / disk.m_spt;
  if (count < 1 || count > 65535)
    return false;
  tracks = (int)count;
  return true;
}

bool CpmDiskGeometry(const CpmDiskDef & disk, int & sectorsPerTrack, int & tracks, int & capacityKb)
{
  if (!CpmDiskTracks(disk, tracks))
    return false;
  sectorsPerTrack = disk.m_spt;
  int64_t bytes = (int64_t)tracks * disk.m_spt * 128;
  capacityKb = (int)(bytes / 1024);
  return true;
}

uint32_t CpmImageTrack(const CpmDiskDef & disk, uint32_t logicalTrack)
{
  // Cylinder order matches the image: C0H0, C0H1, C1H0, C1H1.
  if (disk.m_sides != CpmSides::eInOut)
    return logicalTrack;

  int tracks = 0;
  if (!CpmDiskTracks(disk, tracks) || tracks < 2 || logicalTrack >= (uint32_t)tracks)
    return logicalTrack;

  uint32_t cylinders = (uint32_t)tracks / 2;
  if (logicalTrack < cylinders)
    return logicalTrack * 2;
  uint32_t cylinder = (uint32_t)tracks - 1 - logicalTrack;
  return cylinder * 2 + 1;
}

bool CpmImageFits(const CpmDiskDef & disk, off_t fileSize, std::string & error)
{
  int tracks = 0;
  if (!CpmDiskTracks(disk, tracks)) {
    error = "disk definition is incomplete";
    return false;
  }
  int64_t expected = (int64_t)tracks * disk.m_spt * 128;
  if (fileSize < expected) {
    error = "image is shorter than the disk definition";
    return false;
  }
  if ((fileSize % 128) != 0) {
    error = "image length is not a multiple of 128";
    return false;
  }
  return true;
}

bool ParseCpmDriveRequest(const std::string & spec, CpmDriveRequest & out, std::string & error)
{
  if (spec.size() < 3 || spec[1] != '=') {
    error = "cpm drive '" + spec + "' is not DRIVE=dir:path or DRIVE=image:file,dpb=name";
    return false;
  }
  char letter = (char)toupper((unsigned char)spec[0]);
  if (letter < 'A' || letter > 'P') {
    error = "cpm drive letter is out of range";
    return false;
  }
  out.m_drive = letter - 'A';

  std::string rest = spec.substr(2);
  auto colon = rest.find(':');
  if (colon == std::string::npos) {
    error = "cpm drive '" + spec + "' needs dir: or image:";
    return false;
  }
  std::string kind = rest.substr(0, colon);
  for (auto & ch : kind)
    ch = (char)tolower((unsigned char)ch);
  std::string body = rest.substr(colon + 1);

  if (kind == "dir") {
    if (body.empty()) {
      error = "cpm drive " + std::string(1, letter) + " is missing a directory";
      return false;
    }
    out.m_image = false;
    out.m_path = body;
    return true;
  }

  if (kind != "image") {
    error = "cpm drive '" + spec + "' is not a directory or an image";
    return false;
  }

  auto dpbAt = body.rfind(",dpb=");
  if (dpbAt == std::string::npos || dpbAt == 0) {
    error = "image drive " + std::string(1, letter) + " needs ,dpb=name";
    return false;
  }
  out.m_image = true;
  out.m_path = body.substr(0, dpbAt);
  std::string dpbName = body.substr(dpbAt + 5);
  if (out.m_path.empty() || dpbName.empty()) {
    error = "image drive " + std::string(1, letter) + " needs a file and a dpb name";
    return false;
  }

  std::string path = ResolveDiskDefPath(dpbName, error);
  if (path.empty())
    return false;
  if (!LoadDiskDefFile(path, out.m_disk, error))
    return false;
  if (out.m_disk.m_name.empty())
    out.m_disk.m_name = dpbName;
  return true;
}

static std::string FileExtension(const std::string & path)
{
  auto pos = path.rfind('.');
  if (pos == std::string::npos)
    return {};
  std::string ext = path.substr(pos + 1);
  for (auto & ch : ext)
    ch = (char)tolower((unsigned char)ch);
  return ext;
}

static bool ImageGeometryMatches(const VirtualDrive & drive, const CpmDiskDef & disk, std::string & error)
{
  int sectorSize = drive.GetSectorSize();
  if (sectorSize < 128 || sectorSize > 1024 || (sectorSize % 128) != 0) {
    error = "disk image sector size is not a multiple of 128 bytes";
    return false;
  }
  int recordsPerSector = sectorSize / 128;
  int sectors = drive.GetSectorCount();
  if (sectors < 1 || sectors * recordsPerSector != disk.m_spt) {
    error = "disk image sectors per track do not match the disk definition";
    return false;
  }

  int tracks = 0;
  if (!CpmDiskTracks(disk, tracks)) {
    error = "disk definition is incomplete";
    return false;
  }
  int available = drive.GetSideCount() * drive.GetTrackCount();
  if (available < tracks) {
    error = "disk image does not have enough tracks";
    return false;
  }
  return true;
}

bool OpenCpmImage(const std::string & path, const CpmDiskDef & disk, std::shared_ptr<VirtualDrive> & drive, bool & raw, std::string & error)
{
  drive.reset();
  raw = false;

  // Disk formats register themselves here. gratze also calls this from main;
  // a second call replaces the same keys.
  static bool formatsReady = false;
  if (!formatsReady) {
    VirtualDrive::Init();
    formatsReady = true;
  }

  struct stat st;
  if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
    error = "cannot open CP/M image '" + path + "'";
    return false;
  }

  VirtualFileIdentifier identifier;
  std::shared_ptr<VirtualDrive> found = identifier.Open(path, true);
  if (found) {
    std::string geometryError;
    if (ImageGeometryMatches(*found, disk, geometryError)) {
      drive = found;
      return true;
    }
    std::string ext = FileExtension(path);
    std::string formatExt = found->GetExtension();
    for (auto & ch : formatExt)
      ch = (char)tolower((unsigned char)ch);
    if (ext == formatExt) {
      error = path + ": " + geometryError;
      return false;
    }
  }

  if (!CpmImageFits(disk, st.st_size, error)) {
    if (!identifier.GetError().empty())
      error = identifier.GetError();
    else
      error = path + ": " + error;
    return false;
  }
  raw = true;
  return true;
}

bool CheckCpmDrives(const std::vector<std::string> & specs, std::string & error)
{
  bool seen[16] = {};
  for (const auto & spec : specs) {
    CpmDriveRequest request;
    if (!ParseCpmDriveRequest(spec, request, error))
      return false;
    if (seen[request.m_drive]) {
      error = std::string("drive ") + char('A' + request.m_drive) + " is listed more than once";
      return false;
    }
    seen[request.m_drive] = true;

    if (request.m_image) {
      std::shared_ptr<VirtualDrive> drive;
      bool raw = false;
      if (!OpenCpmImage(request.m_path, request.m_disk, drive, raw, error))
        return false;
    }
    else {
      DIR * dir = opendir(request.m_path.c_str());
      if (dir == nullptr) {
        error = "not a directory: " + request.m_path;
        return false;
      }
      closedir(dir);
    }
  }
  return true;
}
