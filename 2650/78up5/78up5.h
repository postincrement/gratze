#ifndef EA78UP5_H_
#define EA78UP5_H_

#include "2650/2650emulator.h"

class EA78UP5_Emulator : public S2650Emulator
{
  public:
    EA78UP5_Emulator(const EmulatorInfo * info, int baud);

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr) override;

    protected:
      int m_baud;
      uint64_t m_serialInterval;
};

class EA78UP5_PIPBUG_110 : public EA78UP5_Emulator
{
  public:
    EA78UP5_PIPBUG_110();
};

class EA78UP5_PIPBUG_300 : public EA78UP5_Emulator
{
  public:
    EA78UP5_PIPBUG_300();
};

#endif // EA78UP5_H_



