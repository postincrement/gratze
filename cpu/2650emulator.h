
#ifndef S2650_EMULATOR_H_
#define S2650_EMULATOR_H_

#include "cpu/emulator.h"
#include "cpu/cpu_2650.h"

class S2650Emulator : public Emulator
{
  public:
    S2650Emulator(EmulatorInfo * info);

    // overrides from Emulator
    virtual bool Start(int addr = -1) override;
    virtual bool Run(int cycles = 1000)  override;

    virtual void NMI() override;
    virtual void Interrupt(uint16_t vector = 0) override;
    virtual void Reset(int addr = -1) override;
    virtual uint16_t ReadMemoryWord(uint16_t addr) override;

    virtual void GetStack(std::vector<uint16_t> & stack) override;
    virtual void SetTrace(bool v) override;

    virtual unsigned char ReadPortC();
    virtual void WritePortC(unsigned char data);
    virtual unsigned char ReadPortD();
    virtual void WritePortD(unsigned char data);

    //virtual unsigned char ReadPort(unsigned char port);
    //virtual void WritePort(unsigned char port, unsigned char data);

    virtual int GetMemorySize() const override;
    virtual std::string GetName() const override;

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) override;

    protected:
      std::unique_ptr<CPU2650> m_cpu;
};

#endif // Z80_EMULATOR_H_
