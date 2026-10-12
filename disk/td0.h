
#ifndef VIRTUAL_DRIVE_TDO_H_
#define VIRTUAL_DRIVE_TDO_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "virtual_drive.h"

class VirtualDriveTD0 : public VirtualDriveFile
{
  public:
    VirtualDriveTD0();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
    virtual int ReadSector(int side, int track, int sector, SectorInfo & info, uint8_t * data, int len) override;

  protected:
    std::vector<uint8_t> m_imageData;
};

#endif // VIRTUAL_DRIVE_TDO_H_
