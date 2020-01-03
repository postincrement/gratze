#ifndef DG680_H_
#define DG680_H_

#include "config.h"
#include "cpu/z80emulator.h"
#include "options.h"

class DG680_Emulator : public Z80Emulator
{
  public:
    void Init();
    DG680_Emulator();
};

#endif // DG680_H_