#ifndef SYSTEM_2650_H
#define SYSTEM_2650_H

#include "cpu/2650emulator.h"

class BINBUG_2650 : public S2650Emulator
{
  public:
    void Init();
    BINBUG_2650();

    virtual void Reset(int addr) override;

    virtual void OnKeyDown(const SDL_Keysym & keysym) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data) override;

  protected:
    uint8_t m_keyboardData;

};

#endif // SYSTEM_2650_H



