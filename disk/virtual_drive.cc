#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/misc.h"
#include "virtual_drive.h"

#include "jv1.h"
#include "jv3.h"
#include "dmk.h"
#include "dsk.h"

using namespace std;

VirtualDriveFactory VirtualDrive::m_virtualDriveFactory;

#define FILE_IDENTIFIER_HEADER_SIZE      40         // must be bigger than the DSK format header (34 bytes)

/////////////////////////////////////////////////////////////

VirtualFileIdentifier::VirtualFileIdentifier()
: m_verbose(false)
{}

void VirtualFileIdentifier::SetVerbose(bool verbose)
{
  m_verbose = verbose;
}

std::string VirtualFileIdentifier::GetError() const
{
  return m_error.str();
}

VirtualDrive * VirtualFileIdentifier::Open(const std::string & fn, bool readOnly)
{
  // see if file exists
  if (::access(fn.c_str(), 0) != 0) {
    m_error << "cannot find file '" << fn << "'";
    return nullptr;
  }

  // see if underlying file is read-only
  if (!readOnly) {
    readOnly = ::access(fn.c_str(), W_OK) != 0;
  }

  int fd = -1;

  //if (m_readOnly)
    fd = ::open(fn.c_str(), O_RDONLY | O_BINARY);
  //else
  //  fd = ::open(name.c_str(), O_RDWR);

  if (fd < 0) {
    m_error << "cannot open file '" << fn << "' - " << strerror(errno);
    return nullptr;
  }

  // get length
  struct stat stbuf;
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    cerr << "error: '" << fn << "' inaccessible or not a regular file" << endl;
    return nullptr;
  }  
  off_t len = stbuf.st_size;
  

  // get the filename extension
  std::string extension;
  size_t pos = fn.rfind('.');
  if (pos != std::string::npos)
    extension = fn.substr(pos+1);

  // read some bytes
  uint8_t header[FILE_IDENTIFIER_HEADER_SIZE];
  int hdrLen = read(fd, header, sizeof(header));
  if (hdrLen != sizeof(header)) {
    m_error << "file is too short (" << hdrLen << ") to be a disk image";
    close(fd);    
    return nullptr;
  }

  // seek back to start
  lseek(fd, 0, SEEK_SET);

  VirtualDriveFactory::KeyList keys;
  VirtualDrive::m_virtualDriveFactory.GetKeys(keys);

  VirtualDrive * drive = nullptr;

  // give priority to formats that match the extension
  for (auto & r : keys) {
    drive = VirtualDrive::m_virtualDriveFactory.CreateInstance(r);
    if (extension != drive->GetExtension()) {
      delete drive;
    }
    else if (drive->OpenFile(fd, len, header, sizeof(header))) {
      if (m_verbose)
        cout << "info: file is format '" << drive->GetFormat() << "'" << endl;
      return drive;
    }
    else {
      if (m_verbose)
        cout << "info: file has '" << drive->GetFormat() << "' extension but not '" << drive->GetFormat() << "' format" << endl;
      delete drive;
      break;
    }
  }

  // try all formats
  for (auto & r : keys) {
    drive = VirtualDrive::m_virtualDriveFactory.CreateInstance(r);

    if (m_verbose) {
      cout << "info: checking format '" << drive->GetFormat() << "'" << endl;
      drive->SetVerbose(true);
    }
  
    lseek(fd, 0, SEEK_SET);
    if (drive->OpenFile(fd, len, header, sizeof(header))) {
      if (m_verbose)
        cout << "info: file is format '" << drive->GetFormat() << "'" << endl;
      break;
    }

    if (m_verbose && !drive->GetError().empty()) {
      cout << "error: " << drive->GetError() << endl;
    }

    delete drive;
    drive = nullptr;
  }

  if (!drive) {
    m_error << "error: cannot identify format of device '" << fn << "'";
  }

  return drive;
}

/////////////////////////////////////////////////////////////

VirtualDrive::SectorInfo::SectorInfo(int id, off_t offset, int size, uint8_t dam, uint8_t density)
  : m_id(id)
  , m_offset(offset)
  , m_size(size)
  , m_dam(dam)
  , m_density(density)
{}

/////////////////////////////////////////////////////////////

VirtualDrive::TrackInfo::TrackInfo()
{}

int VirtualDrive::TrackInfo::GetSectorCount() const
{
  return m_sectors.size();
}

void VirtualDrive::TrackInfo::AddSector(const SectorInfo & sector)
{
  m_sectors.push_back(sector);
}

VirtualDrive::SectorInfo * VirtualDrive::TrackInfo::GetSector(int sector)
{
  for (auto & r : m_sectors)
    if (sector == r.m_id)
      return &r;

  return nullptr;    
}

const VirtualDrive::SectorInfo * VirtualDrive::TrackInfo::GetSector(int sector) const
{
  for (auto & r : m_sectors)
    if (sector == r.m_id)
      return &r;

  return nullptr;    
}

VirtualDrive::SectorList & VirtualDrive::TrackInfo::GetSectors()
{
  return m_sectors;
}

const VirtualDrive::SectorList & VirtualDrive::TrackInfo::GetSectors() const
{
  return m_sectors;
}

/////////////////////////////////////////////////////////////

VirtualDrive::TrackList & VirtualDrive::SideInfo::GetTracks()
{
  return m_tracks;
}

const VirtualDrive::TrackList & VirtualDrive::SideInfo::GetTracks() const
{
  return m_tracks;
}

int VirtualDrive::SideInfo::GetTrackCount() const
{
  return m_tracks.size();
}

/////////////////////////////////////////////////////////////


void VirtualDrive::Init()
{
  AddFormat<VirtualDriveJV1>();
  AddFormat<VirtualDriveJV3>();
  AddFormat<VirtualDriveDMK>();
  AddFormat<VirtualDriveDSK>();
}

VirtualDrive::VirtualDrive()
{
  m_verbose = false;
}

VirtualDrive::~VirtualDrive()
{}

bool VirtualDrive::IsReadOnly() const
{
  return m_readOnly;
}

std::string VirtualDrive::GetError() const
{
  return m_error.str();
}

void VirtualDrive::SetVerbose(bool v)
{
  m_verbose = v;
}

int VirtualDrive::GetSideCount() const
{
  return m_disk.size();
}

VirtualDrive::SideList & VirtualDrive::GetSides()
{
  return m_disk;
}

const VirtualDrive::SideList & VirtualDrive::GetSides() const
{
  return m_disk;
}

int VirtualDrive::GetDensity() const
{
  return m_density;
}

void VirtualDrive::AddTrack(int sideNum, int trackNum, const TrackInfo & track)
{
  m_disk[sideNum].GetTracks()[trackNum] = track;
}

VirtualDrive::TrackInfo * VirtualDrive::GetTrack(int sideNum, int trackNum)
{
  auto r = m_disk.find(sideNum);
  if (r == m_disk.end())
    return nullptr;
  auto & tracks = r->second.GetTracks();  
  auto s = tracks.find(trackNum);
  if (s == tracks.end())
    return nullptr;
  return &s->second;     
}

const VirtualDrive::TrackInfo * VirtualDrive::GetTrack(int sideNum, int trackNum) const
{
  const auto r = m_disk.find(sideNum);
  if (r == m_disk.end())
    return nullptr;
  const auto & tracks = r->second.GetTracks();  
  const auto s = tracks.find(trackNum);
  if (s == tracks.end())
    return nullptr;
  return &s->second;     
}

/////////////////////////////////////////////////////////////

VirtualDriveFile::VirtualDriveFile(const std::string & str)
  : m_fd(-1)
  , m_name(str)
  , m_extension(str)
{
}

VirtualDriveFile::VirtualDriveFile(const std::string & name, const std::string & ext)
  : VirtualDriveFile(name)
{
  m_extension = ext;
}

VirtualDriveFile::~VirtualDriveFile()
{
  if (m_fd >= 0)
    ::close(m_fd);
}

std::string VirtualDriveFile::GetFormat() const
{
  return m_name;
}

std::string VirtualDriveFile::GetExtension() const
{
  return m_extension;
}

bool VirtualDriveFile::Mount(bool readonly)
{
  return true;
}

VirtualDrive::SectorInfo * VirtualDriveFile::GetInfo(int sideNum, int trackNum, int sectorNum)
{
  auto * trackInfo = GetTrack(sideNum, trackNum);
  if (trackInfo != nullptr) {
    auto * sectorInfo = trackInfo->GetSector(sectorNum);
    if (sectorInfo != nullptr) {
      return sectorInfo;
    }
  }

  m_error << "request for unknown side " << dec << sideNum 
          << ", track " << dec << trackNum 
          << ", sector " << dec << sectorNum 
          << endl;

  return nullptr;
}

int VirtualDriveFile::ReadSector(int side, int track, int sector, SectorInfo & info, uint8_t * data, int len)
{
  auto sectorInfo = GetInfo(side, track, sector);
  info = *sectorInfo;

  if (m_verbose)
    cout << "drive: read side " << side << ",track " << (int)track << ",sector " << sector << " = offset " << info.m_offset << " (" << HEXFORMAT0x4(info.m_offset) << ")" << endl;

  if (lseek(m_fd, info.m_offset, SEEK_SET) < 0) {
    m_error << "cannot seek for sector " << dec << sector << " and track " << track;
    return -1;
  }

  if (len > info.m_size) {
    len = info.m_size;
  }

  int rlen = ::read(m_fd, data, len);
  //if (rlen > 0)
  //  cout << DumpMemory((const uint8_t *)data, rlen);
  return rlen;  
}

int VirtualDriveFile::WriteSector(int side, int track, int sector, uint8_t * data, int len)
{
  return false;
}

/////////////////////////////////////////////////////////////

