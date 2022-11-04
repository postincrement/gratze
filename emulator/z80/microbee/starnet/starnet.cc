#include <iostream>
#include <map>

using namespace std;

#include "starnet.h"

#include <common/misc.h>

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
  m_len = 0;
  m_crc = 0;
}

void StarnetDecoder::OnColdBoot(uint16_t memsize)
{
  cerr << "starnet: cold boot " << HEXFORMAT0x4(memsize) << endl; 
}

uint8_t StarnetDecoder::GetCRC() const
{
  return ~m_crc + 1;
}

void StarnetDecoder::OnReceive(uint8_t v, bool ie)
{
  if (ie)
    return;

  cerr << "starnet: receive " << HEXFORMAT0x2(v) << endl;
  if (m_len < 7) {
    m_buffer[m_len] = v;
    m_crc += v;
    m_len++;
  }
  else if (m_len == 7) {
    Starnet::RequestType requestType = (Starnet::RequestType)m_buffer[0];
    uint16_t parm0 = m_buffer[1] + (m_buffer[2] << 8);
    uint16_t parm1 = m_buffer[3] + (m_buffer[4] << 8);
    uint16_t parm2 = m_buffer[5] + (m_buffer[6] << 8);
    switch ((Starnet::RequestType)m_buffer[0]) {
      case Starnet::RequestType::ColdBoot:
        if (v != GetCRC()) {
          cerr << "starnet: bad CRC - pkt " << HEXFORMAT0x2(v) << ", calc " << HEXFORMAT0x2(GetCRC()) << endl;
        }
        else {
          OnColdBoot(parm0);
        }
        break;
      default:  
        {
          if (g_RequestNames.count(requestType) != 0) {
            cerr << "starnet: request '" << g_RequestNames[requestType] << "' not yet implemented" << endl;
          }
          else {
            cerr << "starnet: request '" << (int)m_buffer[0] << "' not yet implemented" << endl;
          }
        }
        m_len = 0;
        break;
    }
  }
}


#if 0
netcrc:
    push bc			;edb8	c5 	. 
	  ld b,000h		;edb9	06 00 	. . 
	  ld a,000h		;edbb	3e 00 	> . 
ledbdh:
	  inc b			    ;edbd	04 	. 
	  dec hl			  ;edbe	2b 	+ 
	  add a,(hl)	  ;edbf	86 	. 
	  push hl			  ;edc0	e5 	. 
	  and a			    ;edc1	a7 	. 
	  sbc hl,de		  ;edc2	ed 52 	. R 
  	pop hl			  ;edc4	e1 	. 
	  jr nz,ledbdh	;edc5	20 f6 	  . 
	  neg		        ;edc7	ed 44 	. D 
	  push af			  ;edc9	f5 	. 
	  ld a,006h		  ;edca	3e 06 	> . 
	  cp b			    ;edcc	b8 	. 
	  ld a,08ch		  ;edcd	3e 8c 	> . 
	  jr nc,ledd7h  ;edcf	30 06 	0 . 
	  cp b			    ;edd1	b8 	. 
	  jr c,ledd7h	  ;edd2	38 03 	8 . 
	  pop af			  ;edd4	f1 	. 
	  pop bc			  ;edd5	c1 	. 
	  ret			      ;edd6	c9 	. 

ledd7h:
	  pop af			;edd7	f1 	. 
	  or 0ffh		;edd8	f6 ff 	. . 
	  pop bc			;edda	c1 	. 
	  ret

  #endif