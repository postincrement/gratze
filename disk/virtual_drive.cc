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

//
// DMK sector format - 5.25
//

#pragma pack(1)

#pragma pack()

struct DiskFormat {
  const char * name;

  char m_size;             // '3', '5', or '8'
  int  m_sides;            // 1 or 2
  bool m_doubleDensity;    // true is double density

  uint8_t  m_tracks;          // # of tracks
  uint16_t m_sectorSize;      // sector size
  uint8_t  m_sectorsPerTrack; // sectors per track
};


static DiskFormat g_formats[] = { 

//  name         size   sides  density  tracks  sec size  sec/track  
{  "5_SS_SD_35",  '5',    1,    false,    35,      256,       10 },
{  "5_SS_SD_40",  '5',    1,    false,    40,      256,       10 },
{  "5_SS_SD_80",  '5',    1,    false,    80,      256,       10 },

{  "5_SS_SD_35",  '5',    2,    false,    35,      256,       10 },
{  "5_SS_SD_40",  '5',    2,    false,    40,      256,       10 },
{  "5_SS_SD_80",  '5',    2,    false,    80,      256,       10 }

};


VirtualDriveFactory VirtualDrive::g_virtualDriveFactory;

/////////////////////////////////////////////////////////////

void VirtualDrive::Init()
{
  AddFormat<VirtualDriveJV1>();
  AddFormat<VirtualDriveJV3>();
  AddFormat<VirtualDriveDMK>();
}

VirtualDrive::VirtualDrive()
{}

VirtualDrive::~VirtualDrive()
{}

bool VirtualDrive::IsReadOnly() const
{
  return m_readOnly;
}

VirtualDrive * VirtualDrive::Open(const std::string & fn, bool readOnly)
{
  // see if file exists
  if (::access(fn.c_str(), 0) != 0) {
    cerr << "error: cannot find file '" << fn << "'" << endl;
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
    cerr << "error: cannot open file '" << fn << "' - " << strerror(errno) << endl;
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
    cerr << "error: file is too short (" << hdrLen << ") to be a disk image" << endl;
    close(fd);    
    return nullptr;
  }

  // seek back to start
  lseek(fd, 0, SEEK_SET);

  VirtualDriveFactory::KeyList keys;
  g_virtualDriveFactory.GetKeys(keys);

  VirtualDrive * drive = nullptr;
  for (auto & r : keys) {
    drive = g_virtualDriveFactory.CreateInstance(r);

    cout << "info: checking format '" << drive->GetName() << "'" << endl;
  
    // see if file extensions matches
    if (extension == drive->GetExtension()) {
      if (drive->OpenFile(fd, len, header, sizeof(header))) {
        cerr << "info: file '" << fn << "' set to format '" << drive->GetName() << "' using file extension" << endl;
        break;
      }
    }

    if (drive->OpenFile(fd, hdrLen, header, sizeof(header))) {
      cerr << "info: file is format '" << drive->GetName() << "'" << endl;
      break;
    }

    delete drive;
    drive = nullptr;
  }

  if (!drive) {
    cerr << "error: cannot identify format of device '" << fn << "'" << endl;
  }

  return drive;
}

/////////////////////////////////////////////////////////////

VirtualDriveFile::VirtualDriveFile(const std::string & str)
  : m_fd(-1)
  , m_name(str)
  , m_extension(str)
{}

VirtualDriveFile::VirtualDriveFile(const std::string & name, const std::string & ext)
  : m_fd(-1)
  , m_name(name)
  , m_extension(ext)
{
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

bool VirtualDriveFile::Mount(bool readonly)
{
  return true;
}

int VirtualDriveFile::ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len)
{
  int side = 0;
  auto r = m_sectorMap.find(HASH_STS(side, track, sector+1));
  if (r == m_sectorMap.end()) {
    cerr << "error: request for unknown sector " << dec << sector << " and track " << track << endl;
    return -1;
  }

  info = r->second;

  if (m_debug)
    cout << "drive: seek side " << side << ",track " << (int)track << ",sector " << sector << " = offset " << info.m_offset << " (" << HEXFORMAT0x4(info.m_offset) << ")" << endl;

  if (lseek(m_fd, info.m_offset, SEEK_SET) < 0) {
    cerr << "error: cannot seek for sector " << dec << sector << " and track " << track << endl;
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

