
#include <iostream>

#include <../common/misc.h>

#include "cpmfs.h"

using namespace std;

/*
  A file is defined by between 1 and 31 directory entries.
  Each directory entry controls an "extent" which can reference up
  to sixteen blocks.
  Each block contains between 1 and 16 records each of 128 bytes.

  
*/


static CPMFileSystem::DPB microbeeSD = {
  "mbeesd",
  18,     // m_spt = # of 128 byte record per track 
  3,      // m_bsh = block shift: 0x03 = 1k, 0x04 = 2k, 0x05 = 4k
  7,      // m_blm = block mask: 0x07 = 1k, 0x0f = 2k, 0x1f = 4k
  0,      // extent mask ??
  172,    // m_dsm = disk size - 1 
  63,     // m_drm = # of directory entries - 1
  0xc0,   // al0
  0x00,   // al1
  16,     // m_cks = # of directory entries / 4, rounded up
  3,      // m_off = # of reserved tracks

  false,  // is not cylinder
  0,      // sector size : 0 = 128 byte sectors

  // sector skew
  { 
    1, 4, 7, 10, 13, 16,
    2, 5, 8, 11, 14, 17, 
    3, 6, 9, 12, 15, 18 
  }
};

static CPMFileSystem::DPB microbeeDD = {
  "mbeedd",
  40,     // m_spt = # of 128 byte record per track 
  4,      // m_bsh = block shift: 0x03 = 1k, 0x04 = 2k, 0x05 = 4k
  0xf,    // m_blm = block mask: 0x07 = 1k, 0x0f = 2k, 0x1f = 4k
  1,      // extent mask ??
  194,    // m_dsm = disk size - 1 
  127,    // m_drm = # of directory entries - 1
  0xc0,   // al0
  0x00,   // al1
  32,     // m_cks = # of directory entries / 4, rounded up
  2,      // m_off = # of reserved tracks

  true,   // is cylinder
  2,      // sector size : 3 = 1k byte sectors

  // sector skew
  { 
    2, 5, 8,
    1, 4, 7, 10,
    3, 6, 9 
  }
};

std::vector<CPMFileSystem::DPB *> g_formats = {
  &microbeeSD,
  &microbeeDD
};


///////////////////////////////////////////////////////////////////////////////

bool CPMFileSystem::DirectoryEntry::IsDeleted() const
{ 
  return m_uu == 0xe5; 
}

std::string CPMFileSystem::DirectoryEntry::GetFilename() const 
{ 
  return GetBasename() + "." + GetExtension(); 
}

std::string CPMFileSystem::DirectoryEntry::GetBasename() const
{ 
  std::string fn;
  for (int i = 0; i < sizeof(m_fn); ++i)
    fn.push_back(m_fn[i] & 0x7f);
  return fn;  
}

std::string CPMFileSystem::DirectoryEntry::GetExtension() const
{ 
  std::string fn;
  for (int i = 0; i < sizeof(m_ext); ++i)
    fn.push_back(m_ext[i] & 0x7f);
  return fn;  
}

unsigned CPMFileSystem::DirectoryEntry::GetExtentCounter(const DPB & dpb) const
{ 
  return (m_ex & 0x1f) / (dpb.m_exm + 1); 
}

unsigned CPMFileSystem::DirectoryEntry::GetEntryNumber(const DPB & dpb) const
{ 
  return (GetExtentCounter(dpb) + (32 * m_s2)) / (dpb.m_exm + 1); 
}

unsigned CPMFileSystem::DirectoryEntry::GetRecordCount(const DPB & dpb) const
{ 
  return ((m_ex & dpb.m_exm) * 128) + m_rc; 
}

unsigned CPMFileSystem::DirectoryEntry::GetBlockCount(const DPB & dpb)
{
  return dpb.Use16BitBlockNumber() ? 8 : 16; 
}

unsigned CPMFileSystem::DirectoryEntry::GetBlock(const DPB & dpb, int i)
{
  if (!dpb.Use16BitBlockNumber()) {
    return m_al[i & 0xf];
  }
  i = (i & 7) << 1;
  return (m_al[i+1] << 8) + m_al[i];
}

unsigned CPMFileSystem::DirectoryEntry::GetUser() const
{
  return m_uu & 0x1f;
}

bool CPMFileSystem::DirectoryEntry::IsReadOnly() const
{
  return (m_ext[0] & 0x80) != 0;
}

bool CPMFileSystem::DirectoryEntry::IsHidden() const
{
  return (m_ext[1] & 0x80) != 0;
}


///////////////////////////////////////////////////////////////////////////////

CPMFileSystem::CPMFileSystem(std::shared_ptr<VirtualDrive> drive)
  : m_drive(drive)
  , m_open(false)
{
}

struct null_deleter
{ void operator()(void const *) const{} };


bool CPMFileSystem::Open()
{
  m_open = false;

  // find a format match
  for (auto & dpb : g_formats) {

    // calculate the track size in bytes
    unsigned format_blockSizeInBytes = dpb->GetSectorSize();
    unsigned format_trackSizeInBytes = 128 * dpb->m_spt;
    unsigned drive_trackSizeInBytes  = (m_drive->GetSectorCount() * m_drive->GetSectorSize());

    if (
        (format_blockSizeInBytes == m_drive->GetSectorSize()) &&
        (format_trackSizeInBytes == drive_trackSizeInBytes)
       ) {
      m_dpb = std::shared_ptr<DPB>(dpb, null_deleter());
      m_open = true;
      break;
    }
  }
  if (!m_open) {
    cerr << "error: could not find matching format" << endl;
    return false;
  }

  cout << "info: using format '" << m_dpb->m_name << "'" << endl;

  // save some parameters
  m_recordsPerSector = 1 << m_dpb->m_sec;
  m_sectorSize       = m_dpb->GetSectorSize();
  m_tracksPerSide    = m_drive->GetTrackCount();
  m_sectorsPerTrack  = m_dpb->m_spt / (1 << m_dpb->m_sec);
  m_blockSize        = (1 << m_dpb->m_bsh) * 128;
  m_extentSize       = (16 * m_blockSize) / (m_dpb->m_exm + 1);

  m_sectorBuffer.resize(m_sectorSize);
  m_blockBuffer.resize(m_blockSize);

  unsigned directorySize = ((m_dpb->m_drm + 1) * 32);

#if 0
  cout << "  block size     : " << m_blockSize << " bytes" << endl;
  cout << "  extent size    : " << FIXEDFORMAT3(m_extentSize / 1024.0) << " kb (" << (m_extentSize / 128) << " records)"  << endl;
  cout << "  directory size : " << FIXEDFORMAT3(directorySize / 1024.0) << " kb (" << (directorySize / 128) << " records, " << (directorySize / m_blockSize) << " blocks)"  << endl;
  cout << "  max file size  : " << FIXEDFORMAT3(32 * m_extentSize / 1024.0) << " kb (" << (32 * m_extentSize / 128) << " records)" << endl;
  cout << "  max disk size  : " << FIXEDFORMAT3((m_dpb->m_dsm + 1) * m_blockSize / 1024.0) << " kb" << endl;
#endif

  if (!ReadDirectory()) {
    cerr << "could not read directory" << endl;
    return false;
  }
#if 0
  int col = 0;
  for (auto r : m_allocatedBlocks) {
    cout << (r ? "X" : ".");
    if (++col == 16) {
      col = 0;
      cout << endl;
    }
  }
#endif

  return true;
}

void CPMFileSystem::AllocateFromMask(uint8_t bits, unsigned offs)
{
  uint8_t mask = 0x80;
  while (mask != 0) {
    if (bits & mask)
      m_allocatedBlocks[offs] = true;
    ++offs;
    mask = mask >> 1;
  }
}


bool CPMFileSystem::ReadDirectory()
{  
  int track = 0;
  int sector = 1;
  int dirIndex = 0;

  // initialize allocation bitmask
  m_allocatedBlocks.resize(m_dpb->m_dsm + 1);
  std::fill(m_allocatedBlocks.begin(), m_allocatedBlocks.end(), false);

  AllocateFromMask(m_dpb->m_al0, 0);
  AllocateFromMask(m_dpb->m_al1, 8);

  for (;;) {
    if (!ReadSector(track, sector, &m_sectorBuffer[0])) {
      cout << "error: could not read sector" << endl;
      return false;
    }

    uint8_t * record = &m_sectorBuffer[0];
    for (int i = 0; i < m_sectorSize; i += 128) {
      for (int x = 0; x < 128; x += 32) {
        DirectoryEntry * entry = (DirectoryEntry *)record;
        //cout << DumpMemory((const uint8_t *)entry, 32);
        if (!entry->IsDeleted()) {
          std::string filename = entry->GetFilename();
          if (m_fileList.count(filename) == 0) {
            FileInfo info;
            info.m_name     = filename;
            info.m_user     = entry->GetUser();
            info.m_readonly = entry->IsReadOnly();
            info.m_hidden   = entry->IsHidden();

            info.m_size = entry->GetRecordCount(*m_dpb) * 128;
            info.m_extents.emplace(entry->GetExtentCounter(*m_dpb), *entry);
            m_fileList.emplace(filename, info);
          }
          else {
            FileInfo & info = m_fileList.at(filename);
            info.m_size += entry->GetRecordCount(*m_dpb) * 128;
            info.m_extents.emplace(entry->GetExtentCounter(*m_dpb), *entry);
          }
          for (int b = 0; b < entry->GetBlockCount(*m_dpb); ++b) {
            unsigned block = entry->GetBlock(*m_dpb, b);
            if (block != 0)
              m_allocatedBlocks[block] = true;
          }     
        }
        dirIndex++;
        if (dirIndex > m_dpb->m_drm) {
          return true;
        }
        record += 32;
      }
    }

    if (++sector > m_sectorsPerTrack) {
      sector = 1;
      ++track;
    }
  }

  return true;
}

bool CPMFileSystem::ReadSector(int track, int sector, uint8_t * sectorBuffer)
{
  int otrack = track;
  int osector = sector;

  // adjust track for reserved tracks
  track = track + m_dpb->m_off;

  // calculate size and track
  int side = 0;
  if (m_dpb->m_isCylinder) {
    side = track & 1;
    track = track >> 1;
  }
  else {
    side = track / m_tracksPerSide;
    track = track % m_tracksPerSide; 
  }

  // adjust sector for skew
  sector = m_dpb->m_skew[(sector - 1) % m_dpb->m_skew.size()];

  //cout << "track/sector " << otrack << "/" << osector << " -> size/track/sector " << side << "/" << track << "/" << sector << endl;

  // read the sector
  VirtualDrive::SectorInfo sectorInfo;
  return m_drive->ReadSector(side, track, sector, sectorInfo, sectorBuffer, m_sectorSize);
}

bool CPMFileSystem::ReadBlock(unsigned blockNum, uint8_t * blockBuffer)
{
  // convert to record
  unsigned recordInBytes = blockNum * m_blockSize;

  // convert to sector
  unsigned sectorNum = recordInBytes / m_sectorSize;

  // get track and sector
  unsigned sector = (sectorNum % m_sectorsPerTrack) + 1;
  unsigned track  = sectorNum / m_sectorsPerTrack;

  for (unsigned offs = 0; offs < m_blockSize; offs += m_sectorSize) {
    cout << "reading block " << HEXFORMAT0x4(blockNum) << "/" << offs/m_sectorSize << " = " << track << " " << sector << endl;
    if (!ReadSector(track, sector, blockBuffer + offs))
      return false;
    if (++sector > m_sectorsPerTrack) {
      sector = 1;
      ++track;
    }
  }

  return true;
}

bool CPMFileSystem::Read(const std::string & filename, std::vector<uint8_t> & data)
{
  // find filename in list
  if (m_fileList.count(filename) == 0) {
    return false;
  }

  FileInfo & info = m_fileList.at(filename);

  for (auto & r : info.m_extents) {
    auto & extent = r.second;
    unsigned extentSize = extent.GetRecordCount(*m_dpb) * 128;
    unsigned startOffs = data.size();
    for (unsigned blockIndex = 0; blockIndex < extent.GetBlockCount(*m_dpb); ++blockIndex) {
      unsigned blockNumber = extent.GetBlock(*m_dpb, blockIndex);
      if (blockNumber != 0) {
        unsigned offs = data.size();
        data.resize(offs + m_blockSize);
        if (!ReadBlock(blockNumber, &data[offs])) {
          data.resize(offs);
          return false;
        }
      }
    }
    data.resize(startOffs + extentSize); 
  }

  return true;
}

