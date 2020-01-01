#ifndef SYSTEM_2650_H
#define SYSTEM_2650_H

#include "cpu/2650emulator.h"

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

    virtual int GetScreenSizePixels(int & x, int & y) const = 0;
    virtual int GetScreenSizeChars(int & x, int & y) const = 0;

    virtual double GetTargetClockSpeed_Hz() const override;

    virtual int GetVideoMemSize_k();

};

#endif // SYSTEM_2650_H



