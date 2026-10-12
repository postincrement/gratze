#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

#include "common/misc.h"
#include "disk/td0.h"
#include "disk/td0_compress.h"

// http://www.willsworks.net/file-format/teledisk
// http://dunfield.classiccmp.org/img42841/td0notes.txt

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
  uint8_t  m_dosMode;       //  8 non-zero if disk was analysed as DOS ??
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
  uint8_t  m_size;
  uint8_t  m_flags;
  uint8_t  m_crc;
};
#pragma pack()

//////////////////////////////////////////////////////////////////////

VirtualDriveTD0::VirtualDriveTD0()
  : VirtualDriveFile("td0")
{
}

bool VirtualDriveTD0::OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize)
{
  if (len < (off_t)sizeof(TD0Header))
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
    m_error << "td0: unknown id '" << id << "'";
    return false;
  }

  const int max_size = 4*1024*1024;
  std::vector<uint8_t> imagebuf(max_size);

  m_fd = fd;

  if (::lseek(m_fd, sizeof(TD0Header), SEEK_SET) < 0) {
    m_error << "td0: cannot seek to data at offset " << sizeof(TD0Header) << endl;
    return false;
  }

  if (compressed) {
    TD0_Decoder decoder(m_fd, 12);
    decoder.Init();
    imagebuf.resize(decoder.Decode(&imagebuf[0], max_size));
  }
  else {
    int payloadLen = (int)(len - sizeof(TD0Header));
    if (::read(m_fd, &imagebuf[0], payloadLen) != payloadLen) {
      m_error << "td0: cannot read payload of length " << payloadLen << endl;
      return false;
    }
    imagebuf.resize(payloadLen);
  }

  if (m_verbose)
    cerr << "td0: id = '" << id << "', version " << (int)td0.m_version
         << ", length " << imagebuf.size() << endl;

  size_t offs = 0;
  if (td0.m_trackDensity & 0x80) {
    size_t commentLen = imagebuf[2] + (imagebuf[3] << 8);
    size_t commentOffs = 10;
    if (m_verbose)
      cout << DumpMemory(&imagebuf[commentOffs], commentLen);
    offs = commentOffs + commentLen;
  }

  // FM (single density) if source density has 0x80 set.
  const uint8_t density = (td0.m_srcDensity & 0x80) ? 0 : 1;
  m_density = density;

  m_imageData.clear();

  int sectorsPerTrack = imagebuf[offs];
  if (sectorsPerTrack == 255)
    return false;

  if (m_verbose)
    cout << sectorsPerTrack << " sectors per track" << endl;

  while (sectorsPerTrack != 255) {
    if (offs + sizeof(TrackHeader) > imagebuf.size()) {
      m_error << "td0: truncated track header";
      return false;
    }

    TrackHeader & trackInfo = *(TrackHeader *)&imagebuf[offs];
    int track = trackInfo.m_track;
    int head  = trackInfo.m_side & 1;
    offs += sizeof(TrackHeader);

    TrackInfo * trackMap = GetTrack(head, track);
    if (trackMap == nullptr) {
      AddTrack(head, track, TrackInfo());
      trackMap = GetTrack(head, track);
    }

    for (int i = 0; i < sectorsPerTrack; i++) {
      if (offs + sizeof(SectorHeader) > imagebuf.size()) {
        m_error << "td0: truncated sector header";
        return false;
      }

      SectorHeader & sectorHeader = *(SectorHeader *)&imagebuf[offs];
      offs += sizeof(SectorHeader);

      int size = 0;
      if (!(sectorHeader.m_flags & 0x30))
        size = 128 << sectorHeader.m_size;

      if (m_verbose) {
        cout << "head " << (int)sectorHeader.m_head
             << ", track " << (int)track
             << ", sector " << (int)sectorHeader.m_sector;
      }

      std::vector<uint8_t> sectorData(size > 0 ? size : 0);

      if (sectorHeader.m_flags & 0x30) {
        // No data / deleted / skipped — synthesise a blank sector if size known.
        if (size <= 0)
          size = 128 << (sectorHeader.m_size <= 6 ? sectorHeader.m_size : 2);
        sectorData.assign(size, (sectorHeader.m_flags & 0x10) ? 0xf6 : 0x00);
      }
      else {
        if (offs + 3 > imagebuf.size()) {
          m_error << "td0: truncated sector data header";
          return false;
        }

        // Data block size (includes encoding byte), then encoding method.
        uint16_t blockLen = imagebuf[offs] | (imagebuf[offs + 1] << 8);
        uint8_t encoding  = imagebuf[offs + 2];
        offs += 3;
        (void)blockLen;

        if (m_verbose)
          cout << ", type " << (int)encoding;

        switch (encoding) {
          case 0: // raw
            if (offs + (size_t)size > imagebuf.size()) {
              m_error << "td0: truncated raw sector";
              return false;
            }
            memcpy(sectorData.data(), &imagebuf[offs], size);
            offs += size;
            break;

          case 1: { // repeated 2-byte pattern
            if (offs + 4 > imagebuf.size()) {
              m_error << "td0: truncated type-1 sector";
              return false;
            }
            uint16_t count = imagebuf[offs] | (imagebuf[offs + 1] << 8);
            uint8_t p0 = imagebuf[offs + 2];
            uint8_t p1 = imagebuf[offs + 3];
            offs += 4;
            int k = 0;
            int nbytes = (int)count * 2;
            if (nbytes > size)
              nbytes = size;
            while (k + 1 < nbytes) {
              sectorData[k++] = p0;
              sectorData[k++] = p1;
            }
            if (k < size)
              memset(sectorData.data() + k, 0, size - k);
            break;
          }

          case 2: { // RLE
            int k = 0;
            while (k < size) {
              if (offs + 2 > imagebuf.size()) {
                m_error << "td0: truncated type-2 sector";
                return false;
              }
              uint8_t len = imagebuf[offs];
              uint8_t rep = imagebuf[offs + 1];
              offs += 2;
              if (!len) {
                if (offs + rep > imagebuf.size() || k + rep > size) {
                  m_error << "td0: type-2 literal overflow";
                  return false;
                }
                memcpy(sectorData.data() + k, &imagebuf[offs], rep);
                offs += rep;
                k += rep;
              }
              else {
                int block = 1 << len;
                int total = block * rep;
                if (total + k > size)
                  total = size - k;
                if (offs + (size_t)block > imagebuf.size()) {
                  m_error << "td0: type-2 repeat overflow";
                  return false;
                }
                for (int j = 0; j < total; j += block)
                  memcpy(sectorData.data() + k + j, &imagebuf[offs],
                         (j + block <= total) ? block : (total - j));
                k += total;
                offs += block;
              }
            }
            break;
          }

          default:
            m_error << "td0: unsupported encoding " << (int)encoding;
            return false;
        }
      }

      if (m_verbose)
        cout << ", len " << size << endl;

      if (size > 0) {
        off_t dataOffs = (off_t)m_imageData.size();
        m_imageData.insert(m_imageData.end(), sectorData.begin(), sectorData.end());

        // Normal data mark; deleted-data flag uses 0xf8.
        uint8_t dam = (sectorHeader.m_flags & 0x04) ? 0xf8 : 0xfb;
        SectorInfo sectorInfo(sectorHeader.m_sector, dataOffs, size, dam, density);
        trackMap->AddSector(sectorInfo);
      }
    }

    if (offs >= imagebuf.size())
      break;
    sectorsPerTrack = imagebuf[offs];
  }

  return true;
}

int VirtualDriveTD0::ReadSector(int side, int track, int sector, SectorInfo & info, uint8_t * data, int len)
{
  auto const sectorInfo = GetInfo(side, track, sector);
  if (sectorInfo == nullptr)
    return -1;

  info = *sectorInfo;

  if ((info.m_offset < 0) ||
      (info.m_offset + info.m_size > (off_t)m_imageData.size())) {
    m_error << "td0: sector data out of range";
    return -1;
  }

  if (len > info.m_size)
    len = info.m_size;

  if (m_verbose)
    cerr << "td0: read side " << side << ",track " << track
         << ",sector " << sector << " len " << len << endl;

  memcpy(data, m_imageData.data() + info.m_offset, len);
  return len;
}
