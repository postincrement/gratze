#include <iostream>

#include "common/misc.h"
#include "virtual_drive.h"

using namespace std;

#define   JV3_SECTOR_COUNT  (2901 / 3)

#define   SD_SECTOR_SIZE    256
#define   SD_SECTOR_COUNT   10

#define HASH_STS(side, track, sector)     (sector + (side << 8) + (track << 16))


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


static unsigned g_dmk_trackLengths[] = {
  0x0cc0,     // single density 5.25" : 3264 = 0x80 + 10 * 304 + 96
};

#define DMK_TRACKLENGTHS_COUNT (sizeof(g_dmk_trackLengths)/sizeof(g_dmk_trackLengths[0]))


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
      if (drive->OpenFile(fd, hdrLen, header, sizeof(header))) {
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

  cout << "drive: seek side " << side << ",track " << (int)track << ",sector " << sector << " = offset " << info.m_offset << " (" << HEXFORMAT0x4(info.m_offset) << ")" << endl;

  if (lseek(m_fd, info.m_offset, SEEK_SET) < 0) {
    cerr << "error: cannot seek for sector " << dec << sector << " and track " << track << endl;
    return -1;
  }

  if (len > info.m_size) {
    len = info.m_size;
  }

  int rlen = ::read(m_fd, data, len);

  if (rlen > 0)
    cout << DumpMemory((const uint8_t *)data, rlen);

  return rlen;  
}

int VirtualDriveFile::WriteSector(int track, int sector, uint8_t * data, int len)
{
  return false;
}

/////////////////////////////////////////////////////////////

VirtualDriveJV1::VirtualDriveJV1()
  : VirtualDriveFile("jv1")
{
}

bool VirtualDriveJV1::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{  
  // if the file size looks right, it's probably a JV1
  if ((len % (SD_SECTOR_SIZE * SD_SECTOR_COUNT)) != 0)
    return false;

  // read the sector map
  m_fd = fd;
  m_trackCount    = (len / (SD_SECTOR_COUNT * SD_SECTOR_SIZE));

  if (len != (SD_SECTOR_COUNT * SD_SECTOR_SIZE * m_trackCount)) {
    cerr << "drive: file length " << len << " is not compatible with tracks of " << SD_SECTOR_COUNT << " x " << SD_SECTOR_SIZE << " bytes" << endl;
    return false;
  }

  // directory tracks are 0xFA, all other tracks 0xFB

  off_t offs = 0;
  for (int track = 0; track < m_trackCount; ++track) {
    uint8_t dam = (track == 17) ? 0xfa : 0xfb;
    for (int sector = 1; sector <= SD_SECTOR_COUNT; ++sector) {
      cout << "dmk: dam=" << HEXFORMAT0x2(dam) << ","
              << "track=" << (int)track << "," 
              << "sector=" << (int) sector << "," 
              << "dam=" << HEXFORMAT0x2(dam) << endl; 

      m_sectorMap.emplace(HASH_STS(0, track, sector), SectorInfo(offs, SD_SECTOR_SIZE, dam, 0));
      offs += SD_SECTOR_SIZE;
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////

VirtualDriveJV3::VirtualDriveJV3()
  : VirtualDriveFile("jv3")
{
}

bool VirtualDriveJV3::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) 
{
  uint8_t jv3Header[JV3_SECTOR_COUNT*3];
  int ret = ::read(m_fd, jv3Header, sizeof(jv3Header));
  if (ret != sizeof(jv3Header)) {
    cerr << "drive: unable to read JV3 header - "<< ret << " != " << sizeof(jv3Header);
    return false;
  }

  m_trackCount = 0;
  uint8_t * ptr = jv3Header;
  off_t offs = JV3_SECTOR_COUNT*3 + 1;

  for (int i = 0; i < JV3_SECTOR_COUNT; ++i) {
    uint8_t track  = ptr[0];
    uint8_t sector = ptr[1];
    uint8_t flags  = ptr[2];

    int sectorSize;

    if ((track == 0xff) && (sector == 0xff)) {
      switch (flags & 0x3) {
        case 0:
          sectorSize = 512;
          break;
        case 1:
          sectorSize = 1024;
          break;
        case 2:
          sectorSize = 128;
          break;
        case 3:
          sectorSize = 256;
          break;
      }
    }
    else {
      int density = (flags & 0x80) ? 1 : 0;
      uint8_t dam = 0x00;
      int side    = (flags & 0x10) ? 1 : 0;
      switch (flags & 0x3) {
        case 0:
          sectorSize = 256;
          break;
        case 1:
          sectorSize = 128;
          break;
        case 2:
          sectorSize = 1024;
          break;
        case 3:
          sectorSize = 512;
          break;
      }
      switch ((flags & 0x60) + density) {
        case 0x00:  // single
        case 0x01:  // double
          dam = 0xfb;
          break;
        case 0x20:  // single
          dam = 0xfa;
          break;
        case 0x21:  // double
          dam = 0xf8;
          break;
        case 0x40:  // single
          dam = 0xf9;
          break;
        case 0x60:  // double
          dam = 0xf8;
          break;
      }
      if (dam == 0x00) {
        cerr << "drive: unknown DAM code " << HEXFORMAT0x2(dam) << endl;
      }
      else {
        cout << "dmk: dam=" << HEXFORMAT0x2(dam) << ","
               << "track=" << (int)track << "," 
               << "sector=" << (int) sector << "," 
               << "dam=" << HEXFORMAT0x2(dam) << endl; 
        m_sectorMap.emplace(HASH_STS(0, track, sector), SectorInfo(offs, sectorSize, dam, density));
      }
    }

    offs += sectorSize;
    ptr += 3;
  }
  return false;
}

/////////////////////////////////////////////////////////////

VirtualDriveDMK::VirtualDriveDMK()
  : VirtualDriveFile("dmk")
{
}

bool VirtualDriveDMK::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{
  if (headerSize < sizeof(DMKHeader)) {
    cout << "dmk: header size too small" << endl;
    return false;
  }

  DMKHeader * dmk = (DMKHeader *)header;

  bool formatGood =
         ((dmk->m_wrtProt     == 0x00)   || (dmk->m_wrtProt     == 0xff))
      && ((dmk->m_trackCount  == 35)     || (dmk->m_trackCount  == 40)     || (dmk->m_trackCount  == 80))
      && ((dmk->m_virtualFlag == 0x0000) || (dmk->m_virtualFlag == 0x12345678))
      ;

  if (!formatGood)
    return false;

  cout << "dmk: wrtProt=" << (int)dmk->m_wrtProt << ",tracks=" << (int)dmk->m_trackCount << endl;

  if (dmk->m_options & (1 << 4)) {
    cout << "dmk: disk is single density only" << endl;
  }  
  if (dmk->m_options & (1 << 6)) {
    cout << "dmk: disk is single density with double option" << endl;
  }  
  if (dmk->m_options & (1 << 7)) {
    cout << "dmk: disk is double density with single option" << endl;
  }  

  int i;
  for (i = 0; i < DMK_TRACKLENGTHS_COUNT; ++i) {
    if (dmk->m_trackLen == g_dmk_trackLengths[i])
    break;
  }
  if (i == DMK_TRACKLENGTHS_COUNT) {
    cerr << "error: format looks like DMK, but track length " << dmk->m_trackLen << " is not recognised" << endl;
    return false;
  }

  m_fd = fd;
  m_trackCount = dmk->m_trackCount;
  off_t offs = 0x10;

  for (int trackNum = 0; trackNum < m_trackCount; ++trackNum) {
    uint16_t sectorOffsets[64];
    if (::lseek(m_fd, offs, SEEK_SET) < 0) {
      cerr << "dmk: cannot seek to header for track " << trackNum << " at offset " << offs << endl;
    }
    else if (::read(m_fd, sectorOffsets, sizeof(sectorOffsets)) != sizeof(sectorOffsets)) {
      cerr << "dmk: cannot read header for track " << trackNum << " at offset " << offs << endl;
    }
    else {
      int sectorNum = 1;
      for (int i = 0; i < 64; ++i) {
        if (sectorOffsets[i] == 0)
          break;
        DMKSector sector; 
        int idamOffs       = (uint8_t *)&sector.m_idam - (uint8_t *)&sector;
        int headerLen      = (uint8_t *)&sector.m_data - (uint8_t *)&sector;
        int idamToDataOffs = (uint8_t *)&sector.m_data - (uint8_t *)&sector.m_idam;
        int sectorSize = 256;
        int density = 0;

        off_t sectorOffset = sectorOffsets[i] & 0x3fff; 

        if (
            (::lseek(m_fd, offs + sectorOffset-idamOffs, SEEK_SET) < 0) ||
            (::read(m_fd, &sector, headerLen) != headerLen)
          ) {
          cerr << "dmk: cannot read info for sector " << sectorNum << ", track " << trackNum << endl;
        }
        else {
          cout << "dmk: idam=" << HEXFORMAT0x2(sector.m_idam) << ","
               << "track=" << (int) sector.m_track << "," 
               << "sector=" << (int) sector.m_sector << "," 
               << "dam=" << HEXFORMAT0x2(sector.m_dam) << ","
               << "size=" << sizeof(sector) << endl; 
          cout << DumpMemory((const uint8_t *)&sector, headerLen);
//          if (sector.m_track != trackNum) {
//            cerr << "dmk: track " << trackNum << ", sector " << sectorNum << " has mismatched track number " << (int)sector.m_track << endl;
//          }
//          if (sector.m_sector != sectorNum) {
//            cerr << "dmk: track " << trackNum << ", sector " << sectorNum << " has mismatched sector number " << (int)sector.m_sector << endl;
//          }

          m_trackCount = std::max(m_trackCount, (int)trackNum);
          m_sectorMap.emplace(HASH_STS(0, trackNum, sector.m_sector+1), 
              SectorInfo(offs + sectorOffset + idamToDataOffs, sectorSize, sector.m_dam, density));
        }
        ++sectorNum;
      }
    }

    offs += dmk->m_trackLen;
  }

  return true;
}


