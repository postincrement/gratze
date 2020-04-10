#ifndef VIRTUAL_DRIVE_H_
#define VIRTUAL_DRIVE_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <sstream>

#include "common/factory.h"

class VirtualDrive;
using VirtualDriveFactory = Factory<VirtualDrive, std::string>;

#define MAX_SECTOR_SIZE  1024

#define HASH_STS(side, track, sector)     (sector + (side << 8) + (track << 16))

class VirtualDrive;

class VirtualFileIdentifier
{
  public:
    VirtualFileIdentifier();

    void SetVerbose(bool verbose);

    std::string GetError() const;

    VirtualDrive * Open(const std::string & fn, bool readOnly);

  protected:
    bool m_verbose;
    std::stringstream m_error;  
};

class VirtualDrive
{
  public:
    static void Init();

    VirtualDrive();
    virtual ~VirtualDrive();

    void SetVerbose(bool v);

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

    virtual std::string GetFormat() const = 0;
    virtual std::string GetExtension() const = 0;

    virtual int GetTracks() const = 0;
    virtual int GetSectors() const = 0;
    virtual int GetSectorSize() const = 0;
    virtual int GetSides() const = 0;
    virtual int GetDensity() const = 0;
    virtual int GetMinSector() const = 0;
    virtual int GetMinTrack() const = 0;

    virtual std::string GetError() const;

    virtual SectorInfo * GetInfo(int side, int track, int sector) = 0;
    virtual int ReadSector(int track, int side, int sector, SectorInfo & info, uint8_t * data, int len) = 0;
    virtual int WriteSector(int track, int side, int sector, uint8_t * data, int len) = 0;

    template <class Type>
    static void AddFormat()
    {
      std::unique_ptr<VirtualDrive> virtualDrive(new Type());

      std::string key       = virtualDrive->GetFormat();
      m_virtualDriveFactory.AddConcreteClass<Type>(key);
    }

    static VirtualDriveFactory m_virtualDriveFactory;

    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) = 0;
    
  protected:
    bool m_readOnly;
    bool m_verbose;
    std::stringstream m_error;
};

class VirtualDriveFile : public VirtualDrive
{
  public:
    VirtualDriveFile();
    VirtualDriveFile(const std::string & extension);
    VirtualDriveFile(const std::string & name, const std::string & ext);
    ~VirtualDriveFile();

    virtual std::string GetFormat() const override;
    virtual std::string GetExtension() const override;

    virtual bool Mount(bool readOnly) override;

    virtual SectorInfo * GetInfo(int side, int track, int sector) override;
    virtual int ReadSector(int track, int side, int sector, SectorInfo & info, uint8_t * data, int len) override;
    virtual int WriteSector(int track, int side, int sector, uint8_t * data, int len) override;

    virtual int GetTracks() const override;
    virtual int GetSectors() const override;
    virtual int GetSectorSize() const override;
    virtual int GetSides() const override;
    virtual int GetDensity() const override;
    virtual int GetMinSector() const override;
    virtual int GetMinTrack() const override;

  protected:
    int m_fd;
    std::string m_name;
    std::string m_extension;
    int m_minSector = 0;
    int m_maxSector = 0;
    int m_sectorSize;
    int m_minTrack = 0;
    int m_trackCount = 0;
    int m_density;
    int m_sideCount;
    std::map<uint32_t, SectorInfo> m_sectorMap;
};


#endif // VIRTUAL_DRIVE_H_