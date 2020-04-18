#include <iostream>

using namespace std;

#include "common/misc.h"
#include "disk/jv1.h"

#define   SD_SECTOR_COUNT   10

#define   SD_SECTOR_SIZE    256

VirtualDriveJV1::VirtualDriveJV1()
  : VirtualDriveFile("jv1")
{
}

bool VirtualDriveJV1::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{  
  // if the file size looks right, it's probably a JV1
  if ((len % (SD_SECTOR_SIZE * SD_SECTOR_COUNT)) != 0)
    return false;

  int trackCount  = (len / (SD_SECTOR_COUNT * SD_SECTOR_SIZE));
  
  if (len != (SD_SECTOR_COUNT * SD_SECTOR_SIZE * trackCount)) {
    m_error << "jv1 - file length " << len << " is not compatible with tracks of " << SD_SECTOR_COUNT << " x " << SD_SECTOR_SIZE << " bytes";
    return false;
  }

  m_density       = 0;
  m_sideCount     = 1;

  // directory tracks are 0xFA, all other tracks 0xFB
  off_t offs = 0;
  for (int track = 0; track < trackCount; ++track) {
    TrackInfo trackInfo;
    uint8_t dam = (track == 17) ? 0xf8 : 0xfb;
    for (int sector = 0; sector < SD_SECTOR_COUNT; ++sector) {
      if (m_verbose)
        cout << "jv1: dam=" << HEXFORMAT0x2(dam) << ","
                << "track=" << (int)track << "," 
                << "sector=" << (int) sector << "," 
                << "dam=" << HEXFORMAT0x2(dam) << endl; 
      SectorInfo sectorInfo(sector, offs, SD_SECTOR_SIZE, dam, 0);
      trackInfo.AddSector(sectorInfo);
      offs += SD_SECTOR_SIZE;
    }
    AddTrack(track, trackInfo);
  }

  m_fd = fd;
  return true;
}
