#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <string>

#include <SDL_keyboard.h>

#include "common/factory.h"
#include "src/config.h"
#include "src/options.h"
#include "src/emuconfig.h"
#include "video/virtual_screen.h"
#include "devices/fdc.h"
#include "devices/keyboard.h"
#include "terminal/terminal.h"


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

    typedef std::function<void (double, uint64_t)> PollHandler;

    struct PollDef
    {
      PollDef(double interval, PollHandler handler)
        : m_timeInterval(interval)
        , m_handler(handler)
        , m_pollIsTime(true)
      { }

      PollDef(uint64_t interval, PollHandler handler)
        : m_clockInterval(interval)
        , m_handler(handler)
        , m_pollIsTime(false)
      { }

      PollDef(const PollDef & def) = default;

      void Execute(double secs, uint64_t clocks)
      {
        if (m_handler)
          m_handler(secs,clocks);
      }

      void SetHandler(PollHandler handler);

      double m_nextTime;
      double m_timeInterval;
      double m_lastTime;

      uint64_t m_nextClock;
      uint64_t m_clockInterval;
      uint64_t m_lastClock;

      bool m_pollIsTime;

      PollHandler m_handler = nullptr;
    };

    class PollDefList
    {
      public:
        typedef std::map<int, PollDef> ListType;
        int Add(double secs, PollHandler handler)
        {
          m_list.insert(ListType::value_type(m_index, PollDef(secs, handler)));
          return m_index++;
        }

        int Add(uint64_t clocks, PollHandler handler)
        {
          m_list.insert(ListType::value_type(m_index, PollDef(clocks, handler)));
          return m_index++;
        }

        void Remove(int index)
        {
          m_list.erase(index);
        }

        int m_index = 0;
        ListType m_list;
    };

    /////////////////////////////////////////////
    //
    //  main emulator functions
    //

    Emulator(const EmulatorInfo * info);
    virtual ~Emulator();

    virtual void Instantiate();

    int Run(const Options & options);

    virtual bool Open(const Options & options);
    virtual const EmulatorInfo & GetInfo() const;
    void UpdateScreen();
    void CheckKeyboard();

    // info functions
    virtual const Config::Block * GetConfigBlock(Config::Type type) const;
    virtual const Config::CPU * GetCPUInfo() const;
    virtual const Config::MemoryMappedScreen * GetMemoryMappedInfo() const;
    virtual const Config::RAM * GetMainRAMInfo() const;
    virtual int GetRAMSize_k() const;
    virtual bool SetRAMSize_k(int len);

    // CPU functions
    virtual double GetActualCPUSpeed_Hz() const;
    virtual bool Start(int addr = -1) = 0;
    virtual int Exec(int cycles) = 0;

    virtual void NMI() = 0;
    virtual void Interrupt(uint16_t vector = 0) = 0;
    virtual void Reset(int addr = -1);
    virtual unsigned GetPC() const = 0;

    virtual void DumpStack(int count);
    virtual void DumpStack(const std::vector<uint16_t> & stack);

    virtual void GetStack(std::vector<uint16_t> & stack) = 0;
    virtual void SetTrace(bool v) = 0;

    virtual int GetMemorySize() const = 0;
    virtual std::string GetName() const = 0;
    virtual std::string DumpRegs() const;

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
    virtual void WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);
    virtual uint8_t ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void ChangeVideoColour();

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    virtual void MemoryDump() const;

    void SetKeyboard(VirtualKeyboard * keyboard);

    int AddRealTimePollDef(double seconds, PollHandler handler);
    int AddCPUTimePollDef(uint64_t cycles, PollHandler handler);

    void CalcCPUSpeed(double secs, uint64_t clocks);

    void LoadGRZ();

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    virtual uint8_t ReadIOMemoryInternal(const ReadMemoryBlockInfo & info, uint16_t addr);
    virtual void WriteIOMemoryInternal(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data);

    const EmulatorInfo * m_info = nullptr;

    std::vector<ReadMemoryBlockInfo> m_readMemoryBlocks;
    std::vector<WriteMemoryBlockInfo> m_writeMemoryBlocks;

    std::vector<ReadIOPortBlockInfo> m_readIOPortBlocks;
    std::vector<WriteIOPortBlockInfo> m_writeIOPortBlocks;

    PollDefList m_pollers;

    uint64_t m_cycleCounter = 0;

    int m_ramSize_bytes = 0;
    int m_ramMask = 0;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    bool m_debugWriteMemory = false;
    bool m_debugReadMemory = false;
    bool m_keyboardDebug = false;
    bool m_turbo = false;

    unsigned m_verbose = 0;
    std::shared_ptr<VirtualKeyboard>    m_keyboard;
    std::shared_ptr<VirtualScreen>      m_screen;
    std::shared_ptr<MemoryMappedScreen> m_memMapScreen;
    std::shared_ptr<Terminal>           m_terminal;
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

  std::vector<Config::Block> m_blocks;
};

#endif // EMULATOR_H_
