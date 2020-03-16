#ifndef VIRTUAL_DRIVE_H_
#define VIRTUAL_DRIVE_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

#include "common/factory.h"
#include "src/config.h"

class VirtualDrive;
using VirtualDriveFactory = Factory<VirtualDrive, std::string>;

#define MAX_SECTOR_SIZE  1024

class VirtualDrive
{
  public:
    static void Init();

    VirtualDrive();
    virtual ~VirtualDrive();

    static VirtualDrive * Open(const std::string & fn, bool readOnly);

    struct SectorInfo
    {
      SectorInfo() = default;
      SectorInfo(const SectorInfo & obj) = default;
      SectorInfo(off_t offset, int size, uint8_t dam, uint8_t density)
        : m_offset(offset)
        , m_size(size)
        , m_dam(dam)
        , m_density(density)
      {}

      off_t m_offset;
      int m_size;
      uint8_t m_dam;
      uint8_t m_density;
    };

    virtual bool Mount(bool readOnly) = 0;

    virtual bool IsReadOnly() const;

    virtual std::string GetName() const = 0;
    virtual std::string GetExtension() const = 0;

    virtual int ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len) = 0;
    virtual int WriteSector(int track, int sector, uint8_t * data, int len) = 0;

    template <class Type>
    static void AddFormat()
    {
      std::unique_ptr<VirtualDrive> virtualDrive(new Type());

      std::string key       = virtualDrive->GetName();
      g_virtualDriveFactory.AddConcreteClass<Type>(key);
    }

  protected:
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) = 0;
    bool m_readOnly;

    static VirtualDriveFactory g_virtualDriveFactory;
};

class VirtualDriveFile : public VirtualDrive
{
  public:
    VirtualDriveFile();
    VirtualDriveFile(const std::string & extension);
    VirtualDriveFile(const std::string & name, const std::string & ext);
    ~VirtualDriveFile();

    virtual std::string GetName() const override;
    virtual std::string GetExtension() const override;

    virtual bool Mount(bool readOnly) override;

    virtual int ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len) override;
    virtual int WriteSector(int track, int sector, uint8_t * data, int len) override;

  protected:
    int m_fd;
    int m_trackCount;
    std::map<uint32_t, SectorInfo> m_sectorMap;
    std::string m_name;
    std::string m_extension;
};

//    void ReadJV1(off_t len, std::stringstream & formatError);
//    void ReadJV3(off_t len, std::stringstream & formatError);
//    void ReadDMK(off_t len, std::stringstream & formatError);

class VirtualDriveJV1 : public VirtualDriveFile
{
  public:
    VirtualDriveJV1();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
};

class VirtualDriveJV3 : public VirtualDriveFile
{
  public:
    VirtualDriveJV3();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
};

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

#endif // VIRTUAL_DRIVE_H_