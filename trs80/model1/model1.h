#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80/trs80.h"

class Model1_Emulator : public TRS80Emulator
{
  public:
    Model1_Emulator(EmulatorInfo * info);

    virtual bool Open(const Options & options) override;

    virtual void WriteMemory(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t ReadMemory(register uint16_t addr) override;

    virtual bool OpenVideo(MainWindow & mainWindow, const Options & options) override;

  protected:
    bool m_withEI;    
};

class Model1Level1_Emulator : public Model1_Emulator
{
  public:
    Model1Level1_Emulator();
};

class Model1Level2_Emulator : public Model1_Emulator
{
  public:
    Model1Level2_Emulator();
};
#endif // MODEL1_H_