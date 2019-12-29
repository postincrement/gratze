#ifndef MODEL1_H_
#define MODEL1_H_

#include "trs80.h"

#define   MODEL1_ROM_START_ADDR    0x0000
#define   MODEL1_ROM_END_ADDR      0x2fff

#define   MODEL1_MEMIO_START_ADDR  0x3000
#define   MODEL1_MEMIO_END_ADDR    0x37ff

#define   MODEL1_KB_START_ADDR     0x3800
#define   MODEL1_KB_END_ADDR       0x3bff

#define   MODEL1_VIDEO_START_ADDR  0x3c00
#define   MODEL1_VIDEO_END_ADDR    0x3fff

#define   MODEL1_RAM_START_ADDR    0x4000
#define   MODEL1_RAM_END_ADDR      0xffff

class Model1_Emulator : public TRS80Emulator
{
  public:
    Model1_Emulator();

    virtual void WrZ80(register uint16_t addr, register uint8_t val) override;
    virtual uint8_t RdZ80(register uint16_t addr) override;
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