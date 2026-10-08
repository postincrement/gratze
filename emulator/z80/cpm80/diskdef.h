#ifndef CPM80_DISKDEF_H_
#define CPM80_DISKDEF_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <sys/types.h>

class VirtualDrive;

// How the two heads of a double-sided disk are visited.
// single:   one head. Logical track N is cylinder N.
// cylinder: head 0 then head 1 of each cylinder (C0H0, C0H1, C1H0, C1H1).
// inout:    out along head 0, then back along head 1
//           (C0H0, C1H0, ... CnH0, CnH1, ... C1H1, C0H1).
enum class CpmSides
{
  eSingle,
  eCylinder,
  eInOut
};

// One CP/M 2.2 disk definition, loaded from a JSON file.
struct CpmDiskDef
{
  std::string m_name;
  std::string m_description;
  int m_spt = 0;
  int m_bsh = 0;
  int m_blm = 0;
  int m_exm = 0;
  int m_dsm = 0;
  int m_drm = 0;
  int m_al0 = 0;
  int m_al1 = 0;
  int m_cks = 0;
  int m_off = 0;
  CpmSides m_sides = CpmSides::eSingle;
  // Logical 128-byte record to a 0-based record in the track. Length is SPT.
  // Identity when the definition has neither skew nor translate.
  std::vector<int> m_translate;
  // 1-based physical sector ids from a JSON translate list. Empty unless the
  // definition has a translate table. A short table repeats when its length
  // divides the number of physical sectors on the track.
  std::vector<int> m_sectorIds;
};

// One --cpmdrive argument after it has been parsed.
struct CpmDriveRequest
{
  int m_drive = 0;
  bool m_image = false;
  std::string m_path;
  CpmDiskDef m_disk;
};

bool ParseCpmDriveRequest(const std::string & spec, CpmDriveRequest & out, std::string & error);
bool CpmImageFits(const CpmDiskDef & disk, off_t fileSize, std::string & error);
bool CheckCpmDrives(const std::vector<std::string> & specs, std::string & error);

// Open an image. A recognised disk-image format is returned in drive.
// A plain sector dump leaves drive empty and sets raw.
bool OpenCpmImage(const std::string & path, const CpmDiskDef & disk, std::shared_ptr<VirtualDrive> & drive, bool & raw, std::string & error);

// Sectors per track, logical tracks, and total capacity from the DPB.
bool CpmDiskGeometry(const CpmDiskDef & disk, int & sectorsPerTrack, int & tracks, int & capacityKb);

// Logical CP/M track to the track index in a cylinder-ordered image.
uint32_t CpmImageTrack(const CpmDiskDef & disk, uint32_t logicalTrack);

#endif // CPM80_DISKDEF_H_
