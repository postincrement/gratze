#ifndef FDC_H_
#define FDC_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

#include "disk/virtual_drive.h"
#include "common/factory.h"
#include "src/config.h"

class WD_FDC
{
  public:
    enum {
      eWD1771 = 1,
      eWD1791 = 2,
      eWD1793 = 4,
      eWD2793 = 8
    };

    WD_FDC(unsigned int mode, bool debug);

    bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    void Write(uint16_t addr, uint8_t val);
    uint8_t Read(uint16_t addr);

    bool SelectDrive(int drive);
    bool SelectSide(int side);

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

    // type III commands
    int WriteTrackCommand(uint8_t cmd);
    int ReadAddrCommand_1793(uint8_t cmd);

    // type IV commands
    int ForceIntCommand(uint8_t cmd);

    // undocumented commands
    int PercomCommand(uint8_t cmd);

    void UpdateInterrupt(bool interruptOn);

  protected:
    virtual CommandInfo * GetCommand(uint8_t cmd);
    virtual CommandInfo * GetCommand(uint8_t cmd, WD_FDC::CommandInfo * info, size_t count);

    void WriteCmdReg(int8_t command);
    uint8_t ReadStatusReg();
    uint8_t ReadDataReg();

    int SeekTrack(uint8_t cmd, uint8_t track, bool update);
    void RestartHeadLoadTimer();
    void LoadHead(bool load);
    void SetTypeIStatus();

    unsigned int m_mode = 0;

    bool m_debug;
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
    uint8_t m_readDAM;     // DAM for status register at the end of the read
    bool m_pulseIndex = false;

    // copies of registers
    uint8_t m_status;
    uint8_t m_track;
    bool m_side;
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
};

class WD_FD1771 : public WD_FDC
{
  public:
    WD_FD1771(bool debug)
      : WD_FDC(eWD1771, debug)
    {}
};

class WD_FD1791 : public WD_FDC
{
  public:
    WD_FD1791(bool debug)
      : WD_FDC(eWD1791, debug)
    {}
};

class WD_FD17x1 : public WD_FDC
{
  public:
    WD_FD17x1(bool debug)
      : WD_FDC(eWD1771 | eWD1791, debug)
    {}
};

class WD_FD1793 : public WD_FDC
{
  public:
    WD_FD1793(bool debug)
      : WD_FDC(eWD1793, debug)
    {}

    virtual CommandInfo * GetCommand(uint8_t cmd);
};

#endif // FDC_H_
