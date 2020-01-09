#ifndef EMUCONFIG_H_
#define EMUCONFIG_H_

#include "video/chargenrom.h"

class MainWindow;
class VirtualScreen;
class Font;

#define MAX_INFO_BLOCKS   40

////////////////////////////////////////////////////////////////////////////////////////////

#define INFO_FONT(count,width,height,data, creator)  { count, width, height, data, creator }

#define INFO_END()                          { Config::Type::eEnd }    
#define INFO_CPU(speed, addr)               { Config::Type::eCPU,         { .m_cpu={ speed, addr } } }
#define INFO_ROM(addr, data)                { Config::Type::eROM,         { .m_rom={ addr, addr + sizeof(data) - 1, data } } }
#define INFO_MAIN_RAM(start, k, min, max)   { Config::Type::eMainRAM,     { .m_ram={ start, start + k*1024 - 1, min, max } } }
#define INFO_RAM(start, end)                { Config::Type::eRAM,         { .m_ram={ start, end } } }
#define INFO_MEM_IO_READ(start, end, id)    { Config::Type::eMemIORead,   { .m_memIO={ start, end, id } } }
#define INFO_MEM_IO_WRITE(start, end, id)   { Config::Type::eMemIOWrite,  { .m_memIO={ start, end, id } } }
#define INFO_IO_PORT_READ(start, end, id)   { Config::Type::eIOPortRead,  { .m_ioPort={ start, end, id } } }
#define INFO_IO_PORT_WRITE(start, end, id)  { Config::Type::eIOPortWrite, { .m_ioPort={ start, end, id } } }
#define INFO_IO_PORT_RW(start, end, id)     { Config::Type::eIOPortRW,    { .m_ioPort={ start, end, id } } }
#define INFO_MONITOR(freq, h, v, fmt)       { Config::Type::eMonitor,     { .m_monitor={ freq, h, v, Config::VideoStandard::fmt, 6.7, 5.0 } } }

#define INFO_VIDEO_MEMORY_MAPPED(name, start, end, cols, rows, fontWid, fontHgt, count, fontData, fontCreator) \
    { Config::Type::eVideo, { .m_video={ \
      start, end, name, \
      cols, rows, cols*fontWid, rows*fontHgt, \
      INFO_FONT(count, fontWid, fontHgt, fontData, fontCreator) \
    } } }

#define INFO_VIDEO_EXTERNAL(name, size, cols, rows, fontWid, fontHgt, count, fontData, fontCreator) \
    { Config::Type::eVideoExternal, { .m_video={ \
      0, size, name, \
      cols, rows, cols*fontWid, rows*fontHgt, \
      INFO_FONT(count, fontWid, fontHgt, fontData, fontCreator) \
    } } }

class EmulatorInfo;

namespace Config {

enum class Type
{
  eEnd,
  eCPU,
  eMainRAM,
  eRAM,
  eROM,
  eMemIORead,
  eMemIOWrite,
  eIOPortRead,
  eIOPortWrite,
  eIOPortRW,
  eVideo,
  eVideoExternal,
  eMonitor
};

struct Font
{
  int  m_count;                       // number of chars in font
  int  m_width;                       // nominal font width in pixels (X)
  int  m_height;                      // nominal font height in pixels (X)
  CharacterGeneratorROM * m_charGen;  // character generator for base data (if required)
  bool (* m_creator)(const Options & options, const Font & fontInfo, std::vector<uint8_t> & fontData);  // function to create final font data (if required)
};

struct Video 
{
  uint16_t  m_startAddr;
  uint16_t  m_endAddr;

  const char * m_name;        // key into VirtualScreen factory

  int       m_screenCols;     // screen char cols (X)
  int       m_screenRows;     // screen char rows (Y)

  int       m_screenWidth;    // screen width in pixels (X)
  int       m_screenHeight;   // screen height in pixels (Y)

  Font  m_font;
};

enum class VideoStandard
{
  eNone,
  ePAL,
  eNTSC
};

struct Monitor 
{
  double m_pixelFrequency_MHz;
  double m_hScale;
  double m_vScale;
  VideoStandard m_std;
  double m_hOverScan_percent = 6.7;   // title safe
  double m_vOverScan_percent = 5.0;   // title safe
};

struct CPU
{
  double    m_clockSpeed_MHz;  // nominal CPU clock speed
  uint16_t  m_resetAddr;          // address to start when reset
};

struct ROM
{
  uint16_t  m_startAddr;
  uint16_t  m_endAddr;
  uint8_t * m_data;
};

struct RAM
{
  uint16_t m_startAddr;
  uint16_t m_endAddr;
  int m_minSize_K;              // min RAM size, in k
  int m_maxSize_K;              // max RAM size, in k
};

struct MemIO
{
  uint16_t  m_startAddr;
  uint16_t  m_endAddr;
  uint16_t  m_id;
};

struct IOPort
{
  uint16_t  m_startPort;
  uint16_t  m_endPort;
  uint16_t  m_id;
};

/////////////////////////////////////////////
//
//  master config structure
//

struct Block {
  Type m_type;

  union {
    CPU     m_cpu;
    ROM     m_rom;
    RAM     m_ram;
    MemIO   m_memIO;
    IOPort  m_ioPort;
    Video   m_video;
    Monitor m_monitor;
  } m_info;
};

} // namespace Config

#endif // EMUCONFIG_H_