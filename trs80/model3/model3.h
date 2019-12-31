#ifndef MODEL3_H_
#define MODEL3_H_

#include "trs80.h"
#include "model1.h"

class Model3_Emulator : public Model1Level1_Emulator
{
  public:
    Model3_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;
    virtual void WrZ80(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t RdZ80(register uint16_t addr) override;

    virtual double GetTargetClockSpeed_Hz() const override;
};

#endif // MODEL3_H_

