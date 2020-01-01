#ifndef MODEL4_H_
#define MODEL4_H_

#include "trs80/trs80.h"

class Model4_Emulator : public TRS80Emulator
{
  public:
    Model4_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual int GetDefaultRAMSize_k() const override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;

    virtual void GetScreenSizePixels(int & x, int & y) const override;
    virtual void GetScreenSizeChars(int & x, int & y) const override;

    virtual int GetVideoMemSize_k();

    virtual double GetTargetClockSpeed_Hz() const override;
};

#endif // MODEL4_H_