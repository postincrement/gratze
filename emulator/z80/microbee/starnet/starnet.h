#ifndef STARNET_H_
#define STARNET_H_



#include <stdint.h>

class Starnet
{
  public:
    enum class RequestType {
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
};

class StarnetDecoder : public Starnet
{
  public:
    StarnetDecoder();
    void Reset();
    void OnColdBoot(uint16_t memsize);
    void OnReceive(uint8_t v, bool ie);

  protected:
    uint8_t GetCRC() const;

    int m_len = 0;
    uint8_t m_crc;
    uint8_t m_buffer[136];
};

#endif // STARNET_H_

