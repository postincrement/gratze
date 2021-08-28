#ifndef CPM80_NEWBDOS_H_
#define CPM80_NEWBDOS_H_

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <map>
#include <unistd.h>

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
  void CloseFile();       // 15 - Open File
  void SearchFirst();    // 17 - Search for first
  void SearchNext();     // 18 - Search for next
  void DeleteFile();     // 19 - Delete file
  void ReadSeq();        // 20 - Read Sequential
  void GetCurrDisk();    // 25 - Return current disk
  void SetDMAAddress();  // 26 - Set DMA address
  void GetSetUser();     // 32 - Set/get user code
  void ReadRandom();     // 33 - Read random

  void ReadFile(uint8_t * fcb, int code, off_t offs);
  uint8_t FindFile(int disk, const std::string & fn);
  void Boot();
  void PrintCPMString(const char * str);
  bool FCBToFilename(std::string & fn, const char * fcb);
  std::string FCBToRegex(const char * fcb);
  void UpdateDriveInfo(int drive);

  using StringMap = std::map<std::string, std::string>;

  struct DriveInfo {
    std::string m_dir;
    StringMap m_cpmToNative;
    StringMap m_nativeToCPM;
  };

  struct FileInfo {
    FileInfo();  
    ~FileInfo();  
    std::string m_fn;
    int m_fd = -1;
    off_t m_len = 0;
    off_t m_pos = 0;
    bool m_isText = false;
    std::vector<uint8_t> m_data;
  };

  CPM80_Emulator & m_proc;
  uint16_t m_dmaAddress;
  DIR * m_fileFind;
  uint16_t m_findFCB;
  uint8_t m_userCode;
  uint8_t m_currDisk;
  uint8_t * m_memory;
  int m_findIndex;
  std::ofstream m_debug;

  std::map<int, DriveInfo> m_driveInfoMap;
  std::map<int, FileInfo> m_fileMap;
};

#endif // CPM80_NEWBDOS_H_
