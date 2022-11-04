

//
//  Based on information from 
//  http://www.cpcwiki.eu/index.php/Format:DSK_disk_image_file_format
//

/*

  0x0000 0x00033   disk information block
  0x0034 0x000ff   padding

  0x0100 0x0117    track 0 information block
  0x0118 0x011f    track 0, sector 1 information block
  0x0120 0x0127    track 0, sector 2 information block
  ...
  0x01f8 0x01ff    track 0, sector n information block

  0x0200 ....      track 0, sector 1 data
  ...
  
*/

#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;

#include "virtual_drive.h"
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

struct DSKTrackInfo
{
  char     m_id[12];       // ID
  uint8_t  m_unused[4];
  uint8_t  m_track;        // track number
  uint8_t  m_side;         // side
  uint8_t  m_unused2[2];
  uint8_t  m_sectorSize;   // sector size (2 = 512)
  uint8_t  m_sectorCount;  // number of sectors
  uint8_t  m_gap3Count;
  uint8_t  m_filler;       // filler byte (normally 0xe5)
};

struct DSKSectorHeader
{
  uint8_t m_track;         // track (equivalent to C parameter in NEC765 commands
  uint8_t m_side;          // side (equivalent to H parameter in NEC765 commands)	
  uint8_t m_sectorID;      // sector ID (equivalent to R parameter in NEC765 commands)
  uint8_t m_sectorSize;    // sector size (equivalent to N parameter in NEC765 commands)
  uint8_t m_stat1;         // FDC status register 1 (equivalent to NEC765 ST1 status register)
                           // The following bits are used from NEC765 status register 1:
                           // b7 EN (End of Cylinder)
                           // b5 DE (Data Error)
                           // b2 ND (No Data)
                           // b0 MA (Missing Address Mark)
  uint8_t m_stat2;         // FDC status register 2 (equivalent to NEC765 ST2 status register)
                           // The following bits are used from NEC765 status register 2:
                           // b5 CM (Control Mark)
                           // b5 DD (Data Error in Data field)
                           // b0 MD (Missing address Mark in Data field)
  uint8_t m_unused[2];	
};

#define SECTOR_HEADER_COUNT ((256 - sizeof(DSKTrackInfo)) / sizeof(DSKSectorHeader))

struct DSKTrackHeader
{
  DSKTrackInfo    m_track;
  DSKSectorHeader m_sectors[SECTOR_HEADER_COUNT];
};

#pragma pack()

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
    return false;
  }

  // read entire header
  DSKHeader dskHeader;
  if (::read(fd, &dskHeader, sizeof(dskHeader)) != sizeof(dskHeader)) {
    m_error << "dsk: cannot read file header";
    return false;
  }

  //cerr << "dsk: track length = " << dskHeader.m_trackSize << endl;

  int trackCount = dskHeader.m_tracks;
  off_t offs = 0x100;

  for (int trackNum = 0; trackNum < trackCount; ++trackNum) {
    for (int side = 0; side < dskHeader.m_sides; ++side) {
      DSKTrackHeader dskTrackHeader;
      const DSKTrackInfo & dskTrackInfo = dskTrackHeader.m_track;

      if (::lseek(fd, offs, SEEK_SET) < 0) {
        m_error << "dsk: cannot seek to header for track " << trackNum << " at offset " << offs;
        return false;
      }
      else if (::read(fd, &dskTrackHeader, sizeof(dskTrackHeader)) != sizeof(dskTrackHeader)) {
        m_error << "dsk - cannot read header for track " << trackNum << " at offset " << offs;
        return false;
      }
      else if (memcmp(dskTrackHeader.m_track.m_id, TRACK_HEADER_ID, TRACK_HEADER_ID_LEN) != 0) {
        m_error << "dsk: header for track " << trackNum << " at offset " << HEXFORMAT0x4(offs) << " has bad ID";
        return false;
      }

      TrackInfo trackInfo;

      int sectorSizeBytes = dskTrackInfo.m_sectorSize*256;
      int density = 0;

      off_t dataOffs = offs + 0x100;
      uint8_t dam = 0xfb;

      DSKSectorHeader * sectorHeader = dskTrackHeader.m_sectors;
      for (int sector = 0; sector < dskTrackInfo.m_sectorCount; ++sector) {

        //cout << "dsk: sector ID " << (int)sectorHeader->m_sectorID << " has offset " << dataOffs << endl;
        SectorInfo sectorInfo(sectorHeader->m_sectorID, dataOffs, sectorSizeBytes, dam, density);

        trackInfo.AddSector(sectorInfo);

        ++sectorHeader;
        dataOffs += sectorSizeBytes;
      }

      AddTrack(side, trackNum, trackInfo);

      offs += 0x100 + dskTrackInfo.m_sectorCount * sectorSizeBytes;
    }
  }

  m_fd = fd;
  return true;
}
