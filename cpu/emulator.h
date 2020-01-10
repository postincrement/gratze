#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <string>

#include <SDL_keyboard.h> 

#include "video/virtual_screen.h"
#include "factory.h"
#include "config.h"
#include "magmedia/fdc.h"
#include "options.h"
#include "emuconfig.h"

class Emulator
{
  public:
    struct WriteMemoryBlockInfo;
    struct ReadMemoryBlockInfo;

    struct WriteIOPortBlockInfo;
    struct ReadIOPortBlockInfo;

    typedef void (Emulator:: * MemoryWriteFunction)(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);   
    typedef uint8_t (Emulator:: * MemoryReadFunction)(const ReadMemoryBlockInfo & info, uint16_t addr);   

    typedef void (Emulator:: * IOPortWriteFunction)(const WriteIOPortBlockInfo & info, uint16_t addr, uint8_t data);   
    typedef uint8_t (Emulator:: * IOPortReadFunction)(const ReadIOPortBlockInfo & info, uint16_t addr);   


    /////////////////////////////////////////////
    //
    //  read and write memory and IO records
    //

    struct WriteMemoryBlockInfo 
    {
      WriteMemoryBlockInfo() = default;
      WriteMemoryBlockInfo(const WriteMemoryBlockInfo & obj) = default;
      Config::Type m_type = Config::Type::eEnd;
      uint16_t m_startAddr = 0;
      uint16_t m_endAddr = 0;
      uint8_t * m_memory = nullptr; 
      MemoryWriteFunction m_realFunction = nullptr;
      MemoryWriteFunction m_function = nullptr;
      int m_id;
    };

    struct ReadMemoryBlockInfo 
    {
      ReadMemoryBlockInfo() = default;
      ReadMemoryBlockInfo(const ReadMemoryBlockInfo & obj) = default;
      Config::Type m_type = Config::Type::eEnd;
      uint16_t m_startAddr = 0;
      uint16_t m_endAddr = 0;
      const uint8_t * m_memory = nullptr;    // may point to write memory 
      MemoryReadFunction m_realFunction = nullptr;
      MemoryReadFunction m_function = nullptr;
      int m_id;
    };

    struct WriteIOPortBlockInfo 
    {
      WriteIOPortBlockInfo() = default;
      WriteIOPortBlockInfo(const WriteIOPortBlockInfo & obj) = default;
      Config::Type m_type = Config::Type::eEnd;
      uint16_t m_startPort = 0;
      uint16_t m_endPort = 0;
      IOPortWriteFunction m_function = nullptr;
      int m_id;
    };

    struct ReadIOPortBlockInfo 
    {
      ReadIOPortBlockInfo() = default;
      ReadIOPortBlockInfo(const ReadIOPortBlockInfo & obj) = default;
      Config::Type m_type = Config::Type::eEnd;
      uint16_t m_startPort = 0;
      uint16_t m_endPort = 0;
      IOPortReadFunction m_function = nullptr;
      int m_id;
    };

    /////////////////////////////////////////////
    //
    //  main emulator functions
    //

    Emulator(EmulatorInfo * info = nullptr);

    virtual void Init();

    virtual const EmulatorInfo & GetInfo() const;

    virtual bool Open(const Options & options);
    virtual bool Poll();

    // info functions
    virtual const Config::Block * GetConfigBlock(Config::Type type) const;
    virtual const Config::CPU * GetCPUInfo() const;
    virtual const Config::Video * GetVideoInfo() const;
    virtual const Config::RAM * GetMainRAMInfo() const;
    virtual int GetRAMSize_k() const;
    virtual bool SetRAMSize_k(int len);  

    // CPU functions
    virtual double GetActualCPUSpeed_Hz() const;
    virtual bool Start(int addr = -1) = 0;
    virtual bool Run(int cycles = 1000) = 0;

    virtual void NMI() = 0;
    virtual void Interrupt(uint16_t vector = 0) = 0;
    virtual void Reset(int addr = -1) = 0;

    virtual void DumpStack(int count);
    virtual void DumpStack(const std::vector<uint16_t> & stack);

    virtual void GetStack(std::vector<uint16_t> & stack) = 0;
    virtual void SetTrace(bool v) = 0;

    void CompileConfigBlocks();

    // memory functions
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
    virtual void WritePort(register uint16_t port, register uint8_t value);
    virtual uint8_t ReadPort(register uint16_t port);

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t);
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data);

    virtual uint8_t ReadIOPortLog(uint16_t addr);
    virtual void WriteIoPortLog(uint16_t addr, uint8_t val);

    // keyboard functions
    virtual void OnKeyDown(const SDL_Keysym & keysym);
    virtual void OnKeyUp(const SDL_Keysym & keysym);

    // ROM functions
    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    // Video functions
    void CreateScreen(MainWindow & mainWindow, const Options & options);
    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options);
    virtual void WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);
    virtual uint8_t ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void ChangeVideoColour();

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    virtual void MemoryDump() = 0;
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

    std::vector<uint8_t> m_ram;
    int m_ramSize_bytes;
    int m_ramMask;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    long long m_cycleCounter;
    std::chrono::system_clock::time_point m_cpuDelayTimer;

    bool m_debugWriteMemory = false;
    bool m_debugReadMemory = false;
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

  Config::Block m_blocks[20];
};

#endif // EMULATOR_H_