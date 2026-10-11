#ifndef CPMHOST_NEWBDOS_H_
#define CPMHOST_NEWBDOS_H_

#include <array>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <unistd.h>
#include <vector>

#include "cpm_drive.h"
#include "z80/cpmhost/cpmhost.h"

struct NewBDOS
{
  explicit NewBDOS(cpmhost::Host & host);
  ~NewBDOS();

  typedef void (NewBDOS::* Function)();

  // Entry from ED FE / PatchZ80. `code` is the value in C; values use A.
  void OnTrap(uint8_t code);
  void OnBDOSCommand(uint8_t code);
  void CcpCommand();
  void CcpExit();
  void CcpLcd();
  void CcpLcp();
  void CcpLpwd();
  void CcpLls();

  void SystemReset();    //  0 - System reset
  void ConsoleInput();   //  1 - Console input
  void ConsoleOutput();  //  2 - Console output
  void ConsoleDirect();  //  6 - Direct console access
  void PrintString();    //  9 - Print string
  void ReadLine();       // 10 - Read console buffer
  void ConsoleStatus();  // 11 - Get console status
  void ReturnVersion();  // 12 - Return Version
  void ResetDisk();      // 13 - Reset disk system
  void SelDisk();        // 14 - Select disk
  void OpenFile();       // 15 - Open File
  void CloseFile();      // 16 - Close file
  void SearchFirst();    // 17 - Search for first
  void SearchNext();     // 18 - Search for next
  void DeleteFile();     // 19 - Delete file
  void ReadSeq();        // 20 - Read Sequential
  void WriteSeq();       // 21 - Write sequential
  void MakeFile();       // 22 - Create file
  void GetCurrDisk();    // 25 - Return current disk
  void SetDMAAddress();  // 26 - Set DMA address
  void ReturnLoginVector();  // 24 - Return login vector
  void GetAllocVector();      // 27 - Get allocation vector
  void ReturnReadOnlyVector(); // 29 - Get read-only vector
  void GetDiskParams();  // 31 - Get disk parameter block
  void GetSetUser();     // 32 - Set/get user code
  void ReadRandom();     // 33 - Read random
  void WriteRandom();    // 34 - Write random
  void ComputeFileSize(); // 35 - Compute file size
  void SetRandomRecord(); // 36 - Set random record

  void ReadFile(uint8_t * fcb, int code, off_t offs);
  void WriteFile(uint8_t * fcb, int code, off_t offs);
  uint8_t FindFile(const uint8_t * fcb);
  void Boot();
  void PrintCPMString(const char * str);
  void PrintDriveMap();
  bool FCBToFilename(std::string & fn, const char * fcb);
  std::string FCBToRegex(const char * fcb);
  void UpdateDriveInfo(int drive);
  bool ConfigureDrives();
  void ClearHostCaches();
  int DriveFromFCB(const uint8_t * fcb) const;
  void PublishDPB(int drive);
  bool ReadLogical(int drive, uint32_t diskRec, uint8_t * dest);
  bool OpenImage(int drive, uint8_t * fcb);
  uint8_t SearchHost(int drive, const uint8_t * fcb, bool anyUser);
  uint8_t SearchImage(int drive, const uint8_t * fcb, bool anyUser);

  struct FileInfo {
    FileInfo();
    ~FileInfo();
    std::string m_fn;
    int m_fd = -1;
    off_t m_len = 0;
    off_t m_pos = 0;
    bool m_isText = false;
    bool m_isImage = false;
    int m_drive = 0;
    std::vector<uint8_t> m_data;
    // Logical record to on-disk record number. 0 is an unallocated hole.
    std::vector<uint32_t> m_diskRecords;
  };

  void BuildImageRecords(int drive, const uint8_t * entry, FileInfo & info);
  void ReadImageRecord(FileInfo & info, int code, off_t offs);

  cpmhost::Host & m_host;
  MFZ::Z80 & m_cpu;
  uint16_t m_dmaAddress;
  DIR * m_fileFind;
  uint16_t m_findFCB;
  uint8_t m_userCode;
  uint8_t m_currDisk;
  uint8_t * m_memory;
  int m_findIndex;
  int m_nextFileId = 0x40000000;
  uint16_t m_login = 0;
  uint16_t m_readOnly = 0;
  bool m_loadFileDone = false;
  std::ofstream m_debug;

  CpmDriveSet m_drives;
  std::map<int, FileInfo> m_fileMap;
};

#endif // CPMHOST_NEWBDOS_H_
