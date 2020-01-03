
#ifndef Z80_EMULATOR_H_
#define Z80_EMULATOR_H_

#include "cpu/emulator.h"
#include "video/virtual_screen.h"

extern "C" {
#include "cpu/mfz80/Z80.h"
};

class Z80Emulator : public Emulator
{
  public:
    Z80Emulator(EmulatorInfo * info);

    static Z80Emulator * g_z80Instance;

    // overrides from Emulator
    virtual bool Start(int addr = -1) override;
    virtual bool Run(int cycles = 1000)  override;

    virtual void NMI() override;
    virtual void Interrupt(uint16_t vector = 0) override;
    virtual void Reset(int addr = -1) override;
    virtual uint16_t ReadMemoryWord(uint16_t addr) override;

    virtual void GetStack(std::vector<uint16_t> & stack) override;
    virtual void SetTrace(bool v) override;

    virtual void MemoryDump() override;

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) override;
    Z80 m_cpu;
    int m_cpuDelayRepeat;
    uint8_t m_delayBuffer[32];
};

#endif // Z80_EMULATOR_H_
