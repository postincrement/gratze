
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "z80/z80emulator.h"
#include "z80/cpm80/cpm80.h"
#include "z80/cpm80/diskdef.h"

#include "common/misc.h"
#include "common/binfile.h"

using namespace std;

#include <dirent.h>

static void PrintCPMString(const char * str);

//
//
////////////////////////////

///////////////////////////////////////////////////////////////////
//
// CP/M 80 running on a Z80
//

// ZED80 console I/O ports
#define zed80_conout		0x20
#define zed80_const		0x20
#define zed80_conin		0x21

// ZED80 new BDOS command port
#define zed80_newBDOS		0x22

// ZED80 disk drivec I/O ports
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

struct CPM80_Emulator;

extern unsigned char z80_cpm80_newbdos_bin[2398];

INFO_START(cpm80)
{
  INFO_CPU(MEM, BIOS),

  INFO_MAIN_RAM(0x0000, MEM, MEM, MEM),

  INFO_IO_PORT_RW(0x20, 0x3f, 1),

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

  m_loadFileDone = false;

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
  memcpy(m_memory + CCPB, z80_cpm80_newbdos_bin, sizeof(z80_cpm80_newbdos_bin));
}

///////////////////////////////////////////////////
//
//  New functions
//

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

ofstream * g_debugStream = NULL;

extern void DebugOutputChar(ostream & strm, uint8_t ch);

void CPM80_Emulator::ConsoleOut(char data)
{
#if 0  
  m_newBDOS->m_debug << "consoleOutput ";
  DebugOutputChar(m_newBDOS->m_debug, data);
  m_newBDOS->m_debug << endl;
#endif

  m_terminal->WriteChar(data);

#if 0  
  *g_debugStream << "consoleOutput ";
  debugOutputChar(*g_debugStream, data);
  *g_debugStream << endl;

  int x, y;
  data = data & 0x7f;
  switch (data) {
    case 0x08:
      getyx(stdscr, y, x);
      if (x > 1)
        move(y, x-1);
      break;
    case 0x0a:
      getyx(stdscr, y, x);
      move(y+1, x);
      break;
    case 0x0d:
      getyx(stdscr, y, x);
      move(y, 0);
      break;
    //case 0x0b:  // ^K cursor up
    //  addstr("\033[1A");
    //  break;
    //case 0x0c:  // ^K cursor right
    //  addstr("\033[1C");
    //  break;
    //case 0x1a:  // ^Z clear screen
    //  addstr("\033[2J");
    //  break;
    default:
      addch((char)data);
      break;
  }
#endif  
}

int CPM80_Emulator::ConsoleIn()
{
  if (m_kbQueue.size() == 0) {
    return -1;
  }

  int ch = m_kbQueue.front();
  m_kbQueue.pop_front();

  return ch;

//  m_terminal->Update(true);
//  usleep(10000);
#if 0  
  char ch;
  ch = getch();
  //if (read(0, &ch, 1) < 1)
  //  return 0xff;
  switch (ch) {
    case '\\' - 0x20:
      exit(0);

    case 0x0a:
      ch = 0x0d;
    default:
      break;
  }
  return ch;
#endif
  return 0xff;  
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

/*
  m_debug << opStr << " on " << (char)('A' + m_diskDrive)
          << " " << hex << (unsigned int)m_track << "/" << (unsigned int)m_sector << "(offset=" << offset << ")"
          << ", " << dec << (unsigned int)m_dmaLength
          << " to " << hex << (unsigned int)m_dmaAddress << dec << "   status = " << (int)m_status << endl;
  int i, j;
  char * p = 0; //(char *)(m_memory + m_dmaAddress);
  for (i = 0; i < 8; ++i) {
    const char * q  = p;
    m_debug << setfill('0');
    for (j = 0;j < 16; ++j)
      m_debug << hex << setw(2) << (int)(*q++ & 0xff) << " ";
    m_debug << "     " << setfill(' ');;
    q  = p;
    for (j = 0;j < 16; ++j) {
      char ch = *q++;
      m_debug << setw(2) << (char)(isgraph(ch) ? ch : '.');
    }
    m_debug << endl;
    p += 16;
  }
*/

  return result >= 0;
}

////////////////////////////////////////////////////////////////

uint8_t CPM80_Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t address)
{
  switch (address & 0xff) {

    // console status: return 0xff if ready, else 0
    case zed80_const:
       {
         uint8_t r = ConsoleStatus() ? 0xff : 0x00;
         //system->m_debug << "const returned 0x" << hex << (int)r << dec << endl;
         return r;
       }

    // console input: return char or wait
    case zed80_conin:
      {
        int ch = ConsoleIn();
        //system->m_debug << "conin returned 0x" << hex << (int)ch << dec << endl;
        return (ch >= 0) ? ch : 0xff;
      }

    // get current disk drive
    case zed80_drive:
      return m_diskDrive;

    // return status of last disk operation
    case zed80_disk_status:
      return m_status;
  }
  
  return 0;
}

void CPM80_Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, register uint16_t address, register uint8_t data)
{
  switch (address & 0xff) {

    // new BDOS command
    case zed80_newBDOS:
      m_newBDOS->OnBDOSCommand(data);
      break;

    // console output
    case zed80_conout:
      m_newBDOS->m_debug << "conout " << HEXFORMAT0x2(data) << endl;
      ConsoleOut(data);
      break;

    // set disk drive
    case zed80_drive:
      m_diskDrive = data;
      break;

    // set track
    case zed80_track_hi:
      m_track = (data << 8) + (m_track & 0x00ff);
      break;

    case zed80_track_lo:
      m_track = data + (m_track & 0xff00);
      break;

    // set sector
    case zed80_sector_hi:
      m_sector = (data << 8) + (m_sector & 0x00ff);
      break;
    case zed80_sector_lo:
      m_sector = data + (m_sector & 0xff00);
      break;

    // set DMA address
    case zed80_addr_hi:
      m_dmaAddress = (data << 8) + (m_dmaAddress & 0x00ff);
      break;
    case zed80_addr_lo:
      m_dmaAddress = data + (m_dmaAddress & 0xff00);
      break;

    // set DMA length
    case zed80_len_hi:
      m_dmaLength = (data << 8) + (m_dmaLength & 0x00ff);
      break;
    case zed80_len_lo:
      m_dmaLength = data + (m_dmaLength & 0xff00);
      break;

    // read/write disk
    case zed80_disk_cmd:
      DiskOp(data != 2);
      break;
  }
}

///////////////////////////////////////////////////////////////

#if 0

int main(int argc, char ** argv)
{
  CPM22 cpm22;

  if (!cpm22.Open(true))
    return -1;

  g_debugStream = &cpm22.m_debug;

  initscr();
  scrollok(stdscr,TRUE);

  raw();
  keypad(stdscr, TRUE);
  noecho();

  // set raw mode
//  termios termConfig;
//  tcgetattr(0, &termConfig);
//  cfmakeraw(&termConfig);
//  tcsetattr(0, TCSANOW, &termConfig);

  cpm22.Reset(cpm22.m_startAddress);

  while (cpm22.Run()) {
    cpm22.m_debug << "Exec " << hex << cpm22.m_cpu.PC.W << endl;
  }

  endwin();

  return 0;
}

#endif