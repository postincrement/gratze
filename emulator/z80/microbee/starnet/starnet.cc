#include <iostream>
#include <fstream>
#include <map>

#ifndef __APPLE__
#include <endian.h>
#else
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif  /* __APPLE__ */

using namespace std;

#include <common/misc.h>

#include "starnet.h"

#define BOOT_IMAGE    "./z80/microbee/starnet/64kbcpm.slv"
#define BOOT_ADDRESS  0xc000

static std::map<Starnet::RequestType, const char *> g_RequestNames = {
  { Starnet::RequestType::ColdBoot,                  "ColdBoot" },
  { Starnet::RequestType::WarmBoot,                  "WarmBoot" },                  
  { Starnet::RequestType::ReadRecord,                "ReadRecord" },
  { Starnet::RequestType::WriteRecord,               "WriteRecord" },
  { Starnet::RequestType::LogIn,                     "LogIn" },
  { Starnet::RequestType::LogOut,                    "LogOut" },           
  { Starnet::RequestType::Print,                     "Print" },
  { Starnet::RequestType::ChangePassword,            "ChangePassword" },
  { Starnet::RequestType::RequestWritePerm,          "RequestWritePerm" },
  { Starnet::RequestType::RelinquishWritePerm,       "RelinquishWritePerm" },
  { Starnet::RequestType::GetDiskInfo,               "GetDiskInfo" },
  { Starnet::RequestType::GetPermissionData,         "GetPermissionData" },
  { Starnet::RequestType::GetWorkspaceInfo,          "GetWorkspaceInfo" },
  { Starnet::RequestType::GetConfigDataAndPrintSize, "GetConfigDataAndPrintSize" } ,
  { Starnet::RequestType::SendConfigData,            "SendConfigData" },
  { Starnet::RequestType::PrintSpoolBuffer,          "PrintSpoolBuffer" },
  { Starnet::RequestType::SendUserCommand,           "SendUserCommand" },
  { Starnet::RequestType::GetUserCommand,            "GetUserCommand" },
  { Starnet::RequestType::ClearPrintBuffer,          "ClearPrintBuffer" }          
};

StarnetDecoder::StarnetDecoder()
{
  Reset();
}

void StarnetDecoder::Reset()
{
  m_state = 0;
  m_rxLen = 0;
  m_rxCRC = 0;
  m_rxPtr = (uint8_t *)&m_request;
}

uint8_t StarnetDecoder::GetCRC() const
{
  return ~m_rxCRC + 1;
}

void StarnetDecoder::OnReceive(uint8_t v, bool ie)
{
  // ignore characters send while interrupts not enabled
  if (ie)
    return;

  cerr << "starnet: receive " << HEXFORMAT0x2(v) << endl;

  if (m_state >= 100) {
    cerr << "starnet: ignoring rx while sending" << endl;
    return;
  }

  switch (m_state) {
    case 0:
      Reset();
      m_state = 1;
      // fallthrough

    case 1:
      *m_rxPtr++ = v;
      m_rxCRC += v;
      if ((m_rxPtr - ((uint8_t *)&m_request)) >= sizeof(m_request))
        m_state = 2;
      break;

    case 2:  
      m_parm0 = le16toh(m_request.m_parm0);
      m_parm1 = le16toh(m_request.m_parm1);
      m_parm2 = le16toh(m_request.m_parm2);
      switch (m_request.m_code) {
        case Starnet::RequestType::ColdBoot:
          if (v != GetCRC()) {
            cerr << "starnet: bad CRC - pkt " << HEXFORMAT0x2(v) << ", calc " << HEXFORMAT0x2(GetCRC()) << endl;
            SendResponse(ResponseType::ChecksumErrorOnReceivedBlock);
          }
          else {
            OnColdBoot(m_parm0);
          }
          break;

        default:  
          if (g_RequestNames.count(m_request.m_code) != 0) {
            cerr << "starnet: request '" << g_RequestNames[m_request.m_code] << "' not yet implemented" << endl;
          }
          else {
            cerr << "starnet: request '" << (int)m_request.m_code << "' not yet implemented" << endl;
          }
          SendResponse(ResponseType::ChecksumErrorOnReceivedBlock);
          break;
    }
  }
}

uint8_t StarnetDecoder::OnSend()
{
  cerr << "starnet: request to send byte " << m_txLen << " in state " << m_state << endl;

  uint8_t val = 0;
  if (m_state < 100) {
    cerr << "starnet: ignoring tx while receiving" << endl;
  }
  else if (m_txLen >= m_txPayload.size()) {
    cerr << "starnet: ignoring tx past end of buffer" << endl;
  }
  else if (m_state < 200) {
    // boot packet
    val = m_txPayload[m_txLen++];
    if (m_txLen >= m_txPayload.size()) {
      SendNextBootPacket();
    }
  }
  else {
    // normal packet
    val = m_txPayload[m_txLen++];
    if (m_txLen >= m_txPayload.size()) {
      m_state = 0;
    }
  }

  cerr << "starnet: sending " << HEXFORMAT0x2(val) << endl;

  return val;
}

void StarnetDecoder::SendResponse( Starnet::ResponseType code,
                                                uint16_t parm0,
                                                uint16_t parm1,
                                                uint16_t parm2,
                                                uint8_t  * data, 
                                                uint16_t dataLen)
{
  m_state = 200;
  SendRawResponse(code, parm0, parm1, parm2, data, dataLen);
}

void StarnetDecoder::SendRawResponse( Starnet::ResponseType code,
                                                uint16_t parm0,
                                                uint16_t parm1,
                                                uint16_t parm2,
                                                uint8_t  * data, 
                                                uint16_t dataLen)
{
  size_t len = 7 + dataLen + 1;
  m_txPayload.resize(len);

  m_txPayload[0] = (uint8_t)code;
  m_txPayload[1] = parm0 & 0xff;
  m_txPayload[2] = (parm0 >> 8) & 0xff;
  m_txPayload[3] = parm1 & 0xff;
  m_txPayload[4] = (parm1 >> 8) & 0xff;
  m_txPayload[5] = parm2 & 0xff;
  m_txPayload[6] = (parm2 >> 8) & 0xff;

  uint8_t crc = 0;
  int i;
  for (i = 0; i < len-1; ++i)
    crc += m_txPayload[i];

  m_txPayload[i] = crc;
  m_txLen = 0;

  cerr << "starnet: prepared " << len << "  bytes to send" << endl;
}


void StarnetDecoder::OnColdBoot(uint16_t memsize)
{
  cerr << "starnet: cold boot " << HEXFORMAT0x4(memsize) << endl; 
  if (m_coldBootImage.size() == 0) {
    std::ifstream file;
    file.open(BOOT_IMAGE, ios::binary);
    file.seekg(0, std::ios::end);
    size_t length = file.tellg();
    file.seekg(0, std::ios::beg);
    m_coldBootImage.resize(length);
    if (!file.read(&m_coldBootImage[0], length)) {
      cerr << "starnet: could not read '" << BOOT_IMAGE << "'" << endl;
      SendResponse(ResponseType::RequestOutOfRange);
    }
    m_coldBootCount = (length + 127) / 128;
  }
  cerr << "starnet: cold boot needs " << m_coldBootCount << " packets" << endl;
  m_state = 100;
  SendNextBootPacket();
}

void StarnetDecoder::SendNextBootPacket()
{
  cerr << "starnet: cold boot packet in state " << m_state << endl;

  int index = m_state - 100;

  if (index == m_coldBootCount) {
    SendResponse(ResponseType::Acknowledge, 
                  BOOT_ADDRESS, 
                  0, 
                  0);
  } 
  else {
    cout << "starnet: sending boot packet " << index << " for state " << m_state << endl;
    uint16_t offset  = index * 128;
    uint16_t address = BOOT_ADDRESS + offset;

    uint16_t len = m_coldBootImage.size() - offset;
    if (len > 128)
      len = 128;

    SendRawResponse(ResponseType::Acknowledge, 
                    address, 
                    1, 
                    0, 
                    (uint8_t *)&m_coldBootImage[offset], 
                    len);

  }
}

