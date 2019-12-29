#ifndef FDC_H_
#define FDC_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

#include "config.h"

#define MAX_SECTOR_SIZE  1024

class VirtualDrive
{
  public:
    VirtualDrive();
    ~VirtualDrive();

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

    virtual bool Open(const std::string & name, bool readOnly) = 0;
    virtual bool Mount(bool readOnly) = 0;

    virtual bool IsReadOnly() const;
    virtual std::string GetName() const;

    virtual int ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len) = 0;
    virtual int WriteSector(int track, int sector, uint8_t * data, int len) = 0;

  protected:  
    bool m_readOnly;
    std::string m_name;
};

class VirtualDriveFile : public VirtualDrive
{
  public:
    // update g_formatNames in fdc.cc if this is changed
    enum class Format {
      eUnknown,
      eJV1,
      eJV3,
      eDMK,
      eCount
    };

    VirtualDriveFile();
    ~VirtualDriveFile();

    void ReadJV1(off_t len, std::stringstream & formatError);
    void ReadJV3(off_t len, std::stringstream & formatError);
    void ReadDMK(off_t len, std::stringstream & formatError);

    virtual bool Open(const std::string & name, bool readOnly) override;
    virtual bool Mount(bool readOnly) override;

    virtual int ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len) override;
    virtual int WriteSector(int track, int sector, uint8_t * data, int len) override;


  protected:
    int m_fd;
    Format m_format;
    int m_trackCount;
    std::map<uint32_t, SectorInfo> m_sectorMap;
};

class WD_FDC
{
  public:
    WD_FDC();

    bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    void Write(uint16_t addr, uint8_t val);
    uint8_t Read(uint16_t addr);

    bool SelectDrive(int drive);

    void SetInterruptHandler(std::function<void ()> handler);

    void Reset();

  protected:
    void ResetStatus();

    uint8_t ReadStatus();
    uint8_t ReadData();

    void WriteCommand(int8_t command);

    uint8_t m_cmd;
    uint8_t m_status;
    uint8_t m_track;
    uint8_t m_sector;
    uint8_t m_data;

    int m_state;
    int m_drive;
    std::chrono::system_clock::time_point m_timer;
    bool m_noPrint;
    bool m_setInterrupt;

    bool m_intOnNotReadyToReady;

    std::function<void ()> m_interruptHandler;
    std::vector<std::unique_ptr<VirtualDrive>> m_drives;

    uint8_t m_buffer[MAX_SECTOR_SIZE];
    uint8_t m_density;
    int m_bufferLen;
    int m_bufferPtr;
    uint8_t m_dam;
};

#endif // FDC_H_