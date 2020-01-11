#ifndef ETI_685_H_
#define ETI_685_H_

#include "2650/2650emulator.h"

class ETI685 : public S2650Emulator
{
  public:
    void Init();
    ETI685();

    virtual void Reset(int addr) override;

    virtual void OnKeyDown(const SDL_Keysym & keysym) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data) override;

  protected:
    uint8_t m_keyboardData;

};

#endif // ETI_685_H_



