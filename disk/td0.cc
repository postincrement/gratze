#include <iostream>
#include <unistd.h>

using namespace std;

#include "common/misc.h"
#include "disk/td0.h"
#include "disk/td0_compress.h"

// http://www.willsworks.net/file-format/teledisk
// https://hwiegman.home.xs4all.nl/fileformats/teledisk/wteledsk.htm

#pragma pack(1)
struct TD0Header
{
  char     m_id[2];         //  0 "TD" if normal compression
                            //      "td" if advanced compression
  uint8_t  m_volumeSeq;     //  2 first volume is volume 0
  uint8_t  m_checkSig;      //  3 common value for all disks in a set
  uint8_t  m_version;       //  4 version of TELEDISK used to create the image
  uint8_t  m_srcDensity;    //  5 source density:
                            //      0 = 250kps, 1 = 300kpbs, 2 = 500kbps. 
                            //      Add 0x80 if SD FM
  uint8_t  m_driveType;     //  6 1 = 360k, 2 = 1.2M, 3 = 720k, 4 = 1.44M, 
  uint8_t  m_trackDensity;  //  7 0 = source media matches media density
                            //      1 = dbl density in quad drive
                            //      2 quad density in dbl drive
                            //     Add 0x80 if comment follows image
  uint8_t  m_dosMode;       //  8 non-zero if disk was analyszed as DOS ??
  uint8_t  m_sides;         //  9 1 = single sided, 2 = dbl sided
  uint16_t m_crc;           // 10 16 bit CRC for header
};

struct TrackHeader 
{
  uint8_t  m_sectorCount;
  uint8_t  m_track;
  uint8_t  m_side;
  uint8_t  m_crc;
};

struct SectorHeader
{
  uint8_t  m_track;   
  uint8_t  m_head;
  uint8_t  m_sector;
  uint8_t  m_size;     // 
  uint8_t  m_flags;
  uint8_t  m_unknown;
};

struct ExtSectorHeader {
  uint8_t  m_6;
  uint8_t  m_7;
  uint8_t  m_8;
};

#pragma pack()


constexpr int compressedBlockSize = 6144;
constexpr int lzCodeSize          = 12;

#pragma pack()

//////////////////////////////////////////////////////////////////////

VirtualDriveTD0::VirtualDriveTD0()
  : VirtualDriveFile("td0")
{
}

bool VirtualDriveTD0::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{
  // if file length is less than header len, definitely not this foemat
  if (len < sizeof(TD0Header))
    return false;

  TD0Header & td0 = *(TD0Header *)header;
  std::string id(td0.m_id, sizeof(td0.m_id));
  bool compressed = false;

  if (id == "td") {
    compressed = true;
  }
  else if (id == "TD") {
    compressed = false;
  }
  else {
    cerr << "td0: unknown id '" << id << "'" << endl;
    return false;
  }

  const int max_size = 4*1024*1024; // 4MB ought to be large enough for any floppy
	std::vector<uint8_t> imagebuf(max_size);

  m_fd = fd;  

  // seek to start of data (compressed or not)
  if (::lseek(m_fd, sizeof(TD0Header), SEEK_SET) < 0) {
    m_error << "td0: cannot seek to data at offset " << sizeof(TD0Header) << endl;
    return false;
  }

  // read (un)compressed data
  if (compressed) {
    TD0_Decoder decoder(m_fd, 12);
		decoder.Init();
		imagebuf.resize(decoder.Decode(&imagebuf[0], max_size));
  }
  else {
    int payloadLen = len - sizeof(TD0Header);
    if (::read(m_fd, &imagebuf[0], payloadLen) != payloadLen) {
      m_error << "td0: cannot read payload of length " << payloadLen << endl;
      return false;
    }
    imagebuf.resize(payloadLen);
  }

  cerr << "td0: id = '" << id << "', version " << (int)td0.m_version << ", length " << imagebuf.size() << endl;

  size_t offs = 0;
  if (td0.m_trackDensity & 0x80) {
    size_t commentOffs = 10;
    size_t commentLen  = imagebuf[2] + (imagebuf[3] << 8);
    cout << DumpMemory(&imagebuf[commentOffs], commentLen);

    offs = commentOffs + commentLen;
  }

  int sectorsPerTrack = imagebuf[offs];
  if (sectorsPerTrack == 255)
    return false;

  cout << sectorsPerTrack << " sectors per track" << endl;

  while (sectorsPerTrack != 255) {

    TrackHeader & trackInfo = *(TrackHeader *)&imagebuf[offs];

		int track = trackInfo.m_track; 
		int head  = trackInfo.m_side & 1;

		offs += sizeof(TrackHeader);

		for (int i = 0; i < sectorsPerTrack; i++) {

      SectorHeader & sectorHeader = *(SectorHeader *)&imagebuf[offs];
			offs += sizeof(SectorHeader);

			uint16_t size;

      cout << "head " << (int)sectorHeader.m_head << ", track " << (int)track << ", sector " << (int)sectorHeader.m_sector;

			if (sectorHeader.m_flags & 0x30)
				size = 0;
			else {
        size = 128 << sectorHeader.m_size;
        ExtSectorHeader & extHdr = *(ExtSectorHeader *)&imagebuf[offs];
        offs += sizeof(ExtSectorHeader);
        switch (extHdr.m_8) {
          case 0:
            cerr << ", type 0";
            memcpy(&sect_data[sdatapos], &imagebuf[offset], size);
            offs += size;
            break;

          case 1:
            cerr << ", type 1";
            offs += 4;
            break;

          case 2:
            cerr << ", type 2";
            {
              int k = 0;
						  while (k < size) {
							  uint16_t len = imagebuf[offs];
							  uint16_t rep = imagebuf[offs + 1];
							  offs += 2;
							  if (!len) {
								  //memcpy(&sect_data[sdatapos + k], &imagebuf[offset], rep);
								  offs += rep;
								  k += rep;
							  }
							  else {
								  len = (1 << len);
								  rep = len * rep;
								  rep = ((rep + k) <= size) ? rep : (size - k);
								  //for (j = 0; j < rep; j += len)
									  //memcpy(&sect_data[sdatapos + j + k], &imagebuf[offset], len);
								  k += rep;
								  offs += len;
							  }
              }
						}
            break;

          default:  
            cerr << "unsupported mode " << (int)extHdr.m_8 << endl;
            return false;
        }
      }

      cout << ", len " << (int)size << endl;
    }

    sectorsPerTrack = imagebuf[offs];
  }

#if 0

  off_t offs = sizeof(TD0Header);
  // read track header
  if (::lseek(m_fd, offs, SEEK_SET) < 0) {
    m_error << "td0: cannot seek to first track header at offset " << offs << endl;
    return false;
  }

  int trackNum = 0;
  for (;;) {

    // read track header
    TrackHeader hdr;
    if (::read(m_fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
      m_error << "td0 - cannot read track header for track " << (int)trackNum << endl;
      return false;
    }

    cout << "td0: track " << (int)hdr.m_track << " with " << (int)hdr.m_sectorCount << " sectors" << endl;
    break;
  }    
#endif

  return true;
}



