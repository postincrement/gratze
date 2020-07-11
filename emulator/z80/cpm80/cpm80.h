#ifndef CPM80_H_
#define CPM80_H_

#include "common/config.h"
#include "common/misc.h"
#include "common/binfile.h"
#include "z80/z80emulator.h"
#include "z80/cpm80/newbdos.h"
#include "src/options.h"

////////////////////////////
//
// must match defines in cmp80.cpp, newbdos.asm, and newbios.asm
//

#define	MEM	    64
#define	CCPLEN	0x800
#define	BDOSLEN	0x100
#define	BIOSLEN	0x100

#define	MEM_TOP	(MEM * 1024)

#define CCPB	(MEM_TOP - BIOSLEN - BDOSLEN - CCPLEN)
#define	BDOS	(MEM_TOP - BIOSLEN - BDOSLEN)
#define	BIOS	(MEM_TOP - BIOSLEN)



class CPM80_Emulator : public Z80Emulator
{
  public:
    CPM80_Emulator();

    void Instantiate() override;

    virtual bool Open(const Options & options) override;

    virtual void Reset(int addr = -1) override;

    virtual uint8_t ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t) override;
    virtual void WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t, uint8_t data) override;

    // new functions
    bool Mount(const char * fn, int drive);
    bool DiskOp(int op);

    void OnKeyboard(uint8_t ch);

    void ConsoleOut(char data);
    bool ConsoleStatus();
    int ConsoleIn();

    friend class NewBDOS;

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
    std::string m_loadFile;
    bool m_loadFileDone = false;
    std::deque<uint8_t> m_kbQueue;
};

#endif // CPM80_H_