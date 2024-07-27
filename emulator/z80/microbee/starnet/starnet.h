#ifndef STARNET_H_
#define STARNET_H_



#include <stdint.h>

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

    #pragma pack (1)
    struct Header {
      Starnet::RequestType  m_code;
      uint16_t m_parm0;
      uint16_t m_parm1;
      uint16_t m_parm2;

      uint8_t GetCRC() const
      {
        uint8_t crc = 0;
        uint8_t * ptr = (uint8_t *)&m_code;
        for (int i = 0; i < sizeof(Header); ++i)
          crc += *ptr++;
        return crc;  
      }
    };
    #pragma pack()

    uint8_t CalcCRC(void * ptr_, int len) const
    {
      uint8_t crc = 0;
      uint8_t * ptr = (uint8_t *)ptr_;
      for (int i = 0; i < len; ++i)
        crc += *ptr++;
      return crc;  
    }
};

class StarnetDecoder : public Starnet
{
  public:
    StarnetDecoder();
    void Reset();
    void OnColdBoot(uint16_t memsize);
    void OnReceive(uint8_t v, bool ie);
    uint8_t OnSend();

    void SendRawResponse( Starnet::ResponseType code,
                                    uint16_t parm0 = 0,
                                    uint16_t parm1 = 0,
                                    uint16_t parm2 = 0,
                                    uint8_t  * data = nullptr,
                                    uint16_t len = 0);

    void SendResponse( Starnet::ResponseType code,
                                    uint16_t parm0 = 0,
                                    uint16_t parm1 = 0,
                                    uint16_t parm2 = 0,
                                    uint8_t  * data = nullptr,
                                    uint16_t len = 0);

    void SendNextBootPacket();

  protected:
    uint8_t GetCRC() const;

    int m_state = 0;
    size_t m_txLen = 0;
    int m_coldBootCount;

    int m_rxLen = 0;
    uint8_t * m_rxPtr;
    Header m_request;
    uint8_t m_rxPayload[128];
    uint8_t m_rxCRC;
    uint16_t m_parm0;
    uint16_t m_parm1;
    uint16_t m_parm2;

    std::vector<char> m_coldBootImage;
    std::vector<uint8_t> m_txPayload;
};

#endif // STARNET_H_

