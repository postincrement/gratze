#ifndef CPM80_H_
#define CPM80_H_

#include <deque>
#include <memory>

#include "common/config.h"
#include "common/misc.h"
#include "common/binfile.h"
#include "z80/z80emulator.h"
#include "z80/cpmhost/cpmhost.h"
#include "z80/cpmhost/newbdos.h"
#include "src/options.h"

class CPM80_Emulator : public Z80Emulator, public cpmhost::Host
{
  public:
    CPM80_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;

    virtual void Reset(int addr = -1) override;
    void RefreshPanelDrives() override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    // cpmhost::Host
    MFZ::Z80 & Cpu() override;
    uint8_t * Memory() override;
    void ConsoleOut(char data) override;
    int ConsoleIn() override;
    bool ConsoleStatus() override;
    void RunPollers() override;
    void WriteMemory(uint16_t addr, uint8_t data) override;
    bool LoadFile(const std::string & path) override;
    const Options & GetOptions() const override;

    virtual void OnPatchZ80(MFZ::Z80 *R) override;

    // legacy disk port helpers (0x30-0x39)
    bool Mount(const char * fn, int drive);
    bool DiskOp(int op);

    void OnKeyboard(uint8_t ch);

  protected:
    std::unique_ptr<NewBDOS> m_newBDOS;
    uint8_t m_diskDrive;
    uint16_t m_track;
    uint16_t m_sector;
    uint16_t m_dmaAddress;
    uint16_t m_dmaLength;
    uint8_t m_status;
    uint16_t m_startAddress;
    int m_driveFiles[4];
    uint8_t * m_memory;
    std::deque<uint8_t> m_kbQueue;
};

#endif // CPM80_H_
