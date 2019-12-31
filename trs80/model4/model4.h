#ifndef MODEL4_H_
#define MODEL4_H_

#include "trs80.h"

class Model4_Emulator : public TRS80Emulator
{
  public:
    Model4_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual int GetDefaultRAMSize_k() const override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;
    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;

    virtual int GetVideoMemSize_k();

    virtual double GetTargetClockSpeed_Hz() const override;
};

#endif // MODEL4_H_