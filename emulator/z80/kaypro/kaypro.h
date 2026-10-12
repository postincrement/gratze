#ifndef KAYPRO_H_
#define KAYPRO_H_

#include <deque>
#include <memory>

#include "common/config.h"
#include "devices/fdc.h"
#include "devices/keyboard.h"
#include "src/options.h"
#include "z80/z80emulator.h"

extern struct CharacterGeneratorROM g_charGen_kaypro80x25;
extern unsigned char g_kaypro_81_232_ROM[4096];

// Memory-mapped I/O id and I/O port block ids (used by INFO_* macros).
#define KAYPRO_MEMORY_BANKED  1
#define KAYPRO_PORT_SIO       1
#define KAYPRO_PORT_FDC       2
#define KAYPRO_PORT_UNUSED    3  // Port 14: unmapped on II; */84 system port
#define KAYPRO_PORT_SYSTEM    4
#define KAYPRO_PORT_PARALLEL  5
#define KAYPRO_PORT_BAUD      6

class KayproVideo;

class Kaypro_Emulator : public Z80Emulator
{
  public:
    Kaypro_Emulator(const EmulatorInfo * info);

    void Instantiate() override;

    bool Open(const Options & options) override;
    void Reset(int addr = -1) override;
    int Exec(int cycles) override;

    uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t port) override;
    void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data) override;

    void WriteIOMemory(int id, uint16_t addr, uint8_t val) override;
    uint8_t ReadIOMemory(int id, uint16_t addr) const override;

    bool MountDrive(int driveNum, std::shared_ptr<VirtualDrive> drive, bool readOnly) override;
    void RefreshPanelDrives() override;

  protected:
    void ApplySystemBits(uint8_t data);
    void OnKeyChar(uint8_t ch);

    std::unique_ptr<WD_FDC> m_fdc;
    KayproVideo * m_video = nullptr;

    // System DRAM for 0000-3FFF. Visible at 0000-2FFF when bank bit 7 is
    // clear; 3000-3FFF is RAM when bit 7 is clear, VRAM when bit 7 is set.
    uint8_t m_lowRam[0x4000] = {};

    // Port 1C image. Bit 7 set => ROM+VRAM; clear => all RAM at 0000-3FFF.
    uint8_t m_systemBits = 0x80;
    bool m_romMapped = true;

    uint8_t m_sioControl = 0;
    int m_sioControlWrites = 0;

    std::deque<uint8_t> m_kbQueue;
};


class Kaypro2_Emulator : public Kaypro_Emulator
{
  public:
    Kaypro2_Emulator();
};


class KayproIV_Emulator : public Kaypro_Emulator
{
  public:
    KayproIV_Emulator();
};

#endif // KAYPRO_H_
