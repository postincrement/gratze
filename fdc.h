#ifndef FDC_H_
#define FDC_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class VirtualDrive
{
  public:
    VirtualDrive();
    ~VirtualDrive();

    virtual bool Open(const std::string & name, int sectorSize) = 0;
    virtual bool Mount(bool readOnly) = 0;

    virtual std::string GetName();

    virtual bool ReadSector(int track, int sector, uint8_t * data, int len) = 0;
    virtual bool WriteSector(int track, int sector, uint8_t * data, int len) = 0;

  protected:  
    std::string m_name;
};

class VirtualDriveFile : public VirtualDrive
{
  public:
    VirtualDriveFile();
    ~VirtualDriveFile();

    virtual bool Open(const std::string & name, int sectorSize) override;
    virtual bool Mount(bool readOnly) override;

    virtual bool ReadSector(int track, int sector, uint8_t * data, int len) override;
    virtual bool WriteSector(int track, int sector, uint8_t * data, int len) override;
};

class WD_FDC
{
  public:
    WD_FDC();

    bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    void Write(uint16_t addr, uint8_t val);
    uint8_t Read(uint16_t addr);

    bool SelectDrive(int drive);

    uint8_t ReadStatus();
    void WriteCommand(int8_t command);

    void SetInterruptHandler(std::function<void ()> handler);

  protected:
    uint8_t m_cmd;
    uint8_t m_status;
    uint8_t m_track;
    uint8_t m_sector;
    uint8_t m_data;

    int m_state;
    int m_drive;
    std::chrono::system_clock::time_point m_timer;
    bool m_noPrint;

    std::function<void ()> m_interruptHandler;

    std::vector<std::unique_ptr<VirtualDrive>> m_drives;
};

#endif // FDC_H_