#ifndef CPM_FS_
#define CPM_FS_

#include <map>
#include <string>
#include <vector>
#include <stdint.h>

#include "virtual_drive.h"

class CPMFileSystem
{
  public:
    // CP/M disk parameter block
    struct DPB {
      const char * m_name;
      uint16_t m_spt;   // # of 128 byte record per track 
      uint8_t  m_bsh;   // block shift: 0x03 = 1k, 0x04 = 2k, 0x05 = 4k
      uint8_t  m_blm;   // block mask:  0x07 = 1k, 0x0f = 2k, 0x1f = 4k
      uint8_t  m_exm;   // extent mask ??
      uint16_t m_dsm;   // disk size - 1 
      uint16_t m_drm;   // # of directory entries - 1
      uint8_t  m_al0;   // directory allocation byte 0
      uint8_t  m_al1;   // directory allocation byte 1
      uint16_t m_cks;   // checksum vector size, 0 for fixed disk
                        // else # of directory entries / 4, rounded up
      uint16_t m_off;   // # of reserved tracks

      // additional parms
      bool m_isCylinder;    // true if in cylinder format
      uint8_t m_sec;        // sector size shift
      std::vector<uint16_t> m_skew; // sector skew table

      unsigned GetSectorSize() const { return 128 * (1 << m_sec); }
      unsigned GetBlockSize() const  { return 128 * (1 << m_bsh); }

      bool Use16BitBlockNumber() const { return m_dsm > 255; }
    };

    #pragma pack(1)
    struct DirectoryEntry {
      uint8_t m_uu;       // user
      uint8_t m_fn[8];    // filename
      uint8_t m_ext[3];   // extension
      uint8_t m_ex;       // extent counter low byte (0-31)
      uint8_t m_s1;       // extent counter reserved
      uint8_t m_s2;       // extent counter high byte
      uint8_t m_rc;       // record count
      uint8_t m_al[16];   // allocation bits

      bool IsDeleted() const;
      std::string GetFilename()  const;
      std::string GetBasename()  const;
      std::string GetExtension() const;
      unsigned GetUser() const;

      unsigned GetExtentCounter(const DPB & dpb) const;
      unsigned GetEntryNumber(const DPB & dpb) const;
      unsigned GetRecordCount(const DPB & dpb) const;

      unsigned GetBlockCount(const DPB & dpb);
      unsigned GetBlock(const DPB & dpb, int i);

      bool IsReadOnly() const;
      bool IsHidden() const;
    };
    #pragma pack()

    CPMFileSystem(std::shared_ptr<VirtualDrive> drive);

    bool Open();
    bool Read(const std::string & filename, std::vector<uint8_t> & data);

    struct FileInfo
    {
      uint8_t     m_user;
      std::string m_name;
      unsigned m_size = 0;
      bool m_readonly = false;
      bool m_hidden   = false;

      std::map<unsigned, DirectoryEntry> m_extents;
    };

    std::map<std::string, FileInfo> m_fileList;

  protected:  
    bool ReadDirectory();

    void AllocateFromMask(uint8_t mask, unsigned offs);

    bool ReadSector(int track, int sector, uint8_t * sectorBuffer);

    bool ReadBlock(unsigned block, uint8_t * blockBuffer);

    bool m_open = false;
    std::shared_ptr<VirtualDrive> m_drive;
    std::shared_ptr<DPB> m_dpb;

    std::vector<uint8_t> m_sectorBuffer;
    std::vector<uint8_t> m_blockBuffer;
    unsigned m_sectorSize;
    unsigned m_tracksPerSide;
    unsigned m_recordsPerSector;
    unsigned m_sectorsPerTrack;
    unsigned m_extentSize;
    unsigned m_blockSize;

    std::vector<bool> m_allocatedBlocks;

};

#endif // CPM_FS_