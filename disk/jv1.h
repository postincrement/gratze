
#ifndef VIRTUAL_DRIVE_JV1_H_
#define VIRTUAL_DRIVE_JV1_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "virtual_drive.h"

class VirtualDriveJV1 : public VirtualDriveFile
{
  public:
    VirtualDriveJV1();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
};

#endif // VIRTUAL_DRIVE_JV2_H_