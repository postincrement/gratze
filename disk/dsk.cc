#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;

#include "common/misc.h"
#include "disk/dsk.h"

#define DISK_FORMAT_ID      "MV - CPCEMU Disk-File\r\nDisk-Info\r\n"
#define DISK_FORMAT_ID_LEN  (sizeof(DISK_FORMAT_ID)-1)

#define TRACK_HEADER_ID      "Track-Info\r\n"
#define TRACK_HEADER_ID_LEN  (sizeof(TRACK_HEADER_ID)-1)


#pragma pack(1)
struct DSKHeader
{
  char     m_id[34];       // ID
  char     m_creator[14];  // creator
  uint8_t  m_tracks;       // number of tracks
  uint8_t  m_sides;        // number of sides
  uint16_t m_trackSize;    // length of a track
  uint8_t  m_unused[204];  // pad out to 256
};

struct TrackHeader
{
  char     m_id[12];       // ID
  uint8_t  m_unused[4];
  uint8_t  m_track;
  uint8_t  m_side;
  uint8_t  m_unused2[2];
  uint8_t  m_sectorSize;
  uint8_t  m_sectorCount; 
  uint8_t  m_gap3Count;
  uint8_t  m_filler;
};

VirtualDriveDSK::VirtualDriveDSK()
  : VirtualDriveFile("dsk")
{
}

bool VirtualDriveDSK::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{  
  // make sure header is long enough
  if (headerSize < DISK_FORMAT_ID_LEN) {
    m_error << "dsk: preload header to short";
    return false;
  }

  // compare header
  if (memcmp((const char *)header, DISK_FORMAT_ID, DISK_FORMAT_ID_LEN) != 0) {
    m_error << "dsk: file header does not match";
    cout << DumpMemory(header, headerSize);
    cout << DumpMemory((const uint8_t *)DISK_FORMAT_ID, DISK_FORMAT_ID_LEN);
    return false;
  }

  // read entire header
  DSKHeader dskHeader;
  if (::read(fd, &dskHeader, sizeof(dskHeader)) != sizeof(dskHeader)) {
    m_error << "dsk: cannot read file header";
    return false;
  }

  m_trackCount = dskHeader.m_tracks;
  off_t offs = 0x100;

  for (int trackNum = 0; trackNum < m_trackCount; ++trackNum) {
    for (int side = 0; side < dskHeader.m_sides; ++side) {
      TrackHeader trackHeader;

      if (::lseek(fd, offs, SEEK_SET) < 0) {
        m_error << "dsk: cannot seek to header for track " << trackNum << " at offset " << offs;
        return false;
      }
      else if (::read(fd, &trackHeader, sizeof(trackHeader)) != sizeof(trackHeader)) {
        m_error << "dsk - cannot read header for track " << trackNum << " at offset " << offs;
        return false;
      }
      else if (memcmp(trackHeader.m_id, TRACK_HEADER_ID, TRACK_HEADER_ID_LEN) != 0) {
        m_error << "dsk: header for track " << trackNum << " at offset " << HEXFORMAT0x4(offs) << " has bad ID";
        return false;
      }
      else {
        m_trackCount  = std::max((int)trackHeader.m_track,       m_trackCount);
        m_sectorCount = std::max((int)trackHeader.m_sectorCount, m_sectorCount );
        m_sectorSize  = std::max((int)trackHeader.m_sectorSize*256,  m_sectorSize);
  //      m_density     = std::max((int)density,    m_density);
        m_sideCount   = std::max((int)(side+1),   m_sideCount);
      }

      offs += 0x100 + trackHeader.m_sectorCount * (trackHeader.m_sectorSize * 256);
    }
  }

  m_fd = fd;
  return true;
}
