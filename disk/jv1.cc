#include <iostream>

using namespace std;

#include "common/misc.h"
#include "disk/jv1.h"

#define   SD_SECTOR_COUNT   10

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
    uint8_t dam = (track == 17) ? 0xf8 : 0xfb;
    for (int sector = 1; sector <= SD_SECTOR_COUNT; ++sector) {
      if (m_debug)
        cout << "jv1: dam=" << HEXFORMAT0x2(dam) << ","
                << "track=" << (int)track << "," 
                << "sector=" << (int) sector << "," 
                << "dam=" << HEXFORMAT0x2(dam) << endl; 
      m_sectorMap.emplace(HASH_STS(0, track, sector), SectorInfo(offs, SD_SECTOR_SIZE, dam, 0));
      offs += SD_SECTOR_SIZE;
    }
  }

  return true;
}
