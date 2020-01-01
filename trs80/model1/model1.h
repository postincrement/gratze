#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80/trs80.h"

class Model1_Emulator : public TRS80Emulator
{
  public:
    Model1_Emulator();

    virtual bool Open(const Options & options) override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;

    virtual void GetScreenSizePixels(int & x, int & y) const override;
    virtual void GetScreenSizeChars(int & x, int & y) const override;

    virtual double GetTargetClockSpeed_Hz() const override;

    virtual int GetVideoMemSize_k();

  protected:
    bool m_withEI;    
};

class Model1Level1_Emulator : public Model1_Emulator
{
  public:
    Model1Level1_Emulator();

    virtual std::string GetTitle() const  override;

    virtual int GetDefaultRAMSize_k() const override;
};

class Model1Level2_Emulator : public Model1_Emulator
{
  public:
    Model1Level2_Emulator();

    virtual std::string GetTitle() const override;

    virtual int GetDefaultRAMSize_k() const override;
};
#endif // MODEL1_H_