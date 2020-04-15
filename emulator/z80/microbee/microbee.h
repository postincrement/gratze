#ifndef MICROBEE_H_
#define MICROBEE_H_

#include "src/config.h"
#include "z80/z80emulator.h"
#include "src/options.h"
#include "devices/z80pio.h"
#include "devices/keyscan.h"
#include "devices/synertek6545.h"

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

    virtual void WriteIOMemory(int id, uint16_t addr, uint8_t val);
    virtual uint8_t ReadIOMemory(int id, uint16_t addr) const;

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
    std::vector<uint8_t> m_pcgRAM;
};


class Microbee32_Emulator : public Microbee_Emulator
{
  public:
    Microbee32_Emulator();
};


class Microbee56_Emulator : public Microbee_Emulator
{
  public:
    Microbee56_Emulator();

    bool Open(const Options & options);

    bool MountDrive(int driveNum, VirtualDrive *drive, bool readOnly);

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

  protected:  
    mutable std::unique_ptr<WD_FDC> m_fdc;
    int m_drive    = 0x00;
    bool m_side    = false;
    bool m_density = false;
    bool m_charROMEnabled = false;
};


#endif // DG680_H_