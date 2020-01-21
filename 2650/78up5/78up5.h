#ifndef EA78UP5_H_
#define EA78UP5_H_

#include "2650/2650emulator.h"

class EA78UP5_Emulator : public S2650Emulator
{
  public:
    EA78UP5_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr) override;

    void SerialIn(double secs, uint64_t clocks);
    void SerialOut(uint8_t ch)
};

#endif // EA78UP5_H_



