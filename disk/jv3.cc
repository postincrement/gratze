
#include <iostream>
#include <unistd.h>

using namespace std;

#include "common/misc.h"
#include "disk/jv3.h"

#define   JV3_SECTOR_COUNT  (2901 / 3)


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
        if (m_debug)
          cout << "jv3: dam=" << HEXFORMAT0x2(dam) << ","
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
