
#ifndef Z80_EMULATOR_H_
#define Z80_EMULATOR_H_

#include <vector>

namespace MFZ {

extern "C" {
#include "z80/mfz80/Z80.h"

int DAsm(char *S,byte *A);

};

} // namespace MFZ

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
    virtual int Exec(int cycles)  override;
    virtual unsigned GetPC() const override;
    virtual void SetPC(unsigned) override;

    virtual void NMI() override;
    virtual void Interrupt(uint16_t vector = 0) override;
    virtual void Reset(int addr = -1) override;
    virtual uint16_t ReadMemoryWord(uint16_t addr) override;

    virtual void GetStack(std::vector<uint16_t> & stack) override;
    virtual void SetTrace(bool v) override;

    virtual int GetMemorySize() const override;
    virtual std::string GetName() const override;
    virtual std::string DumpRegs() const override;

    virtual unsigned GetOpcode(unsigned addr, std::vector<uint8_t> & opcodes) const override;
    virtual std::string DecodeOpcode(const std::vector<uint8_t> & code) const override;

    // Called when the CPU executes ED FE (MFZ PatchZ80). Default is a no-op.
    virtual void OnPatchZ80(MFZ::Z80 *R);

    // Shadow BDOS bank: instruction fetch in the window always uses m_shadowRam.
    // Data R/W in the window uses shadow when PC is in BDOS/BIOS, and main when
    // PC is in the TPA (so CCP/STAT see interceptor DPH/DPB/ALV).
    void SetShadowBdos(uint16_t base, const std::vector<uint8_t> & image);
    void ClearShadowBdos();
    void WriteShadowByte(uint16_t addr, uint8_t data);
    bool ShadowBdosActive() const { return m_shadowActive; }

    // While set, data reads in this range use main RAM even when PC is in the
    // shadow window — so guest BDOS can consume interceptor DPH/DPB/ALV.
    void SetInterceptorOverlay(uint16_t start, uint16_t end);
    void ClearInterceptorOverlay();

    uint8_t CpuReadMemory(uint16_t addr) const;
    void CpuWriteMemory(uint16_t addr, uint8_t data);
    uint8_t CpuFetchMemory(uint16_t addr) const;

  protected:
    virtual void DumpStackInternal(const std::vector<uint16_t> & stack) override;
    void ServicePendingInterrupt();

    bool InShadowWindow(uint16_t addr) const;
    bool PcInShadow() const;
    bool InInterceptorOverlay(uint16_t addr) const;

    MFZ::Z80 m_cpu;
    bool m_inExec = false;
    int m_cpuDelayRepeat;
    uint8_t m_delayBuffer[32];

    bool m_shadowActive = false;
    uint16_t m_shadowBase = 0;
    uint16_t m_shadowEnd = 0;
    std::vector<uint8_t> m_shadowRam;

    bool m_overlayActive = false;
    uint16_t m_overlayStart = 0;
    uint16_t m_overlayEnd = 0;
};

#endif // Z80_EMULATOR_H_
