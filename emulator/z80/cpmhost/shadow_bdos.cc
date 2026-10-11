#include "z80/cpmhost/shadow_bdos.h"

#include <cstring>
#include <iostream>

using namespace std;

namespace cpmhost {

// Interceptor BDOS layout in main (program-visible):
//   +16  DPH (16 bytes) — BDOS 14 returns this
//   +32  DPB (16 bytes) — BDOS 31 / DPH pointer
//   +48  DIRBUF (128 bytes)
//   +176 ALV (remainder)
static constexpr uint16_t kDphOffset    = 16;
static constexpr uint16_t kDpbOffset    = 32;
static constexpr uint16_t kDirBufOffset = 48;
static constexpr uint16_t kAlvOffset    = 176;

ShadowBdos::ShadowBdos(Z80Emulator & emu, Host & host)
  : m_emu(emu)
  , m_host(host)
{
}

bool ShadowBdos::Init()
{
  m_interceptor.reset(new NewBDOS(m_host));
  m_interceptor->SetInjected(true);
  m_interceptor->LogStartup();
  int dirs = 0;
  for (int d = 0; d < 16; ++d) {
    if (m_interceptor->IsDirectoryDrive(d)) {
      ++dirs;
      cerr << "info: interceptor directory drive " << (char)('A' + d)
           << ": " << m_interceptor->m_drives[d].m_path << endl;
    }
  }
  if (dirs == 0)
    cerr << "warning: shadowBdos enabled but no directory drives were mounted" << endl;
  return true;
}

void ShadowBdos::Poll()
{
  if (m_armed)
    return;
  // Arm as soon as page-0 has a valid CP/M vector set — do not wait for
  // PC==0005 (that race left the interceptor never installed).
  TryArm();
}

int ShadowBdos::FirstImageDrive() const
{
  // Under shadowBdos, image drives are FDC-mounted and often absent from
  // NewBDOS's drive table — treat any non-directory unit as an image drive.
  for (int d = 0; d < 16; ++d) {
    if (!m_interceptor->IsDirectoryDrive(d))
      return d;
  }
  return 0;
}

bool ShadowBdos::TryArm()
{
  uint8_t jp5 = m_emu.ReadMemory(0x0005);
  uint8_t jp0 = m_emu.ReadMemory(0x0000);
  uint16_t bdosEntry = m_emu.ReadMemoryWord(0x0006);
  uint16_t warmBoot = m_emu.ReadMemoryWord(0x0001);

  // Occasional progress so we can see why arming has not happened yet.
  static int s_tries = 0;
  ++s_tries;
  const bool logProgress = (s_tries == 1) || (s_tries % 250000) == 0;
  if (logProgress && m_interceptor) {
    m_interceptor->m_debug << "arm poll #" << s_tries
                           << " PC=" << hex << m_emu.GetPC()
                           << " [0]=" << (int)jp0
                           << " [5]=" << (int)jp5
                           << " w6=" << bdosEntry
                           << " w1=" << warmBoot << dec << endl;
    m_interceptor->m_debug.flush();
    if (s_tries == 1 || (s_tries % 1000000) == 0)
      cerr << "info: shadowBdos arm poll #" << s_tries
           << " [0]=" << (int)jp0 << " [5]=" << (int)jp5
           << " w6=" << hex << bdosEntry << " w1=" << warmBoot << dec << endl;
  }

  if (jp5 != 0xc3 || jp0 != 0xc3)
    return false;
  if (bdosEntry < 6 || warmBoot < 3)
    return false;
  // Reject clearly bogus vectors (e.g. still ROM/zeroed page-0).
  if (bdosEntry < 0x0100 || warmBoot < 0x0100)
    return false;

  uint16_t bdosBase = (uint16_t)(bdosEntry - 6);
  uint16_t biosBase = (uint16_t)(warmBoot - 3);
  if (biosBase <= bdosBase || (biosBase - bdosBase) < 0x80 ||
      (biosBase - bdosBase) > 0x4000) {
    if (m_interceptor) {
      m_interceptor->m_debug << "arm reject layout base=" << hex << bdosBase
                             << " bios=" << biosBase << dec << endl;
      m_interceptor->m_debug.flush();
    }
    return false;
  }

  Install(bdosEntry, warmBoot);
  m_armed = true;

  std::ostream & log = m_interceptor->m_debug;
  log << "shadowBdos ARMED base=" << hex << m_bdosBase
      << " entry=" << m_bdosEntry
      << " end=" << m_bdosEnd
      << " warm=" << m_warmBoot
      << " trampoline=" << (int)m_emu.CpuFetchMemory(m_bdosEntry)
      << " " << (int)m_emu.CpuFetchMemory((uint16_t)(m_bdosEntry + 1))
      << " after " << dec << s_tries << " polls" << endl;
  log.flush();

  cerr << "info: shadowBdos armed: base=" << hex << m_bdosBase
       << " entry=" << m_bdosEntry
       << " end=" << m_bdosEnd
       << " warm=" << m_warmBoot << dec << endl;
  return true;
}

void ShadowBdos::Install(uint16_t bdosEntry, uint16_t warmBoot)
{
  m_bdosEntry = bdosEntry;
  m_bdosBase = (uint16_t)(bdosEntry - 6);
  m_warmBoot = warmBoot;
  m_coldBoot = (uint16_t)(warmBoot - 3);
  // Shadow/interceptor window is exactly the guest BDOS region (do not enter BIOS).
  m_bdosEnd = m_coldBoot;
  if (m_bdosEnd <= m_bdosBase + kAlvOffset + 16) {
    cerr << "warning: guest BDOS region too small for interceptor tables; "
         << "using compact layout" << endl;
  }

  const uint16_t span = (uint16_t)(m_bdosEnd - m_bdosBase);

  // Interceptor DPH/DPB/DIRBUF/ALV must live outside the shadow BDOS window.
  // Guest select copies the DPH while PC is in BDOS (shadow); addresses inside
  // [bdosBase,bdosEnd) would read guest code instead of our tables.
  // 0xEE00 is above Microbee BIOS disk routines (~ec6f) and below video (f000).
  static constexpr uint16_t kHostWs = 0xee00;
  const uint16_t dphAddr    = kHostWs;
  const uint16_t dpbAddr    = (uint16_t)(kHostWs + 16);
  const uint16_t dirBufAddr = (uint16_t)(kHostWs + 32);
  const uint16_t alvAddr    = (uint16_t)(kHostWs + 160);
  const uint16_t alvMax     = 256;
  m_dpbAddr = dpbAddr;
  m_alvAddr = alvAddr;
  m_alvMax  = alvMax;

  // Shadow BDOS = copy of guest BDOS from main, then patch entry trampoline.
  std::vector<uint8_t> shadow(span);
  for (uint16_t i = 0; i < span; ++i)
    shadow[i] = m_emu.ReadMemory((uint16_t)(m_bdosBase + i));

  const uint16_t entryOff = (uint16_t)(m_bdosEntry - m_bdosBase);
  if ((size_t)entryOff + 3 <= shadow.size()) {
    m_savedShadowEntry[0] = shadow[entryOff + 0];
    m_savedShadowEntry[1] = shadow[entryOff + 1];
    m_savedShadowEntry[2] = shadow[entryOff + 2];
    shadow[entryOff + 0] = 0xed;
    shadow[entryOff + 1] = 0xfe;
    shadow[entryOff + 2] = 0xc9;
  }

  m_emu.SetShadowBdos(m_bdosBase, shadow);
  InstallInterceptorBuffers();

  m_interceptor->SetWorkspace(dphAddr, dpbAddr, dirBufAddr, alvAddr, alvMax);
  m_interceptor->SetGuestWarmBoot(m_warmBoot);
  m_interceptor->SetBiosConsole(
      (uint16_t)(m_coldBoot + 6),
      (uint16_t)(m_coldBoot + 9),
      (uint16_t)(m_coldBoot + 12));

  m_interceptor->m_debug << "interceptor workspace dph=" << hex << dphAddr
                         << " dpb=" << dpbAddr
                         << " dirbuf=" << dirBufAddr
                         << " alv=" << alvAddr << dec << endl;

  // Trap real BIOS disk callees (not jump-table slots) so nested CALL 5 during
  // pass-through still hits host-directory handling.
  InstallBiosTraps();

  // Microbee select continuation at e531 is `CALL Z,dc47` after SELDSK. The
  // Z flag from the DPH-copy tail is often set even on success, which sends
  // host-directory selects into the Select error path. Neutralize it.
  if (m_emu.CpuFetchMemory(0xe531) == 0xcc &&
      m_emu.CpuFetchMemory(0xe532) == 0x47 &&
      m_emu.CpuFetchMemory(0xe533) == 0xdc) {
    m_emu.WriteShadowByte(0xe531, 0x00);
    m_emu.WriteShadowByte(0xe532, 0x00);
    m_emu.WriteShadowByte(0xe533, 0x00);
    m_interceptor->m_debug << "patched out CALL Z,dc47 at e531" << endl;
  }
}

void ShadowBdos::PatchBiosTrap(uint16_t addr, uint8_t saved[3])
{
  uint8_t b0 = m_emu.ReadMemory(addr);
  uint8_t b1 = m_emu.ReadMemory((uint16_t)(addr + 1));
  if (!(b0 == 0xed && b1 == 0xfe)) {
    saved[0] = b0;
    saved[1] = b1;
    saved[2] = m_emu.ReadMemory((uint16_t)(addr + 2));
  }
  m_emu.WriteMemory(addr, 0xed);
  m_emu.WriteMemory((uint16_t)(addr + 1), 0xfe);
  m_emu.WriteMemory((uint16_t)(addr + 2), 0xc9);
}

void ShadowBdos::RestoreBiosTrap(uint16_t addr, const uint8_t saved[3])
{
  m_emu.WriteMemory(addr, saved[0]);
  m_emu.WriteMemory((uint16_t)(addr + 1), saved[1]);
  m_emu.WriteMemory((uint16_t)(addr + 2), saved[2]);
}

uint16_t ShadowBdos::ResolveBiosTarget(uint16_t jumpSlot) const
{
  // Prefer the JP target so we trap the real routine. Jump-table slots can
  // be skipped while the BDOS trampoline is temporarily restored.
  uint8_t op = m_emu.ReadMemory(jumpSlot);
  if (op == 0xc3) {
    return (uint16_t)(m_emu.ReadMemory((uint16_t)(jumpSlot + 1)) |
                      (m_emu.ReadMemory((uint16_t)(jumpSlot + 2)) << 8));
  }
  if (op == 0xed && m_emu.ReadMemory((uint16_t)(jumpSlot + 1)) == 0xfe) {
    // Already patched; leave as-is (caller has saved target elsewhere).
    return jumpSlot;
  }
  return jumpSlot;
}

void ShadowBdos::RepatchBiosTraps()
{
  PatchBiosTrap(m_biosSeldsk, m_savedSeldsk);
  PatchBiosTrap(m_biosRead, m_savedRead);
  PatchBiosTrap(m_biosWrite, m_savedWrite);
  m_biosRepatchPending = false;
}

void ShadowBdos::InstallBiosTraps()
{
  const uint16_t slotSeldsk = (uint16_t)(m_coldBoot + 27);
  const uint16_t slotSettrk = (uint16_t)(m_coldBoot + 30);
  const uint16_t slotSetsec = (uint16_t)(m_coldBoot + 33);
  const uint16_t slotSetdma = (uint16_t)(m_coldBoot + 36);
  const uint16_t slotRead   = (uint16_t)(m_coldBoot + 39);
  const uint16_t slotWrite  = (uint16_t)(m_coldBoot + 42);

  m_biosSettrk = slotSettrk;
  m_biosSetsec = slotSetsec;
  m_biosSetdma = slotSetdma;

  // Trap the callee, not the jump table — survives nested CALL 5 while the
  // BDOS entry trampoline is restored for pass-through.
  if (m_biosSeldsk == 0 || m_emu.ReadMemory(m_biosSeldsk) != 0xed)
    m_biosSeldsk = ResolveBiosTarget(slotSeldsk);
  if (m_biosRead == 0 || m_emu.ReadMemory(m_biosRead) != 0xed)
    m_biosRead = ResolveBiosTarget(slotRead);
  if (m_biosWrite == 0 || m_emu.ReadMemory(m_biosWrite) != 0xed)
    m_biosWrite = ResolveBiosTarget(slotWrite);

  RepatchBiosTraps();

  m_interceptor->m_debug << "bios traps seldsk=" << hex << m_biosSeldsk
                         << " (slot " << slotSeldsk << ")"
                         << " read=" << m_biosRead
                         << " write=" << m_biosWrite
                         << " seldskbytes:";
  for (int i = 0; i < 24; ++i)
    m_interceptor->m_debug << " " << (int)m_emu.ReadMemory((uint16_t)(m_biosSeldsk + i));
  // Guest bioscall + post-SELDSK (CALL is at dc5d on this Microbee BDOS).
  m_interceptor->m_debug << " bioscall:";
  for (int i = 0; i < 64; ++i)
    m_interceptor->m_debug << " " << (int)m_emu.CpuFetchMemory((uint16_t)(0xdc59 + i));
  m_interceptor->m_debug << " dc47:";
  for (int i = 0; i < 24; ++i)
    m_interceptor->m_debug << " " << (int)m_emu.CpuFetchMemory((uint16_t)(0xdc47 + i));
  m_interceptor->m_debug << " ddea:";
  for (int i = 0; i < 32; ++i)
    m_interceptor->m_debug << " " << (int)m_emu.CpuFetchMemory((uint16_t)(0xddea + i));
  m_interceptor->m_debug << dec << endl;
  m_interceptor->m_debug.flush();
}

void ShadowBdos::ResumeBiosRoutine(MFZ::Z80 *R, uint16_t addr, uint8_t saved[3])
{
  RestoreBiosTrap(addr, saved);
  m_biosRepatchPending = true;
  R->PC.W = addr;
}

void ShadowBdos::BiosSeldsk(MFZ::Z80 *R)
{
  // CP/M BIOS SELDSK: drive in C. Some callers also leave it in A — accept
  // either when it names a host directory.
  int drive = R->BC.B.l & 0x0f;
  if (!m_interceptor->IsDirectoryDrive(drive)) {
    int aDrive = R->AF.B.h & 0x0f;
    if (m_interceptor->IsDirectoryDrive(aDrive))
      drive = aDrive;
  }
  m_biosDisk = drive;

  if (m_interceptor->IsDirectoryDrive(drive)) {
    R->DE.B.l = (uint8_t)drive;
    m_interceptor->SelDisk();
    KeepHostTDriveOffDirectory();
    m_biosDma = m_interceptor->m_dmaAddress;
    if (m_interceptor->m_dphAddr != 0) {
      uint16_t dirbuf = (uint16_t)(
          m_emu.ReadMemory((uint16_t)(m_interceptor->m_dphAddr + 8)) |
          (m_emu.ReadMemory((uint16_t)(m_interceptor->m_dphAddr + 9)) << 8));
      if (dirbuf != 0)
        m_biosDma = dirbuf;
    }
    const uint16_t dph = R->HL.W;
    m_interceptor->m_debug << "bios SELDSK host " << (char)('A' + drive)
                           << " HL=" << hex << dph
                           << " SP=" << R->SP.W << dec << endl;
    m_interceptor->m_debug.flush();
    cerr << "info: bios SELDSK host " << (char)('A' + drive)
         << " HL=" << hex << dph << dec << endl;

    // Install DPH pointers the way Microbee bioscall (dc59) would, then skip
    // the fragile post-SELDSK flag checks (CALL Z / RET C) that mis-detect
    // host-directory success as "Select" failure. These BDOS variables live
    // in the shadow window — write shadow, not main.
    auto wr16 = [&](uint16_t addr, uint16_t val) {
      m_emu.WriteShadowByte(addr, (uint8_t)(val & 0xff));
      m_emu.WriteShadowByte((uint16_t)(addr + 1), (uint8_t)(val >> 8));
    };
    uint16_t xlt = (uint16_t)(
        m_emu.ReadMemory(dph) | (m_emu.ReadMemory((uint16_t)(dph + 1)) << 8));
    wr16(0xe6b3, (uint16_t)(dph + 2));
    wr16(0xe6b5, (uint16_t)(dph + 4));
    wr16(0xe6b7, (uint16_t)(dph + 6));
    wr16(0xe6d0, xlt);
    for (int i = 0; i < 8; ++i)
      m_emu.WriteShadowByte((uint16_t)(0xe6b9 + i),
                            m_emu.ReadMemory((uint16_t)(dph + 8 + i)));

    // Microbee BDOS exit (e691) returns HL from (dc45), not the HL register.
    // e674 also consults e6de for disk-error post-processing — clear it and
    // jump straight to the SP-restore / return-value load.
    wr16(0xdc45, dph);
    m_emu.WriteShadowByte(0xe6de, 0);
    R->HL.W = dph;
    R->AF.B.h = (uint8_t)(dph & 0xff);
    R->BC.B.h = (uint8_t)(dph >> 8);
    R->AF.B.l = (uint8_t)(R->AF.B.l & ~0x41);
    R->PC.W = 0xe691;
    return;
  }

  // Image/FDC select: resume the real routine. CALL return is still on stack.
  m_interceptor->m_debug << "bios SELDSK image " << (char)('A' + drive)
                         << " via " << hex << m_biosSeldsk << dec << endl;
  m_interceptor->m_debug.flush();
  ResumeBiosRoutine(R, m_biosSeldsk, m_savedSeldsk);
}

void ShadowBdos::BiosRead(MFZ::Z80 *R)
{
  if (m_interceptor->IsDirectoryDrive(m_biosDisk)) {
    // Guest BDOS login scans the directory via BIOS READ. Provide empty
    // (0xE5) sectors so login succeeds; file ops use interceptor BDOS.
    uint16_t dma = m_biosDma ? m_biosDma : m_interceptor->m_dmaAddress;
    if (dma == 0)
      dma = 0x80;
    for (int i = 0; i < 128; ++i)
      m_emu.WriteMemory((uint16_t)(dma + i), 0xe5);
    R->AF.B.h = 0;
    m_interceptor->m_debug << "bios READ host dir dma=" << hex << dma << dec << endl;
    m_interceptor->m_debug.flush();
    return;
  }

  ResumeBiosRoutine(R, m_biosRead, m_savedRead);
}

void ShadowBdos::BiosWrite(MFZ::Z80 *R)
{
  if (m_interceptor->IsDirectoryDrive(m_biosDisk)) {
    R->AF.B.h = 0;
    m_interceptor->m_debug << "bios WRITE host dir (ignored)" << endl;
    m_interceptor->m_debug.flush();
    return;
  }

  ResumeBiosRoutine(R, m_biosWrite, m_savedWrite);
}

bool ShadowBdos::HandleBiosTrap(MFZ::Z80 *R, uint16_t trapAddr)
{
  if (trapAddr == 0 || m_biosSeldsk == 0)
    return false;

  // If this ED FE is at a trap we currently have restored for resume, the
  // pending flag means we should not steal it — but resume sets PC to the
  // restored bytes, so we only get here for a live trampoline.
  if (trapAddr == m_biosSeldsk) {
    BiosSeldsk(R);
    return true;
  }
  if (trapAddr == m_biosRead) {
    BiosRead(R);
    return true;
  }
  if (trapAddr == m_biosWrite) {
    BiosWrite(R);
    return true;
  }

  // Different ED FE (BDOS entry): re-arm any BIOS traps left restored.
  if (m_biosRepatchPending || m_emu.ReadMemory(m_biosSeldsk) != 0xed)
    RepatchBiosTraps();

  (void)m_biosSetdma;
  (void)m_biosSettrk;
  (void)m_biosSetsec;
  return false;
}

void ShadowBdos::InstallInterceptorBuffers()
{
  for (uint16_t a = m_bdosBase; a < m_bdosEnd; ++a)
    m_emu.WriteMemory(a, 0x00);
}

void ShadowBdos::PatchEntryTrampoline()
{
  m_emu.WriteShadowByte(m_bdosEntry, 0xed);
  m_emu.WriteShadowByte((uint16_t)(m_bdosEntry + 1), 0xfe);
  m_emu.WriteShadowByte((uint16_t)(m_bdosEntry + 2), 0xc9);
}

void ShadowBdos::RestoreShadowBdosEntry()
{
  m_emu.WriteShadowByte(m_bdosEntry, m_savedShadowEntry[0]);
  m_emu.WriteShadowByte((uint16_t)(m_bdosEntry + 1), m_savedShadowEntry[1]);
  m_emu.WriteShadowByte((uint16_t)(m_bdosEntry + 2), m_savedShadowEntry[2]);
}

int ShadowBdos::DriveForCall(uint8_t code) const
{
  MFZ::Z80 & cpu = m_host.Cpu();

  if (code == 14)
    return cpu.DE.B.l & 0x0f;

  switch (code) {
    case 15: case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23:
    case 30: case 33: case 34: case 35: case 36: {
      // Search Next must use the Search First FCB; some programs leave DE
      // pointing elsewhere on BDOS 18.
      uint16_t fcbAddr = cpu.DE.W;
      if (code == 18 && m_interceptor->m_findFCB != 0)
        fcbAddr = m_interceptor->m_findFCB;
      uint8_t fcbDrive = m_emu.ReadMemory(fcbAddr);
      if (fcbDrive == 0 || fcbDrive == '?')
        return m_interceptor->m_currDisk;
      int drive = (fcbDrive & 0x1f) - 1;
      if (drive < 0 || drive > 15)
        return m_interceptor->m_currDisk;
      return drive;
    }
    default:
      break;
  }

  return m_interceptor->m_currDisk;
}

bool ShadowBdos::ShouldIntercept(uint8_t code) const
{
  // Interceptor BDOS: directory-drive disk ops only.
  // Everything else → shadow BDOS (guest).
  switch (code) {
    case 13:
    case 0xff:
    case 0xfe:
      return false;
    case 14:
    case 15: case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23:
    case 24: case 25: case 26: case 27: case 29:
    case 30: case 31: case 32:
    case 33: case 34: case 35: case 36:
      break;
    default:
      return false;
  }

  // 25/32 are process-global; 24 login must include host dirs.
  // 26 (Set DMA) must reach the guest BDOS so image/FDC I/O uses the
  // same DMA pointer — track it on the way through OnPatchZ80 instead.
  if (code == 24 || code == 25 || code == 32) {
    for (int d = 0; d < 16; ++d) {
      if (m_interceptor->IsDirectoryDrive(d))
        return true;
    }
    return false;
  }

  return m_interceptor->IsDirectoryDrive(DriveForCall(code));
}

void ShadowBdos::KeepHostTDriveOffDirectory()
{
  const uint8_t tdrive = m_host.GetTDrive();
  const int image = FirstImageDrive();
  m_host.SetTDrive((uint8_t)((tdrive & 0xf0) | (image & 0x0f)));
}

void ShadowBdos::PassThroughShadowBdos()
{
  MFZ::Z80 & cpu = m_host.Cpu();
  const uint16_t retAddr = m_emu.ReadMemoryWord(cpu.SP.W);
  const int savedICount = cpu.ICount;
  const int savedIBackup = cpu.IBackup;

  // Guest BDOS must never see a host-directory unit in TDRIVE — it will
  // FDC-select it and print "Bdos Err On B: Select".
  const uint8_t tdrive = m_host.GetTDrive();
  KeepHostTDriveOffDirectory();
  const uint8_t imageTdrive = m_host.GetTDrive();

  // Restore the real guest entry for this call. Nested CALL 5 will not hit
  // the interceptor, but BIOS routine traps (at the callee) still cover
  // host directories.
  if (m_biosRepatchPending || m_emu.ReadMemory(m_biosSeldsk) != 0xed)
    RepatchBiosTraps();
  ++m_passThroughDepth;
  RestoreShadowBdosEntry();
  cpu.PC.W = m_bdosEntry;

  int guard = 2000000;
  while (--guard > 0) {
    ExecZ80(&cpu, 1);
    if (cpu.PC.W == retAddr)
      break;
    if ((guard & 0xff) == 0)
      m_host.RunPollers();
  }
  --m_passThroughDepth;

  PatchEntryTrampoline();
  if (m_biosRepatchPending || m_emu.ReadMemory(m_biosSeldsk) != 0xed)
    RepatchBiosTraps();
  if (!m_interceptor->IsDirectoryDrive(m_interceptor->m_currDisk))
    m_host.SetTDrive(tdrive);
  else
    m_host.SetTDrive(imageTdrive);

  cpu.ICount = savedICount;
  cpu.IBackup = savedIBackup;

  if (guard <= 0) {
    cerr << "error: shadow BDOS pass-through timed out PC="
         << hex << cpu.PC.W << " ret=" << retAddr << dec << endl;
    cpu.PC.W = retAddr;
  }
}

void ShadowBdos::OnPatchZ80(MFZ::Z80 *R)
{
  if (!m_interceptor)
    return;

  if (!m_armed) {
    m_interceptor->m_debug << "interceptor: ED FE while not armed PC="
                           << hex << R->PC.W << dec << endl;
    m_interceptor->m_debug.flush();
    return;
  }

  // ED FE leaves PC at the byte after FE (our C9). Trap site = PC - 2.
  const uint16_t trapAddr = (uint16_t)(R->PC.W - 2);
  if (HandleBiosTrap(R, trapAddr))
    return;

  uint8_t jp5 = m_emu.ReadMemory(0x0005);
  if (jp5 == 0xc3) {
    uint16_t entry = m_emu.ReadMemoryWord(0x0006);
    if (entry != m_bdosEntry && entry >= 6) {
      uint16_t warm = m_emu.ReadMemoryWord(0x0001);
      Install(entry, warm);
    }
  }

  const uint8_t code = R->BC.B.l;
  if (code == 26)
    m_interceptor->m_dmaAddress = R->DE.W;

  if (ShouldIntercept(code)) {
    const int drive = DriveForCall(code);
    m_interceptor->m_debug << "interceptor: trigger fn=" << (int)code
                           << " DE=" << hex << R->DE.W
                           << " drive=" << (char)('A' + (drive & 15))
                           << dec << endl;
    m_interceptor->m_debug.flush();
    m_interceptor->OnTrap(code);
    if (code == 14) {
      m_interceptor->m_debug << "interceptor: select result HL=" << hex << R->HL.W
                             << " A=" << (int)R->AF.B.h
                             << " TDRIVE=" << (int)m_host.GetTDrive() << dec << endl;
      m_interceptor->m_debug.flush();
      cerr << "info: BDOS select " << (char)('A' + (drive & 15))
           << " HL=" << hex << R->HL.W << dec << endl;
    }
  }
  else {
    const int drive = (code == 14) ? (int)(R->DE.B.l & 0x0f) : -1;
    m_interceptor->m_debug << "interceptor: passthrough fn=" << (int)code
                           << " DE=" << hex << R->DE.W;
    if (drive >= 0)
      m_interceptor->m_debug << " drive=" << (char)('A' + drive);
    m_interceptor->m_debug << dec << endl;
    m_interceptor->m_debug.flush();
    PassThroughShadowBdos();
  }
}

} // namespace cpmhost
