#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <SDL2/SDL_keyboard.h> 

#include "config.h"
#include "fdc.h"
#include "options.h"

#include <string>

#include "mainwindow.h"
#include "virtual_screen.h"

class Emulator
{
  public:
    Emulator();

    virtual std::string GetTitle() const = 0;

    virtual bool Open(const Options & options);
    virtual bool Poll();

    // CPU functions
    virtual double GetTargetClockSpeed_Hz() const;
    virtual double GetActualCPUSpeed_Hz() const;
    virtual uint16_t GetStartAddress() const;
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
    virtual int GetDefaultRAMSize_k() const = 0;
    virtual bool SetRAMSize_k(int len);  
    virtual int GetRAMSize_k() const;

    // ROM functions
    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    // Video functions
    virtual int GetVideoMemSize_k() = 0;
    virtual int GetScreenWidth() const = 0;
    virtual int GetScreenHeight() const = 0;
    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options);
    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);
    virtual void InitializeDG640();

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    std::vector<uint8_t> m_videoRAM;
    std::unique_ptr<VirtualScreen> m_video;

  protected:  
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    std::vector<uint8_t> m_romData;
    uint8_t * m_rom;
    int m_romSize;

    std::vector<uint8_t> m_fontData;
    std::unique_ptr<PixelFont> m_font;

    std::vector<uint8_t> m_ram;
    int m_ramSize_bytes;
    int m_ramMask;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    long long m_cycleCounter;
    std::chrono::system_clock::time_point m_cpuDelayTimer;
};


#endif // EMULATOR_H_