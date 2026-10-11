#ifndef CPMHOST_H_
#define CPMHOST_H_

#include <cstdint>
#include <string>

#include "src/options.h"
#include "z80/z80emulator.h"

//
// Shared CP/M host support for Z80 emulators.
//
// Guest code traps into the host with the undocumented Z80 opcode ED FE
// (handled by MFZ PatchZ80). The function code is in C; values use A:
//
//   0x00-0x24  standard BDOS functions (NewBDOS); result in A
//   0xFE       host CCP extensions (EXIT/LCD/LCP/LLS/LPWD); result in A
//   0xFF       warm boot; A = logged drive/user (from TDRIVE)
//   0xF0       BIOS CONST  → A = 0xff if key ready, else 0
//   0xF1       BIOS CONIN  → A = character, or 0xff if none
//   0xF2       BIOS CONOUT → character in A
//
// BIOS traps preserve BC (and CONOUT also preserves AF). BDOS traps follow
// normal CP/M return conventions (A / HL / BA).
//
// Memory layout constants must match newbdos.asm.
//

#define CPMHOST_MEM      64
#define CPMHOST_CCPLEN   0x800
#define CPMHOST_BDOSLEN  0x100
#define CPMHOST_BIOSLEN  0x100
#define CPMHOST_MEM_TOP  (CPMHOST_MEM * 1024)

#define CCPB  (CPMHOST_MEM_TOP - CPMHOST_BIOSLEN - CPMHOST_BDOSLEN - CPMHOST_CCPLEN)
#define BDOS  (CPMHOST_MEM_TOP - CPMHOST_BIOSLEN - CPMHOST_BDOSLEN)
#define BIOS  (CPMHOST_MEM_TOP - CPMHOST_BIOSLEN)

// Compatibility aliases used by cpm80 and the assembled image size macros.
#define MEM     CPMHOST_MEM
#define CCPLEN  CPMHOST_CCPLEN
#define BDOSLEN CPMHOST_BDOSLEN
#define BIOSLEN CPMHOST_BIOSLEN
#define MEM_TOP CPMHOST_MEM_TOP

namespace cpmhost {

constexpr uint8_t kTrapConst  = 0xF0;
constexpr uint8_t kTrapConin  = 0xF1;
constexpr uint8_t kTrapConout = 0xF2;

// Machine-specific services required by the shared NewBDOS host.
class Host
{
  public:
    virtual ~Host() = default;

    virtual MFZ::Z80 & Cpu() = 0;
    virtual uint8_t * Memory() = 0;
    virtual void ConsoleOut(char ch) = 0;
    virtual int ConsoleIn() = 0;          // next char, or -1 if none
    virtual bool ConsoleStatus() = 0;     // true if a char is waiting
    virtual void RunPollers() = 0;
    virtual void WriteMemory(uint16_t addr, uint8_t data) = 0;
    virtual bool LoadFile(const std::string & path) = 0;
    virtual const Options & GetOptions() const = 0;
};

} // namespace cpmhost

#endif // CPMHOST_H_
