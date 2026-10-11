#ifndef CPM_DRIVE_H_
#define CPM_DRIVE_H_

#include <stdint.h>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "diskdef.h"

class VirtualDrive;

// One CP/M drive letter. A host directory is presented as a disk whose
// directory and DPB are built from the files in that directory. An image
// is a disk file interpreted with a JSON disk definition.
class CpmDrive
{
  public:
    enum class Kind { eHost, eImage };

    struct HostNames {
      std::string m_dir;
      std::map<std::string, std::string> m_cpmToNative;
      std::map<std::string, std::string> m_nativeToCPM;
    };

    CpmDrive();
    ~CpmDrive();

    CpmDrive(const CpmDrive &) = delete;
    CpmDrive & operator=(const CpmDrive &) = delete;
    CpmDrive(CpmDrive && other) noexcept;
    CpmDrive & operator=(CpmDrive && other) noexcept;

    void SetIndex(int index);

    // A configured host directory. The volume is built on Refresh.
    void MountHost(const std::string & path);

    // Open an image and read its directory. On failure the drive is left
    // unconfigured and error explains why.
    bool MountImage(const CpmDriveRequest & request, std::string & error, std::ostream * debug);

    // Point a host drive at a new directory and drop the cached volume.
    void SetHostRoot(const std::string & path);
    void ClearHostCache();

    // Rescan a host directory and rebuild its volume. An image drive
    // does nothing and returns true.
    bool Refresh(std::ostream * debug);

    // Cap the host allocation vector. Starnet slaves only reserve a few
    // hundred bytes of ALV; the default 16MB host volume overflows that
    // and the client's BDOS trashes its BIOS. Zero means no cap.
    void SetMaxBlocks(int maxBlocks) { m_maxBlocks = maxBlocks; }
    int MaxBlocks() const { return m_maxBlocks; }

    // Fifteen bytes: SPT, BSH, BLM, EXM, DSM, DRM, AL0, AL1, CKS, OFF.
    void Dpb(uint8_t bytes[15]) const;

    // One 128-byte record. Record 0 is the first record of track 0.
    bool ReadRecord(uint32_t record, uint8_t * dest);
    bool WriteRecord(uint32_t record, const uint8_t * src);

    void RebuildAllocation();

    const CpmDiskDef & Disk() const { return m_disk; }

    bool IsHost() const { return m_kind == Kind::eHost; }
    bool IsImage() const { return m_kind == Kind::eImage; }
    bool Configured() const { return m_configured; }

    Kind m_kind = Kind::eHost;
    bool m_configured = false;
    std::string m_path;
    std::string m_detail;
    HostNames m_host;
    CpmDiskDef m_disk;
    std::vector<uint8_t> m_directory;
    std::vector<uint8_t> m_alloc;

  private:
    struct HostBlock {
      std::string m_path;
      uint32_t m_offset = 0;
    };

    struct ListedFile {
      std::string m_name;
      std::string m_native;
      long long m_size = 0;
    };

    void ReleaseImage();
    void BuildHostVolume(const std::vector<ListedFile> & files, long long totalBytes, std::ostream * debug);
    bool ReadImage(uint32_t record, uint8_t * dest);
    bool WriteImage(uint32_t record, const uint8_t * src);
    bool LoadImageSector(uint32_t record, int & offset);
    bool ReadHost(uint32_t record, uint8_t * dest);
    bool WriteHost(uint32_t record, const uint8_t * src);
    bool HostRecord(uint32_t record, uint32_t & dataRec, int & block, int & within) const;

    int m_index = 0;
    int m_maxBlocks = 0;
    int m_fd = -1;
    std::shared_ptr<VirtualDrive> m_image;
    bool m_blockValid = false;
    int m_blockSide = -1;
    int m_blockCylinder = -1;
    int m_blockId = -1;
    std::vector<uint8_t> m_block;
    std::vector<HostBlock> m_blocks;
};

// The sixteen drive letters. Mount accepts the same spec strings as
// gratze --cpmdrive and refuses a letter that is listed twice.
class CpmDriveSet
{
  public:
    static const int kDrives = 16;

    CpmDriveSet();

    bool Mount(const std::vector<std::string> & specs, std::string & error, std::ostream * debug);

    CpmDrive & operator[](int drive);
    const CpmDrive & operator[](int drive) const;
    int size() const { return kDrives; }

    std::vector<CpmDrive>::iterator begin() { return m_drives.begin(); }
    std::vector<CpmDrive>::iterator end() { return m_drives.end(); }
    std::vector<CpmDrive>::const_iterator begin() const { return m_drives.begin(); }
    std::vector<CpmDrive>::const_iterator end() const { return m_drives.end(); }

  private:
    std::vector<CpmDrive> m_drives;
};

#endif // CPM_DRIVE_H_
