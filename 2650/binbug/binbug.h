#ifndef SYSTEM_2650_H
#define SYSTEM_2650_H

#include "2650emulator.h"

class BINBUG_2650 : public S2650Emulator
{
  public:
    BINBUG_2650();

    virtual std::string GetTitle() const  override;

    virtual int GetDefaultRAMSize_k() const override;
    
    virtual bool Open(const Options & options) override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;

    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;

    virtual double GetTargetClockSpeed_Hz() const override;

    virtual int GetVideoMemSize_k();

};

#endif // SYSTEM_2650_H



