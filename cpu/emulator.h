#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <string>

#include <SDL2/SDL_keyboard.h> 

#include "video/virtual_screen.h"
#include "config.h"
#include "magmedia/fdc.h"
#include "options.h"

class MainWindow;
class VirtualScreen;
class Font;

struct EmulatorInfo
{
  enum class VideoDriver
  {
    eNone,
    eExplicit,
    eDG640
  };

  struct VideoDriverInfo 
  {
    VideoDriver m_type;

    int       m_memorySize_k;   // video memory size, in k
    int       m_screenCols;     // screen char cols (X)
    int       m_screenRows;     // screen char rows (Y)

    int       m_screenWidth;    // screen width in pixels (X)
    int       m_screenHeight;   // screen height in pixels (Y)

    int       m_fontWidth;      // nominal font width in pixels (X)
    int       m_fontHeight;     // nominal font height in pixels (X)
  };

  struct ROMInfo 
  {
    uint16_t  m_addr;
    int       m_size_k;
    uint8_t * m_data;
  };

  // devices initialise from here

  const char * m_option;   // command line option
  const char * m_name;     // short name
  const char * m_title;    // long name

  double    m_cpuCLockSpeed_MHz;         // nominal CPU clock speed
  uint16_t  m_resetAddr;                 // address to start when reset

  VideoDriverInfo m_video;

  ROMInfo m_rom;

  int       m_ramSize_K;                 // default RAM size, in k
  int       m_minRamSize_K;              // min RAM size, in k
  int       m_maxRamSize_K;              // max RAM size, in k
};

#define   DEFINE_ROM(addr, data) {  addr, sizeof(data), data }

#define   NO_ROM()               { 0x0000, 0, NULL }  

class Emulator
{
  public:
    Emulator(EmulatorInfo * info);

    virtual const EmulatorInfo & GetInfo() const;

    virtual bool Open(const Options & options);
    virtual bool Poll();

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
    virtual int GetRAMSize_k() const;

    virtual uint8_t ReadMemory(uint16_t) = 0;
    virtual void WriteMemory(uint16_t, uint8_t data) = 0;

    virtual uint8_t ReadNull(uint16_t);
    virtual void WriteNull(uint16_t, uint8_t);

    virtual uint8_t ReadLog(uint16_t);
    virtual void WriteLog(uint16_t, uint8_t);

    virtual uint16_t ReadMemoryWord(uint16_t addr) = 0;

    // keyboard functions
    virtual void OnKeyDown(SDL_Keysym & keysym);
    virtual void OnKeyUp(SDL_Keysym & keysym);

    // RAM functions
    virtual bool SetRAMSize_k(int len);  

    // ROM functions
    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    // Video functions
    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options);
    virtual void CreateScreen(MainWindow & mainWindow, const Options & options);
    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);
    virtual void InitializeDG640();

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    std::vector<uint8_t> m_videoRAM;
    std::unique_ptr<VirtualScreen> m_video;

  protected:  
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    const EmulatorInfo * m_info;

    uint8_t * m_rom;
    int m_romSize_bytes;

    std::vector<uint8_t> m_fontData;
    std::unique_ptr<Font> m_font;

    std::vector<uint8_t> m_ram;
    int m_ramSize_bytes;
    int m_ramMask;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    long long m_cycleCounter;
    std::chrono::system_clock::time_point m_cpuDelayTimer;
};

#endif // EMULATOR_H_