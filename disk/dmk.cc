
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
    m_error << "dmk - header size too small" << endl;
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

  if (m_verbose)
    cout << "dmk: wrtProt=" << (int)dmk->m_wrtProt << ",tracks=" << (int)dmk->m_trackCount << endl;

  if (dmk->m_options & (1 << 4)) {
    if (m_verbose)
      cout << "dmk: disk is single density only" << endl;
  }  
  if (dmk->m_options & (1 << 6)) {
    if (m_verbose)
      cout << "dmk: disk is single density with double option" << endl;
  }  
  if (dmk->m_options & (1 << 7)) {
    if (m_verbose)
      cout << "dmk: disk is double density with single option" << endl;
  }  

  int j;
  for (j = 0; j < DMK_TRACKLENGTHS_COUNT; ++j) {
    if (dmk->m_trackLen == g_dmk_trackLengths[j])
      break;
  }
  if (j == DMK_TRACKLENGTHS_COUNT) {
    m_error << "dmk - format looks like DMK, but track length " << dmk->m_trackLen << " is not recognised" << endl;
    return false;
  }

  m_fd = fd;
  m_trackCount = dmk->m_trackCount;
  off_t offs = 0x10;

  for (int trackNum = 0; trackNum < m_trackCount; ++trackNum) {
    uint16_t sectorOffsets[64];
    if (::lseek(m_fd, offs, SEEK_SET) < 0) {
      m_error << "dmk: cannot seek to header for track " << trackNum << " at offset " << offs;
      return false;
    }
    else if (::read(m_fd, sectorOffsets, sizeof(sectorOffsets)) != sizeof(sectorOffsets)) {
      m_error << "dmk - cannot read header for track " << trackNum << " at offset " << offs;
      return false;
    }
    else {
      for (int sectorNumber = 0; sectorNumber < 64; ++sectorNumber) {
        if (sectorOffsets[sectorNumber] == 0)
          break;
        DMKSector sector; 
        int idamOffs       = (uint8_t *)&sector.m_idam - (uint8_t *)&sector;
        int headerLen      = (uint8_t *)&sector.m_data - (uint8_t *)&sector;
        int idamToDataOffs = (uint8_t *)&sector.m_data - (uint8_t *)&sector.m_idam;
        int sectorSize = 256;
        int density = 0;
        int side = 0;

        off_t sectorOffset = sectorOffsets[sectorNumber] & 0x3fff; 

        if (
            (::lseek(m_fd, offs + sectorOffset-idamOffs, SEEK_SET) < 0) ||
            (::read(m_fd, &sector, headerLen) != headerLen)
          ) {
          m_error << "dmk - cannot read info for sector " << sectorNumber << ", track " << trackNum;;
        }
        else {
          if (m_verbose)
            cout << "dmk: idam=" << HEXFORMAT0x2(sector.m_idam) << ","
                << "track=" << (int) sector.m_track << "," 
                << "sector=" << (int) sector.m_sector << "," 
                << "dam=" << HEXFORMAT0x2(sector.m_dam) << ","
                << "size=" << sizeof(sector) << endl; 

          m_trackCount = std::max(m_trackCount, (int)trackNum);
          m_sectorMap.emplace(HASH_STS(0, trackNum, sector.m_sector+1), 
              SectorInfo(offs + sectorOffset + idamToDataOffs, sectorSize, sector.m_dam, density));
        }

        m_sectorCount = std::max((int)sector.m_sector+1, m_sectorCount );
        m_sectorSize  = std::max((int)sectorSize, m_sectorSize);
        m_density     = std::max((int)density,    m_density);
        m_sideCount   = std::max((int)(side+1),   m_sideCount);
      }
    }

    offs += dmk->m_trackLen;
  }

  return true;
}


