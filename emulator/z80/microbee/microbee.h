#ifndef MICROBEE_H_
#define MICROBEE_H_

#include "common/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"
#include "devices/synertek6545.h"

#include "starnet/starnet_client.h"

extern struct CharacterGeneratorROM g_charGen_mbee64x16;
extern struct CharacterGeneratorROM g_charGen_mbee80x24;

class MicrobeeVideo;

class Microbee_Emulator : public Z80Emulator
{
  public:
    Microbee_Emulator(const EmulatorInfo * info);

    void Instantiate() override;

    virtual bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    virtual void WriteIOMemory(int id, uint16_t addr, uint8_t val) override;
    virtual uint8_t ReadIOMemory(int id, uint16_t addr) const override;

    void OnPIOInterrupt(uint8_t vector);

    bool OnKeyboardScan(bool doUpdate, uint16_t & addr);

    void OnSetScreenSize(int cols, int rows, int lines);
    void OnSetVideoStartAddress(uint16_t addr);
    void OnSetCursorAddress(uint16_t addr);
    void OnSetCursorShape(uint8_t start, uint8_t end, int blinkRate);

    bool ScanKeyboard(uint16_t & addr);
    void RewritePCG();

  protected:  
    Z80PIO m_pio;
    Synertek6545 m_crtc;
    uint16_t m_prevKeyboardCode;
    int m_fontOffset = 0;
    int m_lines = 0;

    MicrobeeVideo * m_video; 
};

class MicrobeeDisk_Emulator : public Microbee_Emulator
{
  public:
    MicrobeeDisk_Emulator(const EmulatorInfo * info);

    bool Open(const Options & options) override;

    bool MountDrive(int driveNum, std::shared_ptr<VirtualDrive> drive, bool readOnly) override;
    void RefreshPanelDrives() override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

  protected:  
    mutable std::unique_ptr<WD_FDC> m_fdc;
    int m_drive    = 0x00;
    bool m_side    = false;
    bool m_density = false;
};


class Microbee32_Emulator : public Microbee_Emulator
{
  public:
    Microbee32_Emulator();
};


class Microbee56_Emulator : public MicrobeeDisk_Emulator
{
  public:
    Microbee56_Emulator();

  protected:  
};

class Microbee128_BaseEmulator : public MicrobeeDisk_Emulator
{
  public:
    Microbee128_BaseEmulator(const unsigned char * rom, const EmulatorInfo * info);

    bool Open(const Options & options) override;
    virtual void Reset(int addr = -1) override;

    virtual void WriteIOMemory(int id, uint16_t addr, uint8_t val) override;
    virtual uint8_t ReadIOMemory(int id, uint16_t addr) const override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    virtual void SetBankSel(uint8_t data);

  protected:
    uint8_t m_bankedMemory[4][0x8000];
    uint8_t m_bankSel = 0;
    uint8_t * m_currentLowerBank;
    bool m_romDisable   = false;
    bool m_videoDisable = false;
    bool m_videoLower   = false;
    const unsigned char * m_0x8000_ROM;
};

class Microbee128_Emulator : public Microbee128_BaseEmulator
{
  public:
    Microbee128_Emulator();
};

class Microbee128_BN_Emulator : public Microbee128_BaseEmulator
{
  public:
    Microbee128_BN_Emulator();
    Microbee128_BN_Emulator(const EmulatorInfo * info);
};

class Microbee128_StarnetClient_Emulator : public Microbee128_BN_Emulator
{
  public:
    Microbee128_StarnetClient_Emulator();

    bool Open(const Options & options) override;
    void Reset(int addr = -1) override;
    int Exec(int cycles) override;
    void Interrupt(uint16_t vector = 0) override;
    void WriteIOMemory(int id, uint16_t addr, uint8_t val) override;

    void OnPIOAWrite(uint8_t data, bool ie);
    uint8_t OnPIOARead();
    void OnPIOBWrite(uint8_t data, bool ie);

  protected:
    void WakeReceive();
    void ProbeBoot();
    void DumpDpb();

    enum class RxPhase { Sync, Length, Body };

    StarnetClient m_client;
    bool m_transmit = false;
    RxPhase m_rxPhase = RxPhase::Sync;
    uint16_t m_bodyLeft = 0;
    // Set when the LDIR at EDB0 stores 0xDF45. The next interrupt taken
    // logs the vector word the CPU fetches.
    bool m_logNextInterrupt = false;

    // One-shot cold-boot probes. Cleared on Reset.
    bool m_seenBoot = false;
    bool m_seenSetup = false;
    bool m_seenCcp = false;
    bool m_seenBiosRead = false;
    bool m_seenHalt = false;
    bool m_bootProbeDone = false;
};

#endif // DG680_H_