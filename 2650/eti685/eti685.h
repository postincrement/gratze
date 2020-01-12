#ifndef ETI_685_H_
#define ETI_685_H_

#include "2650/2650emulator.h"
#include "devices/intel8255.h"
#include "devices/keypar.h"

class ETI685 : public S2650Emulator
{
  public:
    ETI685();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr) override;

    virtual void SetKeyboardData(uint8_t data);

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data) override;

  protected:
    ParallelKeyboard m_keyboard;
    Intel8255 m_ppi;
    uint8_t m_keyboardData;
};

#endif // ETI_685_H_



