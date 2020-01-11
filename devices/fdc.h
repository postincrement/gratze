#ifndef FDC_H_
#define FDC_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

#include "src/factory.h"
#include "src/config.h"

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
    void SetDriveChangedHandler(std::function<void (int, bool)> handler);

    void Reset();

    typedef int (WD_FDC::*CommandFunction)(uint8_t cmd); 

    struct CommandInfo
    {
      uint8_t m_andMask;
      uint8_t m_cmd;

      const char * m_name;
      int m_type;

      CommandFunction m_function;
    };

    virtual void Run();

    bool IsCurrentDriveAvailable() const;

    // type I commands
    int HomeCommand(uint8_t cmd);
    int SeekCommand(uint8_t cmd);
    int StepCommand(uint8_t cmd);
    int StepInCommand(uint8_t cmd); 
    int StepOutCommand(uint8_t cmd);

    // type II commands
    int ReadCommand(uint8_t cmd);

    // type IV commands
    int ForceIntCommand(uint8_t cmd);

  protected:
    virtual CommandInfo * GetCommand(uint8_t cmd);
    void UpdateInterrupt(bool interruptOn);
    void WriteCmdReg(int8_t command);
    uint8_t ReadStatusReg();
    uint8_t ReadDataReg();

    int SeekTrack(uint8_t cmd, uint8_t track, bool update);
    void RestartHeadLoadTimer();
    void LoadHead(bool load);
    void SetTypeIStatus();

    int m_state;
    int m_drive;
    bool m_headLoaded;
    bool m_interrupt;
    double m_diskRevTime_ms;
    std::chrono::system_clock::time_point m_headLoadtimer;

    bool m_directionIn;
    bool m_setInterrupt;
    uint8_t m_realTrack;

    int m_currentCommand;  // currently active command, or -1
    uint8_t m_statusMask;  // how to mask the status reg at the end of the command

    // copies of registers
    uint8_t m_status;
    uint8_t m_track;
    uint8_t m_sector;
    uint8_t m_data;

    std::function<void ()> m_interruptHandler;
    std::vector<std::unique_ptr<VirtualDrive>> m_drives;

    std::function<void (int, bool)> m_driveChangedHandler;

    uint8_t m_buffer[MAX_SECTOR_SIZE];
    uint8_t m_density;
    int m_bufferLen;
    int m_bufferPtr;
    bool m_reading;
    bool m_writing;

    ///////////////////////
/*
    void ResetStatus();



    uint8_t m_cmd;

    std::chrono::system_clock::time_point m_timer;
    bool m_noPrint;
    bool m_setInterrupt;

    bool m_intOnNotReadyToReady;


 */   
};

class WD_FD1771 : public WD_FDC
{
  public:
    WD_FD1771();
};


#endif // FDC_H_