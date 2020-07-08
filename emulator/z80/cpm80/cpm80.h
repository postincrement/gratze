#ifndef CPM80_H_
#define CPM80_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"

#include <dirent.h>
#include <iostream>

class CPM80_Emulator;

struct NewBDOS
{
  NewBDOS(CPM80_Emulator & proc);
  ~NewBDOS();

  typedef void (NewBDOS::* Function)();

  void OnBDOSCommand();

  void SystemReset();    //  0 - System reset
  void ConsoleInput();   //  1 - Console input
  void ConsoleOutput();  //  2 - Console output
  void ConsoleDirect();  //  6 - Direct console access
  void PrintString();    //  9 - Print string
  void ReadLine();       // 10 - Read console buffer
  void ConsoleStatus();  // 11 - Get console status
  void ReturnVersion();  // 12 - Return Version
  void ResetDisk();      // 13 - Read console buffer
  void SelDisk();        // 14 - Select disk
  void OpenFile();       // 15 - Open File
  void SearchFirst();    // 17 - Search for first
  void SearchNext();     // 18 - Search for next
  void DeleteFile();     // 19 - Delete file
  void ReadSeq();        // 20 - Read Sequential
  void GetCurrDisk();    // 25 - Return current disk
  void SetDMAAddress();  // 26 - Set DMA address
  void GetSetUser();     // 32 - Set/get user code
  void ReadRandom();     // 33 - Read random

  int FindFile(uint8_t disk, const char * fcb);
  void Boot();
  void PrintCPMString(const char * str);

  CPM80_Emulator & m_proc;
  uint16_t m_dmaAddress;
  DIR * m_fileFind;
  uint16_t m_findFCB;
  uint8_t m_userCode;
  uint8_t m_currDisk;
  std::string m_currPath;
  uint8_t * m_memory;
};


class CPM80_Emulator : public Z80Emulator
{
  public:
    CPM80_Emulator();

    void Instantiate() override;

    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    // new functions
    bool Mount(const char * fn, int drive);
    bool DiskOp(int op);

    void OnKeyboard(uint8_t ch);

    void ConsoleOut(char data);
    bool ConsoleStatus();
    int ConsoleIn();

    friend class NewBDOS;

  protected:  
    std::unique_ptr<NewBDOS> m_newBDOS;
    uint8_t m_diskDrive;
    uint16_t m_track;
    uint16_t m_sector;
    uint16_t m_dmaAddress;
    uint16_t m_dmaLength;
    uint8_t m_status;
    uint16_t m_startAddress;
    int m_driveFiles[4];
    uint8_t * m_memory;
    std::deque<uint8_t> m_kbQueue;
};

#endif // CPM80_H_