#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "z80/z80emulator.h"
#include "z80/cpm80/cpm80.h"
#include "diskdef.h"

#include "common/misc.h"
#include "common/binfile.h"

using namespace std;

#include <dirent.h>

///////////////////////////////////////////////////////////////////
//
// CP/M 80 running on a Z80
//

// Legacy ZED80 disk I/O ports (optional block-device API)
#define zed80_drive		    0x30
#define zed80_track_hi		0x31
#define zed80_track_lo		0x32
#define zed80_sector_hi		0x33
#define zed80_sector_lo		0x34
#define zed80_addr_hi		  0x35
#define zed80_addr_lo		  0x36
#define zed80_len_hi		  0x37
#define zed80_len_lo		  0x38
#define zed80_disk_cmd		0x39
#define zed80_disk_status	0x39

////////////////////////////////////////////////////////////////

extern unsigned char z80_cpmhost_newbdos_bin[];
extern unsigned z80_cpmhost_newbdos_bin_len;

INFO_START(cpm80)
{
  INFO_CPU(MEM, BIOS),

  INFO_MAIN_RAM(0x0000, MEM, MEM, MEM),

  INFO_IO_PORT_RW(0x30, 0x3f, 1),

  INFO_TERMINAL(80, 24)
}
INFO_END(cpm80);

static EmulatorInfo g_emulatorInfo = 
{
  "cpm80",               // command line option
  "CPM 2.2 on Z80",      // short name
  "CPM 2.2 on Z80",      // long name
  false,                 // host console

  INFO_INSERT(cpm80)
};


CPM80_Emulator::CPM80_Emulator() 
  : Z80Emulator(&g_emulatorInfo) 
{ 
}

void CPM80_Emulator::Instantiate()
{  
}

bool CPM80_Emulator::Open(const Options & options)
{
  if (!Z80Emulator::Open(options))
    return false;

  std::string driveError;
  if (!CheckCpmDrives(options.m_cpmDrives, driveError)) {
    cerr << "error: " << driveError << endl;
    return false;
  }

  return true;
}

void CPM80_Emulator::Reset(int addr)
{
  Z80Emulator::Reset(addr);

  using namespace std::placeholders;
  m_terminal->SetKeyboardHandler(std::bind(&CPM80_Emulator::OnKeyboard, this, _1));

  m_diskDrive    = 0;
  m_track        = 0;
  m_sector       = 0;
  m_dmaAddress   = 0;
  m_dmaLength    = 0;
  m_status       = 0;
  m_startAddress = 0;

  m_memory = GetMainMemoryPtr();
  m_newBDOS.reset(new NewBDOS(*this));

  // copy the BIOS/BDOS etc
  memcpy(m_memory + CCPB, z80_cpmhost_newbdos_bin, z80_cpmhost_newbdos_bin_len);
}

void CPM80_Emulator::RefreshPanelDrives()
{
  m_panelDrives.clear();
  if (!m_newBDOS)
    return;

  for (int drive = 0; drive < 16; ++drive) {
    const CpmDrive & slot = m_newBDOS->m_drives[drive];
    if (!slot.m_configured && (drive != 0))
      continue;

    StatusDrive status;
    status.m_label.assign(1, (char)('A' + drive));
    auto mount = m_options.m_drives.find((unsigned)drive);
    if (mount != m_options.m_drives.end() && !mount->second.m_label.empty()) {
      status.m_name = mount->second.m_label;
    }
    else {
      std::string path = slot.m_path;
      if (path.empty())
        path = ".";
      auto slash = path.find_last_of("/\\");
      status.m_name = (slash == std::string::npos) ? path : path.substr(slash + 1);
      if (status.m_name.empty())
        status.m_name = path;
    }
    status.m_mounted = true;
    status.m_selected = (m_newBDOS->m_currDisk == drive);
    m_panelDrives.push_back(status);
  }
}

///////////////////////////////////////////////////
//
//  cpmhost::Host
//

MFZ::Z80 & CPM80_Emulator::Cpu()
{
  return m_cpu;
}

uint8_t * CPM80_Emulator::Memory()
{
  return GetMainMemoryPtr();
}

void CPM80_Emulator::RunPollers()
{
  Emulator::RunPollers();
}

void CPM80_Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  Emulator::WriteMemory(addr, data);
}

bool CPM80_Emulator::LoadFile(const std::string & path)
{
  return Emulator::LoadFile(path);
}

const Options & CPM80_Emulator::GetOptions() const
{
  return m_options;
}

void CPM80_Emulator::OnPatchZ80(MFZ::Z80 *R)
{
  if (m_newBDOS)
    m_newBDOS->OnTrap(R->BC.B.l);
}

void CPM80_Emulator::OnKeyboard(uint8_t ch)
{
  if (ch == 0x0a)
    ch = 0x0d;
  m_kbQueue.push_back(ch);
}

bool CPM80_Emulator::ConsoleStatus()
{
  return m_kbQueue.size() > 0;
}

void CPM80_Emulator::ConsoleOut(char data)
{
  m_terminal->WriteChar(data);
}

int CPM80_Emulator::ConsoleIn()
{
  if (m_kbQueue.size() == 0) {
    return -1;
  }

  int ch = m_kbQueue.front();
  m_kbQueue.pop_front();

  return ch;
}

bool CPM80_Emulator::Mount(const char * fn, int drive)
{
  m_driveFiles[drive] = open(fn, O_RDWR);
  if (m_driveFiles[drive] < 0) {
    cerr << "error: cannot mount '" << fn << "' as drive " << (char)('A' + drive) << endl;
    return false;
  }
  return true;
}

bool CPM80_Emulator::DiskOp(int op)
{
  if (m_diskDrive > 2)
    return false;

  off_t offset = ((m_track * 26) + m_sector-1) * 128;

  int fd = m_driveFiles[m_diskDrive];
  int result = lseek(fd, offset, SEEK_SET);
  const char * opStr = "(unknown)";

  if (result >= 0) {
    switch (op) {

      // read
      case 1:
        opStr = "read";
        result = write(fd, m_memory + m_dmaAddress, m_dmaLength);
        if (result > 0)
          result = (result == m_dmaLength) ? 0 : 1;
        break;

      // write
      case 2:
        opStr = "write";
        result = read(fd, m_memory + m_dmaAddress, m_dmaLength);
        if (result > 0)
          result = (result == m_dmaLength) ? 0 : 1;
        break;

      // unknown
      default:
        opStr = "unknown";
        result = -1;
        break;
    }
  }

  if (result == 0) 
    m_status = 0;
  else
    m_status = 1;

  (void)opStr;
  return result >= 0;
}

////////////////////////////////////////////////////////////////

uint8_t CPM80_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t address)
{
  switch (address & 0xff) {

    case zed80_drive:
      return m_diskDrive;

    case zed80_disk_status:
      return m_status;
  }
  
  return 0;
}

void CPM80_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, register uint16_t address, register uint8_t data)
{
  switch (address & 0xff) {

    case zed80_drive:
      m_diskDrive = data;
      break;

    case zed80_track_hi:
      m_track = (data << 8) + (m_track & 0x00ff);
      break;

    case zed80_track_lo:
      m_track = data + (m_track & 0xff00);
      break;

    case zed80_sector_hi:
      m_sector = (data << 8) + (m_sector & 0x00ff);
      break;
    case zed80_sector_lo:
      m_sector = data + (m_sector & 0xff00);
      break;

    case zed80_addr_hi:
      m_dmaAddress = (data << 8) + (m_dmaAddress & 0x00ff);
      break;
    case zed80_addr_lo:
      m_dmaAddress = data + (m_dmaAddress & 0xff00);
      break;

    case zed80_len_hi:
      m_dmaLength = (data << 8) + (m_dmaLength & 0x00ff);
      break;
    case zed80_len_lo:
      m_dmaLength = data + (m_dmaLength & 0xff00);
      break;

    case zed80_disk_cmd:
      DiskOp(data != 2);
      break;
  }
}
