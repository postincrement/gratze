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

using namespace std;

VirtualDriveFactory VirtualDrive::m_virtualDriveFactory;

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

  // get size of file
  off_t len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  // get the filename extension
  std::string extension;
  size_t pos = fn.rfind('.');
  if (pos != std::string::npos)
    extension = fn.substr(pos+1);

  // read some bytes
  uint8_t header[16];
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
  for (auto & r : keys) {
    drive = VirtualDrive::m_virtualDriveFactory.CreateInstance(r);

    if (m_verbose) {
      cout << "info: checking format '" << drive->GetName() << "'" << endl;
      drive->SetVerbose(true);
    }
  
    // see if file extensions matches
    if (extension == drive->GetExtension()) {
      if (drive->OpenFile(fd, len, header, sizeof(header))) {
        if (m_verbose)
          cout << "info: file '" << fn << "' set to format '" << drive->GetName() << "' using file extension" << endl;
        break;
      }
    }

    if (drive->OpenFile(fd, hdrLen, header, sizeof(header))) {
      if (m_verbose)
        cout << "info: file is format '" << drive->GetName() << "'" << endl;
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

void VirtualDrive::Init()
{
  AddFormat<VirtualDriveJV1>();
  AddFormat<VirtualDriveJV3>();
  AddFormat<VirtualDriveDMK>();
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

/////////////////////////////////////////////////////////////

VirtualDriveFile::VirtualDriveFile(const std::string & str)
  : m_fd(-1)
  , m_name(str)
  , m_extension(str)
  , m_sectorCount(0)
  , m_sectorSize(0)
  , m_trackCount(0)
  , m_density(0)
  , m_sideCount(0)
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

std::string VirtualDriveFile::GetName() const
{
  return m_name;
}

std::string VirtualDriveFile::GetExtension() const
{
  return m_extension;
}

int VirtualDriveFile::GetTracks() const
{
  return m_trackCount;
}

int VirtualDriveFile::GetSectors() const
{
  return m_sectorCount;
}

int VirtualDriveFile::GetSectorSize() const
{
  return m_sectorSize;
}

int VirtualDriveFile::GetSides() const
{
  return m_sideCount;
}

int VirtualDriveFile::GetDensity() const
{
  return m_density;
}

bool VirtualDriveFile::Mount(bool readonly)
{
  return true;
}

int VirtualDriveFile::ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len)
{
  int side = 0;
  auto r = m_sectorMap.find(HASH_STS(side, track, sector+1));
  if (r == m_sectorMap.end()) {
    m_error << "request for unknown sector " << dec << sector << " and track " << track;
    return -1;
  }

  info = r->second;

  if (m_verbose)
    cout << "drive: seek side " << side << ",track " << (int)track << ",sector " << sector << " = offset " << info.m_offset << " (" << HEXFORMAT0x4(info.m_offset) << ")" << endl;

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

int VirtualDriveFile::WriteSector(int track, int sector, uint8_t * data, int len)
{
  return false;
}

/////////////////////////////////////////////////////////////

