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

#define   SD_SECTOR_SIZE    256

#define HASH_STS(side, track, sector)     (sector + (side << 8) + (track << 16))

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
    bool m_debug = false;
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


#endif // VIRTUAL_DRIVE_H_