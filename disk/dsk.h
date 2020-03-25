
#ifndef VIRTUAL_DRIVE_DSK_H_
#define VIRTUAL_DRIVE_DSK_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "virtual_drive.h"

class VirtualDriveDSK : public VirtualDriveFile
{
  public:
    VirtualDriveDSK();
    virtual bool OpenFile(int fd, off_t len, const uint8_t * header, size_t headerSize) override;
};

#endif // VIRTUAL_DRIVE_DSK_H_