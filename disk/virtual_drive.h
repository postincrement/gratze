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

    class SectorInfo
    {
      public:
        SectorInfo() = default;
        SectorInfo(const SectorInfo & obj) = default;
        SectorInfo(int id, off_t offset, int size, uint8_t dam, uint8_t density)
          : m_id(id)
          , m_offset(offset)
          , m_size(size)
          , m_dam(dam)
          , m_density(density)
        {}

        int     m_id = -1;
        off_t   m_offset = 0;
        int     m_size = 0;
        uint8_t m_dam = 0;
        uint8_t m_density = 0;
    };

    class TrackInfo
    {
      public:
        TrackInfo();

        virtual void AddSector(const SectorInfo & sector);

        virtual SectorInfo * GetSector(int sector);
        virtual const SectorInfo * GetSector(int sector) const;

        std::vector<SectorInfo> m_sectors;
    };

    virtual bool Mount(bool readOnly) = 0;

    virtual bool IsReadOnly() const;

    virtual std::string GetFormat() const = 0;
    virtual std::string GetExtension() const = 0;

    virtual void AddTrack(int trackNum, const TrackInfo & track);
    
    virtual TrackInfo * GetTrack(int trackNum);
    virtual const TrackInfo * GetTrack(int trackNum) const;

    virtual int GetSides() const = 0;
    virtual int GetDensity() const = 0;

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
    int  m_density;
    int  m_sideCount;
    std::stringstream m_error;
    std::map<int, TrackInfo> m_tracks;
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

    virtual int GetSides() const override;
    virtual int GetDensity() const override;

  protected:
    int m_fd;
    std::string m_name;
    std::string m_extension;
};


#endif // VIRTUAL_DRIVE_H_