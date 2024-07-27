
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
    
  protected:
    bool m_compressed;  
};

#endif // VIRTUAL_DRIVE_TDO_H_