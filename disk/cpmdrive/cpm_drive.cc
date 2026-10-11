#include "cpm_drive.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <algorithm>
#include <iostream>

#include <dirent.h>
#include <unistd.h>

#include "common/config.h"
#include "disk/virtual_drive.h"

namespace {

void Replace(std::string & str, char from, char to)
{
  size_t pos = 0;
  while ((pos = str.find(from)) != std::string::npos)
    str[pos] = to;
}

std::string ShortenFilename(const std::string & in)
{
  std::string str(in);
  for (auto & ch : str)
    ch = (char)toupper((unsigned char)ch);
  Replace(str, '[', '_');
  Replace(str, ']', '_');
  Replace(str, ',', '_');

  size_t pos = str.rfind('.');
  std::string extension;
  if (pos == std::string::npos) {
    pos = str.length();
  }
  else {
    size_t extPos = pos + 1;
    size_t extLen = str.size() - extPos;
    if (extLen > 3)
      extLen = 3;
    extension = str.substr(extPos, extLen);
  }

  if (pos > 8)
    pos = 8;
  str = str.substr(0, pos);
  while (str.length() < 8)
    str += ' ';
  Replace(str, '.', '_');
  while (extension.length() < 3)
    extension += ' ';
  Replace(extension, '.', '_');
  str += extension;
  return str;
}

int HostBlockSize(long long diskBytes)
{
  const long long mb = 1024ll * 1024ll;
  if (diskBytes <= 16 * mb)
    return 4096;
  if (diskBytes <= 64 * mb)
    return 8192;
  return 16384;
}

int HostExtentCount(long long size, int recordsPerEntry)
{
  long long records = (size + 127) / 128;
  if (records > 65536)
    records = 65536;
  if (records <= 0)
    return 1;
  return (int)((records + recordsPerEntry - 1) / recordsPerEntry);
}

void StoreBlock(uint8_t * entry, int index, int block, bool words)
{
  if (words) {
    entry[16 + index * 2] = (uint8_t)(block & 0xff);
    entry[17 + index * 2] = (uint8_t)((block >> 8) & 0xff);
  }
  else {
    entry[16 + index] = (uint8_t)(block & 0xff);
  }
}

void MapCylinder(const CpmDiskDef & disk, uint32_t logicalTrack, int & side, int & cylinder)
{
  side = 0;
  cylinder = (int)logicalTrack;
  if (disk.m_sides == CpmSides::eSingle)
    return;

  int sectors = 0;
  int tracks = 0;
  int capacityKb = 0;
  if (!CpmDiskGeometry(disk, sectors, tracks, capacityKb) || tracks < 2)
    return;

  uint32_t cylinders = (uint32_t)tracks / 2;
  if (disk.m_sides == CpmSides::eCylinder) {
    side = (int)(logicalTrack & 1);
    cylinder = (int)(logicalTrack >> 1);
    return;
  }
  if (logicalTrack < cylinders) {
    side = 0;
    cylinder = (int)logicalTrack;
  }
  else {
    side = 1;
    cylinder = (int)((uint32_t)tracks - 1 - logicalTrack);
  }
}

int Allocation(const uint8_t * entry, int index, bool words)
{
  if (words) {
    if (index < 0 || index >= 8)
      return 0;
    return entry[16 + index * 2] | (entry[17 + index * 2] << 8);
  }
  if (index < 0 || index >= 16)
    return 0;
  return entry[16 + index];
}

void SetAllocBit(std::vector<uint8_t> & bits, int block)
{
  if (block < 0)
    return;
  size_t index = (size_t)block / 8;
  if (index >= bits.size())
    return;
  bits[index] |= (uint8_t)(0x80 >> (block % 8));
}

} // namespace

CpmDrive::CpmDrive() = default;

CpmDrive::~CpmDrive()
{
  ReleaseImage();
}

CpmDrive::CpmDrive(CpmDrive && other) noexcept
{
  *this = std::move(other);
}

CpmDrive & CpmDrive::operator=(CpmDrive && other) noexcept
{
  if (this == &other)
    return *this;
  ReleaseImage();
  m_kind = other.m_kind;
  m_configured = other.m_configured;
  m_path = std::move(other.m_path);
  m_detail = std::move(other.m_detail);
  m_host = std::move(other.m_host);
  m_disk = std::move(other.m_disk);
  m_directory = std::move(other.m_directory);
  m_alloc = std::move(other.m_alloc);
  m_index = other.m_index;
  m_fd = other.m_fd;
  other.m_fd = -1;
  m_image = std::move(other.m_image);
  m_blockValid = other.m_blockValid;
  m_blockSide = other.m_blockSide;
  m_blockCylinder = other.m_blockCylinder;
  m_blockId = other.m_blockId;
  m_block = std::move(other.m_block);
  m_blocks = std::move(other.m_blocks);
  other.m_blockValid = false;
  other.m_configured = false;
  other.m_kind = Kind::eHost;
  return *this;
}

void CpmDrive::SetIndex(int index)
{
  m_index = index;
}

void CpmDrive::ReleaseImage()
{
  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
  m_image.reset();
  m_blockValid = false;
  m_block.clear();
}

void CpmDrive::MountHost(const std::string & path)
{
  ReleaseImage();
  m_kind = Kind::eHost;
  m_configured = true;
  m_path = path;
  m_detail.clear();
  ClearHostCache();
}

void CpmDrive::SetHostRoot(const std::string & path)
{
  MountHost(path);
}

void CpmDrive::ClearHostCache()
{
  m_host.m_cpmToNative.clear();
  m_host.m_nativeToCPM.clear();
  m_host.m_dir.clear();
  m_directory.clear();
  m_alloc.clear();
  m_blocks.clear();
  m_disk = CpmDiskDef();
  m_blockValid = false;
  m_block.clear();
}

bool CpmDrive::MountImage(const CpmDriveRequest & request, std::string & error, std::ostream * debug)
{
  std::shared_ptr<VirtualDrive> image;
  bool raw = false;
  if (!OpenCpmImage(request.m_path, request.m_disk, image, raw, error))
    return false;

  ReleaseImage();
  m_kind = Kind::eImage;
  m_configured = true;
  m_path = request.m_path;
  m_detail = request.m_disk.m_description.empty() ? request.m_disk.m_name : request.m_disk.m_description;
  m_disk = request.m_disk;
  ClearHostCache();
  // ClearHostCache wipes the disk definition. Put it back.
  m_disk = request.m_disk;
  m_configured = true;
  m_kind = Kind::eImage;
  m_path = request.m_path;

  if (image) {
    VirtualFileIdentifier identifier;
    std::shared_ptr<VirtualDrive> writable = identifier.Open(request.m_path, false);
    m_image = writable ? writable : image;
    if (debug)
      *debug << "info: drive " << (char)('A' + request.m_drive) << ": " << m_image->GetFormat() << " image" << std::endl;
  }
  else if (raw) {
    m_fd = ::open(request.m_path.c_str(), O_RDWR | O_BINARY);
    if (m_fd < 0)
      m_fd = ::open(request.m_path.c_str(), O_RDONLY | O_BINARY);
    if (m_fd < 0) {
      error = "cannot open CP/M image '" + request.m_path + "'";
      m_kind = Kind::eHost;
      m_configured = false;
      m_disk = CpmDiskDef();
      return false;
    }
  }

  int records = ((m_disk.m_drm + 1) * 32 + 127) / 128;
  m_directory.assign((size_t)records * 128, 0xe5);
  for (int i = 0; i < records; ++i) {
    uint32_t diskRec = (uint32_t)m_disk.m_off * (uint32_t)m_disk.m_spt + (uint32_t)i;
    if (!ReadRecord(diskRec, m_directory.data() + i * 128)) {
      error = "cannot read the directory of '" + request.m_path + "'";
      ReleaseImage();
      m_kind = Kind::eHost;
      m_configured = false;
      m_directory.clear();
      m_disk = CpmDiskDef();
      return false;
    }
  }
  return true;
}

bool CpmDrive::Refresh(std::ostream * debug)
{
  if (m_kind == Kind::eImage)
    return true;

  std::string root = "./";
  bool appendLetter = m_index != 0;
  if (m_configured) {
    root = m_path;
    appendLetter = false;
  }

  char * canonicalPath = nullptr;
#if defined(__linux__) || defined(__APPLE__)
  canonicalPath = realpath(root.c_str(), nullptr);
#endif
#if defined(__WIN32)
  canonicalPath = _fullpath(nullptr, root.c_str(), 0);
#endif
  if (canonicalPath == nullptr) {
    if (debug)
      *debug << "cannot get real path for '" << root << "'" << std::endl;
    m_directory.clear();
    m_disk = CpmDiskDef();
    m_blocks.clear();
    return false;
  }

  if (debug)
    *debug << "'" << root << "' resolved to '" << canonicalPath << "'" << std::endl;
  std::string path(canonicalPath);
  free(canonicalPath);

  if (!path.empty() && path.back() != DIR_SEPERATOR)
    path += DIR_SEPERATOR;
  if (appendLetter) {
    path += (char)('a' + m_index);
    path += DIR_SEPERATOR;
  }

  m_host.m_cpmToNative.clear();
  m_host.m_nativeToCPM.clear();
  m_host.m_dir = path;

  DIR * dir = opendir(path.c_str());
  if (dir == nullptr) {
    m_directory.clear();
    m_disk = CpmDiskDef();
    m_blocks.clear();
    return false;
  }

  std::vector<ListedFile> listed;
  long long totalBytes = 0;
  for (;;) {
    dirent * dirEnt = readdir(dir);
    if (dirEnt == nullptr)
      break;
    const char * fn = dirEnt->d_name;
    if (strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0)
      continue;
    if (fn[0] == '.')
      continue;

    struct stat attr;
    std::string full = path + fn;
    if (stat(full.c_str(), &attr) != 0) {
      if (debug)
        *debug << "error: stat error " << strerror(errno) << " - " << full << std::endl;
      continue;
    }
    bool isDir = S_ISDIR(attr.st_mode);
    if (!isDir && !S_ISREG(attr.st_mode))
      continue;

    std::string cpmFilename = ShortenFilename(fn);
    if (isDir) {
      cpmFilename[8] = 'D';
      cpmFilename[9] = 'I';
      cpmFilename[10] = 'R';
    }

    int index = 1;
    while (m_host.m_cpmToNative.count(cpmFilename) > 0) {
      if (index < 10)
        cpmFilename[7] = (char)('0' + index);
      else if (index < 100) {
        cpmFilename[7] = (char)('0' + (index % 10));
        cpmFilename[6] = (char)('0' + (index / 10));
      }
      else if (index < 1000) {
        cpmFilename[7] = (char)('0' + (index % 10));
        cpmFilename[6] = (char)('0' + ((index / 10) % 10));
        cpmFilename[5] = (char)('0' + (index / 100));
      }
      else
        break;
      ++index;
    }
    if (index >= 1000) {
      if (debug)
        *debug << "warning: exhausted suffixes to disambiguate '" << fn << "'" << std::endl;
      continue;
    }

    m_host.m_cpmToNative[cpmFilename] = fn;
    m_host.m_nativeToCPM[fn] = cpmFilename;
    long long bytes = (!isDir && attr.st_size > 0) ? (long long)attr.st_size : 0;
    listed.push_back(ListedFile{cpmFilename, full, bytes});
    totalBytes += bytes;
    if (debug)
      *debug << "native '" << fn << "' mapped to CP/M '" << cpmFilename << "'" << std::endl;
  }
  closedir(dir);
  BuildHostVolume(listed, totalBytes, debug);
  if (debug)
    *debug << "finished mapping" << std::endl;
  return true;
}

void CpmDrive::BuildHostVolume(const std::vector<ListedFile> & files, long long totalBytes, std::ostream * debug)
{
  const long long mb = 1024ll * 1024ll;
  long long need = totalBytes * 2;
  if (need < 16 * mb)
    need = 16 * mb;

  int blockSize = 4096;
  int blocks = 4096;
  int exm = 1;
  int dirEntries = 128;
  for (int pass = 0; pass < 6; ++pass) {
    blockSize = HostBlockSize(need);
    long long maxDisk = 65536ll * blockSize;
    long long diskBytes = need > maxDisk ? maxDisk : need;
    blocks = (int)((diskBytes + blockSize - 1) / blockSize);
    if (blockSize == 4096)
      blocks = (blocks + 3) & ~3;
    else if (blockSize == 8192)
      blocks = (blocks + 1) & ~1;
    if (blocks > 65536)
      blocks = 65536;
    if (m_maxBlocks > 0 && blocks > m_maxBlocks)
      blocks = m_maxBlocks;
    if (blocks < 1)
      blocks = 1;

    exm = (blockSize / 1024) - 1;
    if (blocks >= 256)
      exm >>= 1;
    int recordsPerEntry = (exm + 1) * 128;
    int extents = 0;
    for (const auto & file : files)
      extents += HostExtentCount(file.m_size, recordsPerEntry);

    dirEntries = extents < 128 ? 128 : extents;
    dirEntries = (dirEntries + 3) & ~3;
    int maxEntries = (16 * blockSize / 32) & ~3;
    if (dirEntries > maxEntries)
      dirEntries = maxEntries;

    int dirBlocks = (dirEntries * 32 + blockSize - 1) / blockSize;
    long long data = 0;
    for (const auto & file : files) {
      long long size = file.m_size;
      if (size > 8 * mb)
        size = 8 * mb;
      if (size > 0)
        data += (size + blockSize - 1) / blockSize * blockSize;
    }
    long long used = data + (long long)dirBlocks * blockSize;
    long long have = (long long)blocks * blockSize;
    if (have >= used && extents <= dirEntries)
      break;

    long long next = used;
    if (extents > dirEntries && blockSize < 16384)
      next = (blockSize == 4096) ? 16 * mb + 1 : 64 * mb + 1;
    if (next <= need)
      next = need + blockSize;
    need = next;
  }

  int bsh = 0;
  for (int size = blockSize; size > 128; size >>= 1)
    ++bsh;

  m_disk = CpmDiskDef();
  m_disk.m_name = "host";
  m_disk.m_spt = 128;
  m_disk.m_bsh = bsh;
  m_disk.m_blm = (1 << bsh) - 1;
  m_disk.m_exm = exm;
  m_disk.m_dsm = blocks - 1;
  m_disk.m_drm = dirEntries - 1;
  m_disk.m_off = 0;
  m_disk.m_cks = 0;
  m_disk.m_sides = CpmSides::eSingle;
  int dirBlocks = (dirEntries * 32 + blockSize - 1) / blockSize;
  if (dirBlocks > 16)
    dirBlocks = 16;
  uint16_t mask = 0;
  for (int i = 0; i < dirBlocks; ++i)
    mask |= (uint16_t)(0x8000 >> i);
  m_disk.m_al0 = (mask >> 8) & 0xff;
  m_disk.m_al1 = mask & 0xff;
  m_disk.m_translate.resize(m_disk.m_spt);
  for (int i = 0; i < m_disk.m_spt; ++i)
    m_disk.m_translate[i] = i;

  m_directory.assign((size_t)dirEntries * 32, 0xe5);
  m_blocks.clear();
  m_blocks.resize((size_t)dirBlocks);
  int recordsPerBlock = 1 << bsh;
  int recordsPerEntry = (exm + 1) * 128;
  bool words = m_disk.m_dsm >= 256;
  int slots = words ? 8 : 16;
  int nextBlock = dirBlocks;
  int nextEntry = 0;

  for (const auto & file : files) {
    long long records = (file.m_size + 127) / 128;
    if (records > 65536)
      records = 65536;
    int remaining = (int)records;
    int base = 0;
    bool once = true;
    while ((remaining > 0 || once) && nextEntry < dirEntries) {
      once = false;
      int recs = remaining > recordsPerEntry ? recordsPerEntry : remaining;
      long long placed = records - remaining;
      uint8_t * entry = m_directory.data() + nextEntry * 32;
      memset(entry, 0, 32);
      entry[0] = 0;
      for (int i = 0; i < 11; ++i)
        entry[1 + i] = (i < (int)file.m_name.size()) ? (uint8_t)file.m_name[i] : (uint8_t)' ';

      int logicals = recs == 0 ? 0 : (recs + 127) / 128;
      int last = base + (logicals == 0 ? 0 : logicals - 1);
      if (recs == recordsPerEntry) {
        last = base + exm;
        entry[15] = 128;
      }
      else if (recs == 0) {
        entry[15] = 0;
      }
      else {
        int rc = recs % 128;
        entry[15] = (uint8_t)(rc == 0 ? 128 : rc);
      }
      entry[12] = (uint8_t)(last & 0x1f);
      entry[14] = (uint8_t)((last >> 5) & 0x3f);

      int nblocks = recs == 0 ? 0 : (recs + recordsPerBlock - 1) / recordsPerBlock;
      if (nblocks > slots)
        nblocks = slots;
      for (int b = 0; b < nblocks; ++b) {
        if (nextBlock > m_disk.m_dsm)
          break;
        StoreBlock(entry, b, nextBlock, words);
        if ((int)m_blocks.size() <= nextBlock)
          m_blocks.resize((size_t)nextBlock + 1);
        m_blocks[(size_t)nextBlock].m_path = file.m_native;
        m_blocks[(size_t)nextBlock].m_offset = (uint32_t)(placed * 128 + (long long)b * blockSize);
        ++nextBlock;
      }

      ++nextEntry;
      remaining -= recs;
      base += exm + 1;
      if (recs == 0)
        break;
    }
  }

  if (debug) {
    *debug << "host disk " << (blockSize / 1024) << "kb blocks, "
           << ((long long)blocks * blockSize / 1024) << "kb, "
           << files.size() << " files, " << totalBytes << " bytes" << std::endl;
  }
}

void CpmDrive::Dpb(uint8_t bytes[15]) const
{
  memset(bytes, 0, 15);
  const CpmDiskDef & disk = m_disk;
  bytes[0] = (uint8_t)(disk.m_spt & 0xff);
  bytes[1] = (uint8_t)((disk.m_spt >> 8) & 0xff);
  bytes[2] = (uint8_t)disk.m_bsh;
  bytes[3] = (uint8_t)disk.m_blm;
  bytes[4] = (uint8_t)disk.m_exm;
  bytes[5] = (uint8_t)(disk.m_dsm & 0xff);
  bytes[6] = (uint8_t)((disk.m_dsm >> 8) & 0xff);
  bytes[7] = (uint8_t)(disk.m_drm & 0xff);
  bytes[8] = (uint8_t)((disk.m_drm >> 8) & 0xff);
  bytes[9] = (uint8_t)disk.m_al0;
  bytes[10] = (uint8_t)disk.m_al1;
  bytes[11] = (uint8_t)(disk.m_cks & 0xff);
  bytes[12] = (uint8_t)((disk.m_cks >> 8) & 0xff);
  bytes[13] = (uint8_t)(disk.m_off & 0xff);
  bytes[14] = (uint8_t)((disk.m_off >> 8) & 0xff);
}

bool CpmDrive::ReadRecord(uint32_t record, uint8_t * dest)
{
  if (m_kind == Kind::eImage)
    return ReadImage(record, dest);
  return ReadHost(record, dest);
}

bool CpmDrive::WriteRecord(uint32_t record, const uint8_t * src)
{
  if (m_kind == Kind::eImage)
    return WriteImage(record, src);
  return WriteHost(record, src);
}

bool CpmDrive::HostRecord(uint32_t record, uint32_t & dataRec, int & block, int & within) const
{
  if (m_disk.m_spt <= 0 || m_disk.m_bsh < 0)
    return false;
  uint32_t recordsPerBlock = 1u << m_disk.m_bsh;
  uint32_t capacity = (uint32_t)m_disk.m_off * (uint32_t)m_disk.m_spt
                    + (uint32_t)(m_disk.m_dsm + 1) * recordsPerBlock;
  if (record >= capacity)
    return false;
  uint32_t reserved = (uint32_t)m_disk.m_off * (uint32_t)m_disk.m_spt;
  if (record < reserved) {
    dataRec = 0;
    block = -1;
    within = -1;
    return true;
  }
  dataRec = record - reserved;
  block = (int)(dataRec >> m_disk.m_bsh);
  within = (int)(dataRec & (uint32_t)m_disk.m_blm);
  return true;
}

bool CpmDrive::ReadHost(uint32_t record, uint8_t * dest)
{
  uint32_t dataRec = 0;
  int block = 0;
  int within = 0;
  if (!HostRecord(record, dataRec, block, within))
    return false;
  if (block < 0) {
    memset(dest, 0xe5, 128);
    return true;
  }
  size_t dirRecords = m_directory.size() / 128;
  if (dataRec < dirRecords) {
    memcpy(dest, m_directory.data() + (size_t)dataRec * 128, 128);
    return true;
  }
  if (block >= (int)m_blocks.size() || m_blocks[(size_t)block].m_path.empty()) {
    memset(dest, 0xe5, 128);
    return true;
  }
  const HostBlock & mapped = m_blocks[(size_t)block];
  int fd = ::open(mapped.m_path.c_str(), O_RDONLY | O_BINARY);
  if (fd < 0) {
    memset(dest, 0xe5, 128);
    return true;
  }
  off_t at = (off_t)mapped.m_offset + (off_t)within * 128;
  ssize_t n = ::pread(fd, dest, 128, at);
  ::close(fd);
  if (n < 0)
    return false;
  if (n < 128)
    memset(dest + n, 0x1a, (size_t)(128 - n));
  return true;
}

bool CpmDrive::WriteHost(uint32_t record, const uint8_t * src)
{
  uint32_t dataRec = 0;
  int block = 0;
  int within = 0;
  if (!HostRecord(record, dataRec, block, within))
    return false;
  if (block < 0)
    return false;
  size_t dirRecords = m_directory.size() / 128;
  if (dataRec < dirRecords) {
    memcpy(m_directory.data() + (size_t)dataRec * 128, src, 128);
    return true;
  }
  // Only blocks the volume builder assigned to a host file can be written.
  // A new allocation has no file to receive the bytes.
  if (block >= (int)m_blocks.size() || m_blocks[(size_t)block].m_path.empty())
    return false;
  const HostBlock & mapped = m_blocks[(size_t)block];
  int fd = ::open(mapped.m_path.c_str(), O_RDWR | O_BINARY);
  if (fd < 0)
    return false;
  off_t at = (off_t)mapped.m_offset + (off_t)within * 128;
  ssize_t n = ::pwrite(fd, src, 128, at);
  ::close(fd);
  return n == 128;
}

bool CpmDrive::LoadImageSector(uint32_t diskRec, int & offset)
{
  const CpmDiskDef & disk = m_disk;
  if (!m_image || disk.m_spt <= 0)
    return false;
  uint32_t logicalTrack = diskRec / (uint32_t)disk.m_spt;
  uint32_t logical = diskRec % (uint32_t)disk.m_spt;
  if (logical >= disk.m_translate.size())
    return false;

  int side = 0;
  int cylinder = 0;
  MapCylinder(disk, logicalTrack, side, cylinder);

  int sectorSize = m_image->GetSectorSize();
  if (sectorSize < 128 || (sectorSize % 128) != 0)
    return false;
  int recordsPerSector = sectorSize / 128;
  if ((disk.m_spt % recordsPerSector) != 0)
    return false;

  int sectorId = 0;
  int physicalSectors = disk.m_spt / recordsPerSector;
  int span = (int)disk.m_sectorIds.size();
  if (span > 0 && (physicalSectors % span) == 0) {
    int physical = (int)logical / recordsPerSector;
    offset = ((int)logical % recordsPerSector) * 128;
    int index = physical % span;
    int base = (physical / span) * span;
    sectorId = base + disk.m_sectorIds[index];
  }
  else {
    uint32_t record = disk.m_translate[logical];
    sectorId = (int)(record / recordsPerSector) + 1;
    offset = (int)(record % recordsPerSector) * 128;
  }

  const VirtualDrive::TrackInfo * track = m_image->GetTrack(side, cylinder);
  if (track == nullptr)
    return false;
  const VirtualDrive::SectorInfo * info = track->GetSector(sectorId);
  if (info == nullptr && sectorId > 0)
    info = track->GetSector(sectorId - 1);
  if (info == nullptr && sectorId > 0 && sectorId - 1 < track->GetSectorCount())
    info = &track->GetSectors()[sectorId - 1];
  if (info == nullptr)
    return false;

  bool cached = m_blockValid
    && m_blockSide == side
    && m_blockCylinder == cylinder
    && m_blockId == info->m_id
    && (int)m_block.size() >= offset + 128;
  if (!cached) {
    m_blockValid = false;
    m_block.assign((size_t)sectorSize, 0);
    VirtualDrive::SectorInfo got;
    int n = m_image->ReadSector(side, cylinder, info->m_id, got, m_block.data(), sectorSize);
    if (n < offset + 128)
      return false;
    if (n < sectorSize)
      m_block.resize((size_t)n);
    m_blockSide = side;
    m_blockCylinder = cylinder;
    m_blockId = info->m_id;
    m_blockValid = true;
  }
  return true;
}

bool CpmDrive::ReadImage(uint32_t record, uint8_t * dest)
{
  if (m_image) {
    int offset = 0;
    if (!LoadImageSector(record, offset))
      return false;
    memcpy(dest, m_block.data() + offset, 128);
    return true;
  }
  if (m_fd < 0 || m_disk.m_spt <= 0)
    return false;
  uint32_t track = record / (uint32_t)m_disk.m_spt;
  uint32_t logical = record % (uint32_t)m_disk.m_spt;
  if (logical >= m_disk.m_translate.size())
    return false;
  uint32_t physical = (uint32_t)m_disk.m_translate[logical];
  uint32_t imageTrack = CpmImageTrack(m_disk, track);
  off_t byteOff = ((off_t)imageTrack * m_disk.m_spt + physical) * 128;
  ssize_t n = ::pread(m_fd, dest, 128, byteOff);
  if (n < 0)
    return false;
  if (n < 128)
    memset(dest + n, 0x1a, (size_t)(128 - n));
  return n > 0;
}

bool CpmDrive::WriteImage(uint32_t record, const uint8_t * src)
{
  if (m_image) {
    int offset = 0;
    if (!LoadImageSector(record, offset))
      return false;
    memcpy(m_block.data() + offset, src, 128);
    int n = m_image->WriteSector(m_blockSide, m_blockCylinder, m_blockId, m_block.data(), (int)m_block.size());
    return n >= offset + 128;
  }
  if (m_fd < 0 || m_disk.m_spt <= 0)
    return false;
  uint32_t track = record / (uint32_t)m_disk.m_spt;
  uint32_t logical = record % (uint32_t)m_disk.m_spt;
  if (logical >= m_disk.m_translate.size())
    return false;
  uint32_t physical = (uint32_t)m_disk.m_translate[logical];
  uint32_t imageTrack = CpmImageTrack(m_disk, track);
  off_t byteOff = ((off_t)imageTrack * m_disk.m_spt + physical) * 128;
  ssize_t n = ::pwrite(m_fd, src, 128, byteOff);
  return n == 128;
}

void CpmDrive::RebuildAllocation()
{
  int blocks = m_disk.m_dsm + 1;
  if (blocks < 1 || m_disk.m_spt <= 0) {
    m_alloc.clear();
    return;
  }
  m_alloc.assign((size_t)(blocks + 7) / 8, 0);
  uint16_t al = (uint16_t)((m_disk.m_al0 << 8) | (m_disk.m_al1 & 0xff));
  for (int i = 0; i < 16; ++i) {
    if (al & (uint16_t)(0x8000 >> i))
      SetAllocBit(m_alloc, i);
  }
  bool words = m_disk.m_dsm >= 256;
  int slots = words ? 8 : 16;
  int entries = m_disk.m_drm + 1;
  for (int index = 0; index < entries; ++index) {
    size_t off = (size_t)index * 32;
    if (off + 32 > m_directory.size())
      break;
    const uint8_t * entry = m_directory.data() + off;
    if (entry[0] == 0xe5)
      continue;
    for (int s = 0; s < slots; ++s) {
      int block = Allocation(entry, s, words);
      if (block > 0 && block <= m_disk.m_dsm)
        SetAllocBit(m_alloc, block);
    }
  }
}

CpmDriveSet::CpmDriveSet()
{
  m_drives.reserve(kDrives);
  for (int i = 0; i < kDrives; ++i) {
    m_drives.emplace_back();
    m_drives.back().SetIndex(i);
  }
}

bool CpmDriveSet::Mount(const std::vector<std::string> & specs, std::string & error, std::ostream * debug)
{
  bool seen[kDrives] = {};
  for (const auto & spec : specs) {
    CpmDriveRequest request;
    if (!ParseCpmDriveRequest(spec, request, error))
      return false;
    if (seen[request.m_drive]) {
      error = std::string("drive ") + char('A' + request.m_drive) + " is listed more than once";
      return false;
    }
    seen[request.m_drive] = true;
    if (!request.m_image) {
      m_drives[(size_t)request.m_drive].MountHost(request.m_path);
      continue;
    }
    if (!m_drives[(size_t)request.m_drive].MountImage(request, error, debug))
      return false;
  }
  return true;
}

CpmDrive & CpmDriveSet::operator[](int drive)
{
  return m_drives.at((size_t)drive);
}

const CpmDrive & CpmDriveSet::operator[](int drive) const
{
  return m_drives.at((size_t)drive);
}
