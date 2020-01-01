#ifndef MODEL3_H_
#define MODEL3_H_

#include "trs80/trs80.h"
#include "trs80/model1/model1.h"

class Model3_Emulator : public Model1Level1_Emulator
{
  public:
    Model3_Emulator();
    
    virtual std::string GetTitle() const override;

    virtual void GetScreenSizePixels(int & x, int & y) const override;
    virtual void GetScreenSizeChars(int & x, int & y) const  override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;
    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual double GetTargetClockSpeed_Hz() const override;
};

#endif // MODEL3_H_

