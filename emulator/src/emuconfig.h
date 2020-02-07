#ifndef EMUCONFIG_H_
#define EMUCONFIG_H_

#include "video/chargenrom.h"

class MainWindow;
class VirtualScreen;
class Font;

////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_TTF_FONT   "./fonts/UbuntuMono-R.ttf"

#define INFO_START(name)                    const struct Config::Block g_configInfo_##name[] = 
#define INFO_END(name)                      ; const size_t g_configInfo_##name##_size = (sizeof(g_configInfo_##name) / sizeof(g_configInfo_##name[0]));

#define INFO_INSERT(name)                   g_configInfo_##name, g_configInfo_##name##_size

#define INFO_CPU(speed, addr)               { Config::Type::eCPU,         { .m_cpu={ speed, addr } } }
#define INFO_ROM(addr, data)                { Config::Type::eROM,         { .m_rom={ addr, addr + sizeof(data) - 1, data } } }
#define INFO_MAIN_RAM(start, k, min, max)   { Config::Type::eMainRAM,     { .m_ram={ start, start + (k)*1024 - 1, min, max } } }
#define INFO_RAM(start, end)                { Config::Type::eRAM,         { .m_ram={ start, end } } }
#define INFO_MEM_IO_READ(start, end, id)    { Config::Type::eMemIORead,   { .m_memIO={ start, end, id } } }
#define INFO_MEM_IO_WRITE(start, end, id)   { Config::Type::eMemIOWrite,  { .m_memIO={ start, end, id } } }
#define INFO_IO_PORT_READ(start, end, id)   { Config::Type::eIOPortRead,  { .m_ioPort={ start, end, id } } }
#define INFO_IO_PORT_WRITE(start, end, id)  { Config::Type::eIOPortWrite, { .m_ioPort={ start, end, id } } }
#define INFO_IO_PORT_RW(start, end, id)     { Config::Type::eIOPortRW,    { .m_ioPort={ start, end, id } } }
#define INFO_MONITOR(freq, h, v, fmt)       { Config::Type::eMonitor,     { .m_monitor={ freq, h, v, Config::VideoStandard::fmt, 6.7, 5.0 } } }

#define INFO_FONT(count,width,height,data, creator)  { count, width, height, data, creator }

#define INFO_SCREEN_MEMORY_MAPPED(name, start, end, cols, rows, fontWid, fontHgt, count, fontData, fontCreator, var) \
    { Config::Type::eMemoryMappedScreen, { .m_memoryMappedScreen={ \
      start, end, name, \
      cols, rows, cols*fontWid, rows*fontHgt, \
      INFO_FONT(count, fontWid, fontHgt, fontData, fontCreator), var \
    } } }

#define INFO_SCREEN_MEMORY_MAPPED_FIXED(name, start, end, cols, rows, fontWid, fontHgt, count, fontData, fontCreator) \
  INFO_SCREEN_MEMORY_MAPPED(name, start, end, cols, rows, fontWid, fontHgt, count, fontData, fontCreator, false)

#define INFO_SCREEN_MEMORY_MAPPED_VARIABLE(name, size, cols, rows, fontWid, fontHgt, count, fontData, fontCreator) \
  INFO_SCREEN_MEMORY_MAPPED(name, 0, size, cols, rows, fontWid, fontHgt, count, fontData, fontCreator, true)

#define INFO_TERMINAL(cols, rows)   { Config::Type::eTerminal,    { .m_terminal={ cols, rows, DEFAULT_TTF_FONT } } }

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
  eMemoryMappedScreen,
  eTerminal,
  eMonitor
};

struct Font
{
  int  m_count;                       // number of chars in font
  int  m_width;                       // nominal font width in pixels (X)
  int  m_height;                      // nominal font height in pixels (X)
  CharacterGeneratorROM * m_charGen;  // character generator for base data (if required)
  bool (* m_creator)(const Font & fontInfo, std::vector<uint8_t> & fontData);  // function to create final font data (if required)
};

struct MemoryMappedScreen 
{
  uint16_t  m_startAddr;
  uint16_t  m_endAddr;

  const char * m_name;        // key into VirtualScreen factory

  int       m_screenCols;     // screen char cols (X)
  int       m_screenRows;     // screen char rows (Y)

  int       m_screenWidth;    // screen width in pixels (X)
  int       m_screenHeight;   // screen height in pixels (Y)

  Font      m_font;

  bool      m_variable;
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

struct Terminal
{
  int  m_cols;
  int  m_rows;
  const char * m_fontName;
};

/////////////////////////////////////////////
//
//  master config structure
//

struct Block {
  Type m_type;

  union {
    CPU                 m_cpu;
    ROM                 m_rom;
    RAM                 m_ram;
    MemIO               m_memIO;
    IOPort              m_ioPort;
    MemoryMappedScreen  m_memoryMappedScreen;
    Monitor             m_monitor;
    Terminal            m_terminal;
  } m_info;
};

} // namespace Config

#endif // EMUCONFIG_H_