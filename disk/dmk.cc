
#include <iostream>
#include <unistd.h>

using namespace std;

#include "common/misc.h"
#include "disk/dmk.h"

static unsigned g_dmk_trackLengths[] = {
  0x0cc0,     // single density 5.25" : 3264 = 0x80 + 10 * 304 + 96
};

#define DMK_TRACKLENGTHS_COUNT (sizeof(g_dmk_trackLengths)/sizeof(g_dmk_trackLengths[0]))


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
          if (m_debug)
            cout << "dmk: idam=" << HEXFORMAT0x2(sector.m_idam) << ","
                << "track=" << (int) sector.m_track << "," 
                << "sector=" << (int) sector.m_sector << "," 
                << "dam=" << HEXFORMAT0x2(sector.m_dam) << ","
                << "size=" << sizeof(sector) << endl; 
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


