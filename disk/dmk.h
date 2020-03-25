
#ifndef VIRTUAL_DRIVE_DMK_H_
#define VIRTUAL_DRIVE_DMK_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "virtual_drive.h"

class VirtualDriveDMK : public VirtualDriveFile
{
  public:
#pragma pack(1)
    struct DMKHeader
    {
      uint8_t  m_wrtProt;     // FF = write protected, 00 = read/write
      uint8_t  m_trackCount;  // number of tracks
      uint16_t m_trackLen;    // 0x80h + track length
      uint8_t  m_options;     // various flags
      uint8_t  m_reserved[7]; // not used 
      uint32_t m_virtualFlag; // 12345678h if real disk, else zero
    };

    // 304 bytes
    struct DMKSector
    {
      uint8_t m_hdr1_0xff[14];  // 0xff
      uint8_t m_hdr2_0x00[6];   // 0x00
      uint8_t m_idam;           // 0xfe
      uint8_t m_track;          // 0..n
      uint8_t m_side;           // 0x00
      uint8_t m_sector;         // 1..n
      uint8_t m_hdr4_0x01;      // 0x00 or 0x01 
      uint8_t m_crc1[2];        // crc
      uint8_t m_hdr5_0xff[11];  // 0xff 
      uint8_t m_hdr6_0x00[6];   // 0x00 
      uint8_t m_dam;            // 0xfb
      uint8_t m_data[256];      // data
      uint8_t m_unknown1_0x00;  // 0x00 ?
      uint8_t m_crc2[2];        // crc
    };
#pragma pack()

    VirtualDriveDMK();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
};

#endif // VIRTUAL_DRIVE_DMK_H_