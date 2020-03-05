#ifndef SORCERER_H_
#define SORCERER_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"

class Sorcerer_Emulator : public Z80Emulator
{
  public:
    Sorcerer_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    uint8_t OnReadKeyboard();

  protected:  
};

#endif // SORCERER_H_