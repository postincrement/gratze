#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <string>

#include <SDL_keyboard.h> 

#include "video/virtual_screen.h"
#include "config.h"
#include "magmedia/fdc.h"
#include "options.h"

class MainWindow;
class VirtualScreen;
class Font;

#define MAX_INFO_BLOCKS   20

////////////////////////////////////////////////////////////////////////////////////////////

class EmulatorInfo;

class Emulator
{
  public:
    enum class BlockType
    {
      eEnd,
      eCPU,
      eMainRAM,
      eRAM,
      eROM,
      eMemIORead,
      eMemIOWrite,
      eVideo
    };

    struct WriteMemoryBlockInfo;
    struct ReadMemoryBlockInfo;

    struct WriteIOPortBlockInfo;
    struct ReadIOPortBlockInfo;

    typedef void (Emulator:: * MemoryWriteFunction)(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);   
    typedef uint8_t (Emulator:: * MemoryReadFunction)(const ReadMemoryBlockInfo & info, uint16_t addr);   

    typedef void (Emulator:: * IOPortWriteFunction)(const WriteIOPortBlockInfo & info, uint16_t addr, uint8_t data);   
    typedef uint8_t (Emulator:: * IOPortReadFunction)(const WriteIOPortBlockInfo & info, uint16_t addr);   
    
#define INFO_END() { Emulator::BlockType::eEnd }    

    /////////////////////////////////////////////
    //
    //  CPU info
    //

    struct CPUInfo 
    {
      double    m_clockSpeed_MHz;  // nominal CPU clock speed
      uint16_t  m_resetAddr;          // address to start when reset
    };
  #define INFO_CPU(speed, addr) { Emulator::BlockType::eCPU, { .m_cpu={ speed, addr } } }

    /////////////////////////////////////////////
    //
    //  ROM block
    //
    struct ROMInfo 
    {
      uint16_t  m_startAddr;
      uint16_t  m_endAddr;
      uint8_t * m_data;
    };

  #define INFO_ROM(addr, data) { Emulator::BlockType::eROM, { .m_rom={ addr, addr + sizeof(data) - 1, data } } }

    /////////////////////////////////////////////
    //
    //  RAM block
    //
    struct RAMInfo 
    {
      uint16_t m_startAddr;
      uint16_t m_endAddr;
      int m_minSize_K;              // min RAM size, in k
      int m_maxSize_K;              // max RAM size, in k
    };

  #define INFO_MAIN_RAM(start, k, min, max) { Emulator::BlockType::eMainRAM, { .m_ram={ start, start + k*1024 - 1, min, max } } }
  #define INFO_RAM(start, end)              { Emulator::BlockType::eRAM,     { .m_ram={ start, end } } }

    /////////////////////////////////////////////
    //
    //  Memory IO block
    //
    struct MemIOInfo 
    {
      uint16_t  m_startAddr;
      uint16_t  m_endAddr;
      uint16_t  m_id;
    };

  #define INFO_MEM_IO_READ(start, end, id)  { Emulator::BlockType::eMemIORead,  { .m_memIO={ start, end, id } } }
  #define INFO_MEM_IO_WRITE(start, end, id) { Emulator::BlockType::eMemIOWrite, { .m_memIO={ start, end, id } } }

    /////////////////////////////////////////////
    //
    //  IOPort block
    //
    struct IOPortInfo 
    {
      uint16_t  m_startPort;
      uint16_t  m_endPort;
      uint16_t  m_id;
    };

  #define INFO_IO_PORT_READ(start, end, id)  { Emulator::BlockType::eIOPortRead,  { .m_ioPort={ start, end, id } } }
  #define INFO_IO_PORT_WRITE(start, end, id) { Emulator::BlockType::eIOPortWrite, { .m_ioPort={ start, end, id } } }

    /////////////////////////////////////////////
    //
    //  Video and font information block
    //

    enum class VideoDriver
    {
      eNone,
      eMemoryMapped
    };

    struct FontInfo
    {
      int  m_count;                       // number of chars in font
      int  m_width;                   // nominal font width in pixels (X)
      int  m_height;                  // nominal font height in pixels (X)
      uint8_t * m_fontData;               // base font data
      void (* m_creator)(const Options & options, const Emulator::FontInfo & fontInfo, std::vector<uint8_t> & fontData);
    };

    struct VideoDriverInfo 
    {
      uint16_t  m_startAddr;
      uint16_t  m_endAddr;

      VideoDriver m_type;

      int       m_screenCols;     // screen char cols (X)
      int       m_screenRows;     // screen char rows (Y)

      int       m_screenWidth;    // screen width in pixels (X)
      int       m_screenHeight;   // screen height in pixels (Y)

      FontInfo  m_font;
    };

  #define INFO_FONT(count,width,height,data, creator)  { count, width, height, data, creator }
  #define INFO_VIDEO_NONE() { Emulator::VideoDriver::eNone } //, 0, 0, 0, 0, 0, 0, { 0, 0, 0, NULL, NULL } }

  #define INFO_VIDEO_MEMORY_MAPPED(start, end, cols, rows, fontWid, fontHgt, count, fontData, fontCreator) \
    { Emulator::BlockType::eVideo, { .m_video={ \
      start, end, \
      Emulator::VideoDriver::eMemoryMapped, cols, rows, cols*fontWid, rows*fontHgt, \
      INFO_FONT(count, fontWid, fontHgt, fontData, fontCreator) \
    } } }

    /////////////////////////////////////////////
    //
    //  master config structure
    //

    struct InfoBlock {
      BlockType m_type;

      union {
        CPUInfo    m_cpu;
        ROMInfo    m_rom;
        RAMInfo    m_ram;
        MemIOInfo  m_memIO;
        IOPortInfo m_ioPort;
        VideoDriverInfo m_video;
      } m_info;
    };


    /////////////////////////////////////////////
    //
    //  read and write memory and IO records
    //

    struct WriteMemoryBlockInfo 
    {
      BlockType m_type = BlockType::eEnd;
      uint16_t m_startAddr = 0;
      uint16_t m_endAddr = 0;
      uint8_t * m_memory = nullptr; 
      std::vector<uint8_t> m_storage;
      MemoryWriteFunction m_function = nullptr;
      int m_id;
    };

    struct ReadMemoryBlockInfo 
    {
      BlockType m_type = BlockType::eEnd;
      uint16_t m_startAddr = 0;
      uint16_t m_endAddr = 0;
      const uint8_t * m_memory = nullptr;    // may point to write memory 
      std::vector<uint8_t> m_storage;
      MemoryReadFunction m_function = nullptr;
      int m_id;
    };

    struct WriteIOPortBlockInfo 
    {
      BlockType m_type = BlockType::eEnd;
      uint16_t m_startPort = 0;
      uint16_t m_endPort = 0;
      IOPortWriteFunction m_function = nullptr;
      int m_id;
    };

    struct ReadIOPortBlockInfo 
    {
      BlockType m_type = BlockType::eEnd;
      uint16_t m_startPort = 0;
      uint16_t m_endPort = 0;
      IOPortReadFunction m_function = nullptr;
      int m_id;
    };

    /////////////////////////////////////////////
    //
    //  main emulator functions
    //

    Emulator(EmulatorInfo * info);

    virtual const EmulatorInfo & GetInfo() const;

    virtual bool Open(const Options & options);
    virtual bool Poll();

    // info functions
    virtual const InfoBlock * GetInfoBlock(BlockType type) const;
    virtual const CPUInfo * GetCPUInfo() const;
    virtual const VideoDriverInfo * GetVideoInfo() const;
    virtual const RAMInfo * GetMainRAMInfo() const;
    virtual int GetRAMSize_k() const;
    virtual bool SetRAMSize_k(int len);  

    // CPU functions
    virtual double GetActualCPUSpeed_Hz() const;
    virtual bool Start(int addr = -1) = 0;
    virtual bool Run(int cycles = 1000) = 0;

    virtual void NMI() = 0;
    virtual void Interrupt(uint16_t vector = 0) = 0;
    virtual void Reset(uint16_t addr = 0) = 0;

    virtual void DumpStack(int count);
    virtual void DumpStack(const std::vector<uint16_t> & stack);

    virtual void GetStack(std::vector<uint16_t> & stack) = 0;
    virtual void SetTrace(bool v) = 0;

    // memory functions
    void CompileMemoryBlocks();

    virtual uint8_t ReadMemory(uint16_t);
    virtual void WriteMemory(uint16_t, uint8_t data);

    virtual uint8_t ReadIOMemory(int id, uint16_t);
    virtual void WriteIOMemory(int id, uint16_t, uint8_t data);

    virtual uint8_t DebugReadMemory(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void DebugWriteMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);

    
    virtual uint8_t DebugReadIOMemory(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void DebugWriteIOMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);

    virtual uint8_t ReadNull(uint16_t);
    virtual void WriteNull(uint16_t, uint8_t);

    virtual uint8_t ReadLog(uint16_t);
    virtual void WriteLog(uint16_t, uint8_t);

    virtual uint16_t ReadMemoryWord(uint16_t addr) = 0;

    // IO port functions
    virtual void CompileIOPortBlocks();

    virtual void WritePort(register uint16_t port, register uint8_t value);
    virtual uint8_t ReadPort(register uint16_t port);

    // keyboard functions
    virtual void OnKeyDown(const SDL_Keysym & keysym);
    virtual void OnKeyUp(const SDL_Keysym & keysym);

    // ROM functions
    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    // Video functions
    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options);
    virtual void CreateScreen(MainWindow & mainWindow, const Options & options);
    virtual void WriteToMemoryMappedVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);
    virtual uint8_t ReadFromMemoryMappedVideo(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);
    virtual int GetVideoOffset();
    virtual void ChangeVideoColour();

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    std::vector<uint8_t> m_videoRAM;
    std::unique_ptr<VirtualScreen> m_video;

  protected:  
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    virtual uint8_t ReadIOMemoryInternal(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void WriteIOMemoryInternal(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);

    const EmulatorInfo * m_info;

    std::vector<ReadMemoryBlockInfo> m_readMemoryBlocks;
    std::vector<WriteMemoryBlockInfo> m_writeMemoryBlocks;

    std::vector<ReadIOPortBlockInfo> m_readIOPortBlocks;
    std::vector<WriteIOPortBlockInfo> m_writeIOPortBlocks;

    std::vector<uint8_t> m_fontData;
    std::unique_ptr<Font> m_font;

    std::vector<uint8_t> m_ram;
    int m_ramSize_bytes;
    int m_ramMask;
    int m_videoOffset;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    long long m_cycleCounter;
    std::chrono::system_clock::time_point m_cpuDelayTimer;

    bool m_debugWriteMemory;
    bool m_debugReadMemory;
};

struct EmulatorInfo
{
  /////////////////////////////////////////////////
  //
  // devices initialise from here
  //

  const char * m_option;   // command line option
  const char * m_name;     // short name
  const char * m_title;    // long name

  Emulator::InfoBlock m_blocks[20];
};

#endif // EMULATOR_H_