#ifndef CPM80_NEWBDOS_H_
#define CPM80_NEWBDOS_H_

#include <array>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <unistd.h>
#include <vector>

#include "disk/virtual_drive.h"
#include "z80/cpm80/diskdef.h"

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
  void ResetDisk();      // 13 - Reset disk system
  void SelDisk();        // 14 - Select disk
  void OpenFile();       // 15 - Open File
  void CloseFile();      // 16 - Close file
  void SearchFirst();    // 17 - Search for first
  void SearchNext();     // 18 - Search for next
  void DeleteFile();     // 19 - Delete file
  void ReadSeq();        // 20 - Read Sequential
  void GetCurrDisk();    // 25 - Return current disk
  void SetDMAAddress();  // 26 - Set DMA address
  void GetDiskParams();  // 31 - Get disk parameter block
  void GetSetUser();     // 32 - Set/get user code
  void ReadRandom();     // 33 - Read random

  void ReadFile(uint8_t * fcb, int code, off_t offs);
  uint8_t FindFile(const uint8_t * fcb);
  void Boot();
  void PrintCPMString(const char * str);
  void PrintDriveMap();
  bool FCBToFilename(std::string & fn, const char * fcb);
  std::string FCBToRegex(const char * fcb);
  void UpdateDriveInfo(int drive);
  bool ConfigureDrives();
  bool OpenImageDrive(const CpmDriveRequest & request);
  void ClearHostCaches();
  int DriveFromFCB(const uint8_t * fcb) const;
  void PublishDPB(int drive);
  bool ReadLogical(int drive, uint32_t diskRec, uint8_t * dest);
  bool OpenImage(int drive, uint8_t * fcb);
  uint8_t SearchHost(int drive, const uint8_t * fcb, bool anyUser);
  uint8_t SearchImage(int drive, const uint8_t * fcb, bool anyUser);

  using StringMap = std::map<std::string, std::string>;

  struct DriveInfo {
    std::string m_dir;
    StringMap m_cpmToNative;
    StringMap m_nativeToCPM;
  };

  // One drive letter. An unconfigured letter stays a host directory:
  // A is ".", every other letter is "./" plus that letter.
  struct DriveSlot {
    enum class Kind { eHost, eImage };
    Kind m_kind = Kind::eHost;
    bool m_configured = false;
    std::string m_path;
    std::string m_detail;
    DriveInfo m_host;
    CpmDiskDef m_disk;
    int m_fd = -1;
    std::shared_ptr<VirtualDrive> m_image;
    std::vector<uint8_t> m_directory;
    // BIOS deblock buffer. One physical sector, reused by the 128-byte
    // records that share it.
    bool m_blockValid = false;
    int m_blockSide = -1;
    int m_blockCylinder = -1;
    int m_blockId = -1;
    std::vector<uint8_t> m_block;
  };

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

  CPM80_Emulator & m_proc;
  uint16_t m_dmaAddress;
  DIR * m_fileFind;
  uint16_t m_findFCB;
  uint8_t m_userCode;
  uint8_t m_currDisk;
  uint8_t * m_memory;
  int m_findIndex;
  int m_nextFileId = 0x40000000;
  std::ofstream m_debug;

  std::array<DriveSlot, 16> m_drives;
  std::map<int, FileInfo> m_fileMap;
};

#endif // CPM80_NEWBDOS_H_
