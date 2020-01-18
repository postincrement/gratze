
#ifndef Z80_EMULATOR_H_
#define Z80_EMULATOR_H_

extern "C" {
#include "z80/mfz80/Z80.h"
};

#include "src/emulator.h"
#include "video/virtual_screen.h"

class Z80Emulator : public Emulator
{
  public:
    Z80Emulator(const EmulatorInfo * info);

    virtual bool Open(const Options & options) override;

    static Z80Emulator * g_z80Instance;

    // overrides from Emulator
    virtual bool Start(int addr = -1) override;
    virtual bool Exec(int cycles = 1000)  override;

    virtual void NMI() override;
    virtual void Interrupt(uint16_t vector = 0) override;
    virtual void Reset(int addr = -1) override;
    virtual uint16_t ReadMemoryWord(uint16_t addr) override;

    virtual void GetStack(std::vector<uint16_t> & stack) override;
    virtual void SetTrace(bool v) override;

    virtual int GetMemorySize() const override;
    virtual std::string GetName() const override;
    virtual std::string DumpRegs() const;

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) override;
    Z80 m_cpu;
    int m_cpuDelayRepeat;
    uint8_t m_delayBuffer[32];
};

#endif // Z80_EMULATOR_H_
