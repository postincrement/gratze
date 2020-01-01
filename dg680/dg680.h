#ifndef DG680_H_
#define DG680_H_

#include "config.h"
#include "z80emulator.h"
#include "options.h"

#define   DG680_ROM_START_ADDR    0xd000
#define   DG680_ROM_END_ADDR      0xd7ff

#define   DG680_RAM_START_ADDR    0xd800
#define   DG680_RAM_END_ADDR      0xdfff

#define   DG640_VIDEO_START_ADDR  0xf000
#define   DG640_VIDEO_END_ADDR    0xf7ff

class DG680_Emulator : public Z80Emulator
{
  public:
    DG680_Emulator();

    virtual bool Open(const Options & options) override;

    virtual std::string GetTitle() const  override;

    virtual int GetDefaultRAMSize_k() const override;
    virtual uint16_t GetStartAddress() const override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;

    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;

    virtual double GetTargetClockSpeed_Hz() const override;

    virtual int GetVideoMemSize_k();

  protected:
    uint8_t m_dgosRAM[2048];  
};

#endif // DG680_H_