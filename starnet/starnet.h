#ifndef STARNET_H_
#define STARNET_H_

#include <stddef.h>
#include <stdint.h>
#include <vector>

// Wire frame: one uint8_t code, three little-endian words, an optional
// payload, and one checksum byte. The BN sender stores the two's complement
// of the sum, and the receiver accepts a block whose bytes add to zero.

class Starnet
{
  public:
    enum class RequestType : uint8_t {
      ColdBoot                    = 1,
      WarmBoot                    = 2,
      ReadRecord                  = 3,
      WriteRecord                 = 4,
      LogIn                       = 5,
      LogOut                      = 6,
      Print                       = 7,
      ChangePassword              = 8,
      RequestWritePerm            = 9,
      RelinquishWritePerm         = 10,
      GetDiskInfo                 = 11,
      GetPermissionData           = 12,
      GetWorkspaceInfo            = 13,
      GetConfigDataAndPrintSize   = 14,
      SendConfigData              = 15,
      PrintSpoolBuffer            = 16,
      SendUserCommand             = 17,
      GetUserCommand              = 18,
      ClearPrintBuffer            = 19
    };

    enum class ResponseType : uint8_t {
      Acknowledge                      = 0x00,

      // User commands
      CannotSaveLoadUserCmd            = 0x90,

      // Password
      CantChangePasswordNotLoggedIn    = 0xa0,
      OldPasswordIncorrect             = 0xa1,
      WriteErrorChangingPassword       = 0xa2,

      // Login
      WorkspaceUnusable                = 0xb0,       
      PasswordIncorrect                = 0xb1,
      DualWorkSpacePermissionNotGiven  = 0xb2,    
      FileServerQuitting               = 0xb3,       
      WorkSpaceNonExistent             = 0xb4,

      PrinterNotConnected              = 0xc0,  
      Unused                           = 0xc1,       
      PrinterPermissionNotAvailable    = 0xc2,       
      BufferFullPleaseWait             = 0xc3,         

      AnotherStationHasWritePermission  = 0xd0,         
      NotPermittedToHaveWritePermission = 0xd1,         
      NotLoggedIn                       = 0xd2,       
      BadDriveSpecification             = 0xd4,         

      NonExistentReadWriteSpecified     = 0xe0,         
      BadSectorReadWriteSpecified       = 0xe1,       
      BadTrackReadWriteSpecified        = 0xe2,             
      PhysicalReadWriteError            = 0xe3,               
      NoWritePermissionAndPermissionNotGrantable  = 0xe4,
      NoWritePermissionAndPermissionGrantable     = 0xe5,   
      NoWritePermissionAndPermissionAnotherStationHasIt = 0xe6,   
      ReadBeyondEndOfWorkSpace          = 0xe7,
      ReadWriteToADriveNotLoggedIn      = 0xe8,  

      RequestOutOfRange                 = 0xfe,        
      ChecksumErrorOnReceivedBlock      = 0xff
    };

    static const int kHeaderBytes = 7;
    static const int kRecordBytes = 128;

    // Workstation slots on one server. 0xFF asks the UDP shim to assign one.
    // The session and the PIO wire never carry this.
    static const int kStationCount = 16;
    static const uint8_t kAssignStation = 0xff;

    // UDP only. The session never reads it.
    static const uint16_t kPort = 0xbee;
};

struct StarnetRequest {
  uint8_t m_code = 0;
  uint16_t m_parm0 = 0;
  uint16_t m_parm1 = 0;
  uint16_t m_parm2 = 0;
  std::vector<uint8_t> m_payload;
};

struct StarnetFrame {
  uint8_t m_code = 0;
  uint16_t m_parm0 = 0;
  uint16_t m_parm1 = 0;
  uint16_t m_parm2 = 0;
  std::vector<uint8_t> m_payload;
};

inline uint8_t StarnetSum(const uint8_t * data, size_t len)
{
  uint8_t sum = 0;
  for (size_t i = 0; i < len; ++i)
    sum = (uint8_t)(sum + data[i]);
  return sum;
}

inline uint8_t StarnetRxChecksum(const uint8_t * data, size_t len)
{
  return (uint8_t)(~StarnetSum(data, len) + 1);
}

#endif // STARNET_H_
