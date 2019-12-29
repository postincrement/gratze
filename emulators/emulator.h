#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>
#include <SDL2/SDL_keyboard.h> 

extern "C" {
#include "mfz80/Z80.h"
};

#include "config.h"
#include "sdl_video.h"
#include "fdc.h"
#include "options.h"

#include <string>

class Emulator
{
  public:
    Emulator();

    virtual std::string GetTitle() const = 0;

    virtual bool Open(const Options & options);

    virtual bool Start(uint16_t addr = 0) = 0;
    virtual bool Run() = 0;
    virtual void Execute() = 0;

    virtual void NMI() = 0;
    virtual void Interrupt(uint16_t vector = 0) = 0;
    virtual void Reset(uint16_t addr = 0) = 0;

    virtual void Poll();

    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);
    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);

    virtual void OnKeyDown(SDL_Keysym & keysym);
    virtual void OnKeyUp(SDL_Keysym & keysym);

    virtual int GetDefaultRAMSize_k() const = 0;
    virtual bool SetRAMSize_k(int len);  
    virtual int GetRAMSize_k() const;

    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    virtual uint8_t ReadNull(uint16_t);
    virtual void WriteNull(uint16_t, uint8_t);

    virtual uint8_t ReadLog(uint16_t);
    virtual void WriteLog(uint16_t, uint8_t);

    virtual uint16_t ReadMemoryWord(uint16_t addr) = 0;

    virtual void DumpStack(int count);
    virtual void DumpStack(const std::vector<uint16_t> & stack);

    virtual void GetStack(std::vector<uint16_t> & stack) = 0;
    virtual void SetTrace(bool v) = 0;

  protected:  
    bool OpenVideo(int rows, int cols, MemoryMappedVideo::Font * font);
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) = 0;

    std::vector<uint8_t> m_ram;
    int m_ramSize;
    int m_ramMask;

    uint8_t m_drvSel;
    std::unique_ptr<MemoryMappedVideo> m_video;
    WD_FDC m_fdc;
};

class Z80Emulator : public Emulator
{
  public:
    Z80Emulator();

    static Z80Emulator * g_z80Instance;

    // overrides from Emulator
    virtual bool Start(uint16_t addr = 0) override;
    virtual bool Run()  override;
    virtual void Execute()  override;

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
};

#endif // EMULATOR_H_