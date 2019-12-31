#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <SDL2/SDL_keyboard.h> 

extern "C" {
#include "mfz80/Z80.h"
};

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
    virtual void Poll();

    // CPU functions
    virtual double GetTargetClockSpeed_Hz() const;
    virtual double GetActualCPUSpeed_Hz() const;
    virtual bool Start(uint16_t addr = 0) = 0;
    virtual bool Run(int cycles = 1000) = 0;

    virtual void NMI() = 0;
    virtual void Interrupt(uint16_t vector = 0) = 0;
    virtual void Reset(uint16_t addr = 0) = 0;

    virtual void DumpStack(int count);
    virtual void DumpStack(const std::vector<uint16_t> & stack);

    virtual void GetStack(std::vector<uint16_t> & stack) = 0;
    virtual void SetTrace(bool v) = 0;

    // memory functions
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

    // Floppy/hard drive functions
    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

  protected:  
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    std::vector<uint8_t> m_ram;
    int m_ramSize;
    int m_ramMask;

    uint8_t m_drvSel;
    WD_FDC m_fdc;

    std::unique_ptr<VirtualScreen> m_video;

    double m_targetCPUClock_Hz;
    double m_actualCPUClock_Hz;

    long long m_cycleCounter;
    std::chrono::system_clock::time_point m_cpuDelayTimer;
};

class Z80Emulator : public Emulator
{
  public:
    Z80Emulator();

    static Z80Emulator * g_z80Instance;

    // overrides from Emulator
    virtual bool Start(uint16_t addr = 0) override;
    virtual bool Run(int cycles = 1000)  override;

    virtual uint8_t RdZ80(register uint16_t Addr);
    virtual void WrZ80(register uint16_t Addr,register uint8_t Value);

    virtual uint8_t InZ80(register uint16_t Port);
    virtual void OutZ80(register uint16_t Port, register uint8_t Value);

    virtual void NMI() override;
    virtual void Interrupt(uint16_t vector = 0) override;
    virtual void Reset(uint16_t addr = 0) override;
    virtual uint16_t ReadMemoryWord(uint16_t addr) override;

    virtual void GetStack(std::vector<uint16_t> & stack) override;
    virtual void SetTrace(bool v) override;

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) override;
    Z80 m_cpu;
    int m_cpuDelayRepeat;
    uint8_t m_delayBuffer[32];
};

#endif // EMULATOR_H_