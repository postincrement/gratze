#ifndef CPMHOST_SHADOW_BDOS_H_
#define CPMHOST_SHADOW_BDOS_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "z80/cpmhost/cpmhost.h"
#include "z80/cpmhost/newbdos.h"

namespace cpmhost {

// shadowBdos mode controller.
//
// Naming:
//   Emulated BDOS  — cpm80 NewBDOS (not used here)
//   Interceptor BDOS — same NewBDOS class, dir-only; main stub + DPB/ALV
//   Shadow BDOS — guest BDOS copy in shadow RAM (images/FDC/console/pass-through)
//
// Directory-drive emulation is shared with emulated BDOS via NewBDOS/CpmDrive.
class ShadowBdos
{
  public:
    explicit ShadowBdos(Z80Emulator & emu, Host & host);

    bool Init();
    void Poll();
    void OnPatchZ80(MFZ::Z80 *R);

    // Interceptor BDOS: NewBDOS in injected (dir-only) mode.
    NewBDOS * Interceptor() { return m_interceptor.get(); }
    const NewBDOS * Interceptor() const { return m_interceptor.get(); }
    bool Armed() const { return m_armed; }

  private:
    bool TryArm();
    void Install(uint16_t bdosEntry, uint16_t warmBoot);
    void InstallInterceptorBuffers();
    void PatchEntryTrampoline();
    void RestoreShadowBdosEntry();
    void PatchBiosTrap(uint16_t addr, uint8_t saved[3]);
    void RestoreBiosTrap(uint16_t addr, const uint8_t saved[3]);
    void InstallBiosTraps();
    void RepatchBiosTraps();
    uint16_t ResolveBiosTarget(uint16_t jumpSlot) const;
    bool HandleBiosTrap(MFZ::Z80 *R, uint16_t trapAddr);
    void ResumeBiosRoutine(MFZ::Z80 *R, uint16_t addr, uint8_t saved[3]);
    bool ShouldIntercept(uint8_t code) const;
    int DriveForCall(uint8_t code) const;
    void PassThroughShadowBdos();
    int FirstImageDrive() const;
    void KeepHostTDriveOffDirectory();
    void BiosSeldsk(MFZ::Z80 *R);
    void BiosRead(MFZ::Z80 *R);
    void BiosWrite(MFZ::Z80 *R);

    Z80Emulator & m_emu;
    Host & m_host;
    std::unique_ptr<NewBDOS> m_interceptor;

    bool m_armed = false;
    int m_passThroughDepth = 0;
    bool m_biosRepatchPending = false;

    uint16_t m_bdosBase = 0;
    uint16_t m_bdosEntry = 0;
    uint16_t m_bdosEnd = 0;
    uint16_t m_warmBoot = 0;
    uint16_t m_coldBoot = 0;
    uint16_t m_dpbAddr = 0;
    uint16_t m_alvAddr = 0;
    uint16_t m_alvMax = 0;

    // Original shadow BDOS entry bytes (before trampoline patch).
    uint8_t m_savedShadowEntry[3] = {0, 0, 0};

    // BIOS disk traps in main memory (outside the shadow window).
    uint16_t m_biosSeldsk = 0;
    uint16_t m_biosSettrk = 0;
    uint16_t m_biosSetsec = 0;
    uint16_t m_biosSetdma = 0;
    uint16_t m_biosRead = 0;
    uint16_t m_biosWrite = 0;
    uint8_t m_savedSeldsk[3] = {0, 0, 0};
    uint8_t m_savedSetdma[3] = {0, 0, 0};
    uint8_t m_savedRead[3] = {0, 0, 0};
    uint8_t m_savedWrite[3] = {0, 0, 0};

    int m_biosDisk = 0;
    uint16_t m_biosTrack = 0;
    uint16_t m_biosSector = 0;
    uint16_t m_biosDma = 0x80;
};

} // namespace cpmhost

#endif // CPMHOST_SHADOW_BDOS_H_
