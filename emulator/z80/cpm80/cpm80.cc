
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "z80/z80emulator.h"
#include "z80/cpm80/cpm80.h"

#include "common/misc.h"
#include "common/binfile.h"

using namespace std;

#include <dirent.h>

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

static void PrintCPMString(const char * str);

//
//
////////////////////////////

#define ColdBootTitle "CP/M 2.2 NewBDOS\r\n$"

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

// CP/M FCB offsets
enum {
  eFCB_User = 16,
  eFCB_R0   = 33,
  eFCB_R1   = 34,
  eFCB_R2   = 35
};

////////////////////////////////////////////////////////////////

void debugOutputChar(ostream & strm, uint8_t ch)
{
  if (ch < 0x20)
    strm << "'^" << (char)(ch + 0x40) << "'";
  else
    strm << "'" << (char)ch << "'";
}

struct CPM80_Emulator;

extern unsigned char z80_cpm80_newbdos_bin[2390];

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

  m_loadFileDone = false;

  if (options.m_arg.size() > 0) {
    m_loadFile = options.m_arg[0];
    if (m_options.m_verbose)
      cerr << "info: load '" << m_loadFile << "' on startup" << endl;
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
  memcpy(m_memory + CCPB, z80_cpm80_newbdos_bin, sizeof(z80_cpm80_newbdos_bin));
}

///////////////////////////////////////////////////
//
//  New functions
//

void CPM80_Emulator::OnKeyboard(uint8_t ch)
{
  m_kbQueue.push_back(ch);
}

bool CPM80_Emulator::ConsoleStatus()
{
  return m_kbQueue.size() > 0;
//  m_terminal->Update(true);
//  usleep(10000);
#if 0
  int fd = 0;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  timeval t;
  t.tv_sec  = 0;
  t.tv_usec = 0;
  int result = select(fd+1, &fds, NULL, NULL, &t);
  return result > 0;
#endif
  return false;  
}

ofstream * g_debugStream = NULL;

void CPM80_Emulator::ConsoleOut(char data)
{
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

class CPMCOMFile : public BINFile
{
  public:
    CPMCOMFile(const std::string & fn)
      : BINFile(fn)
    { }

    virtual bool Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver)
    {
      int fd = ::open(m_fn.c_str(), O_RDONLY | O_BINARY);
      if (fd < 0) {
        cerr << "could not open file '" << m_fn << "'" << endl;
        return false;
      }

      // get size of file
      off_t len = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);

      std::vector<uint8_t> buffer;
      buffer.resize(len);

      int readLen = read(fd, &buffer[0], len);

      close(fd);
      if (readLen != len) {
        cerr << "could not read " << len << " - got " << readLen << endl;
        return false;
      }

      if (!saver(0x100, &buffer[0], len)) {
        cerr << "could not save data of length " << len << endl;
        return false;
      }

      m_execAddr = 0x100;
      m_hasExec = true;

      return true; 
    }
};


static NewBDOS::Function NewBDOSCommands[] = {
  &NewBDOS::SystemReset,    //  0 - System reset
  &NewBDOS::ConsoleInput,   //  1 - Console input
  &NewBDOS::ConsoleOutput,  //  2 - Console output
  NULL,                     //  3 - Reader input
  NULL,                     //  4 - Punch input
  NULL,                     //  5 - List output
  &NewBDOS::ConsoleDirect,  //  6 - Direct console I/O
  NULL,                     //  7 - Get IOByte
  NULL,                     //  8 - Set IOByte
  &NewBDOS::PrintString,    //  9 - Print string
  &NewBDOS::ReadLine,       // 10 - Read console buffer
  &NewBDOS::ConsoleStatus,  // 11 - Get console status
  &NewBDOS::ReturnVersion,  // 12 - Return version number
  &NewBDOS::ResetDisk,      // 13 - Reset disk system
  &NewBDOS::SelDisk,        // 14 - Select disk
  &NewBDOS::OpenFile,       // 15 - Open file
  NULL,                     // 16 - Close file
  &NewBDOS::SearchFirst,    // 17 - Search for first
  &NewBDOS::SearchNext,     // 18 - Search for next
  &NewBDOS::DeleteFile,     // 19 - Delete file
  &NewBDOS::ReadSeq,        // 20 - Read sequential
  NULL,                     // 21 - Write sequential
  NULL,                     // 22 - Create file
  NULL,                     // 23 - Rename file
  NULL,                     // 24 - Return login vector
  &NewBDOS::GetCurrDisk,    // 25 - Return current disk
  &NewBDOS::SetDMAAddress,  // 26 - Set DMA address
  NULL,                     // 27 - Get allocation vector
  NULL,                     // 28 - Write protect disk
  NULL,                     // 29 - Get read/only vector
  NULL,                     // 30 - Set file attributes
  NULL,                     // 31 - Get disk parm addr
  &NewBDOS::GetSetUser,     // 32 - Set/get user code
  &NewBDOS::ReadRandom,     // 33 - Read random
  NULL,                     // 34 - Write random
  NULL,                     // 35 - Computer file size
  NULL                      // 36 - Set random record
};

NewBDOS::NewBDOS(CPM80_Emulator & proc)
  : m_proc(proc)
  , m_fileFind(NULL)
  , m_userCode(0x00)
  , m_currDisk(0x00)
  , m_dmaAddress(0x80)
{
  m_memory = m_proc.GetMainMemoryPtr();
}

NewBDOS::~NewBDOS()
{
  if (m_fileFind != NULL)
    closedir(m_fileFind);
}


void NewBDOS::OnBDOSCommand()
{
  int code = m_proc.m_cpu.BC.B.l;
  if (code == 0xff) {
    //cerr << "BDOS " << code << "0xff" << endl;
    Boot();
    return;
  }
  if (code >= sizeof(NewBDOSCommands)/sizeof(NewBDOSCommands[0])) {
    //cerr << "BDOS " << code << ": not handled" << endl;
    m_proc.m_cpu.AF.B.h = 0;
    return;
  }

  Function func = NewBDOSCommands[code];
  if (func == NULL) {
    m_proc.m_cpu.AF.B.h = 0;
    //cerr << "BDOS " << dec << code << ": NULL handler" << endl;
  }
  else {
    //cerr << "BDOS " << dec << code << " called" << endl;
    (this->*func)();
  }
}

void NewBDOS::SystemReset()
{
  //m_proc.m_debug << "BDOS 0: system reset" << endl;
  if (m_proc.m_loadFile.empty()) {
    stringstream strm;
    strm << "CCP=" << hex << CCPB << ",BDOS=" << BDOS << ",BIOS=" << BIOS << "\r\n$";
    PrintCPMString(strm.str().c_str());
    PrintCPMString(ColdBootTitle);
  }

  // set current disk to A
  m_proc.WriteMemory(4, 0x00);

  Boot();
}

void NewBDOS::Boot()
{
  // set warm boot vector at 0x0000 to BIOS + 3
  m_memory[0x0000] = 0xc3;
  m_memory[0x0001] = (BIOS + 3) & 0xff;
  m_memory[0x0002] = ((BIOS + 3) >> 8) & 0xff;

  // set BDOS jump vector at 0x0005 to BDOS + 0
  m_memory[0x0005] = 0xc3;
  m_memory[0x0006] = BDOS & 0xff;
  m_memory[0x0007] = (BDOS >> 8) & 0xff;

  // start running CCP
  m_dmaAddress        = 0x0080;
  m_proc.m_cpu.BC.B.l = m_proc.ReadMemory(4);
  m_proc.m_cpu.SP.W   = 0x0100;
  m_proc.m_cpu.PC.W   = CCPB;

  // copy the BIOS/BDOS etc
  memcpy(m_memory + CCPB, z80_cpm80_newbdos_bin, sizeof(z80_cpm80_newbdos_bin));

  if (!m_proc.m_loadFileDone) {
    if (m_proc.m_loadFile.empty()) {
      if (m_proc.m_options.m_verbose)
        cerr << "no load file" << endl;
    }
    else {
      if (m_proc.m_options.m_verbose)
        cerr << "Trying to load '" << m_proc.m_loadFile << "'" << endl;
      BINFileIdentifier binFile;
      BINFile::AddFormat<CPMCOMFile>("com");
      if (!m_proc.LoadFile(m_proc.m_loadFile)) {
        cerr << "error: load of '" << m_proc.m_loadFile << "' failed" << endl;
      }
    }
    m_proc.m_loadFileDone = true;
  }
  else if (!m_proc.m_loadFile.empty()) {
    exit(0);
  }
}

void NewBDOS::ConsoleInput()
{
  //m_proc.m_debug << "BDOS 1: console input" << endl;
  m_proc.m_cpu.AF.B.h = m_proc.ConsoleIn();
}

void NewBDOS::ConsoleOutput()
{
  int ch = m_proc.m_cpu.DE.B.l;
  //m_proc.m_debug << "BDOS 2: console output '";
  //if (ch < 0x20)
  //  m_proc.m_debug << "^" << (char)(ch + 0x40);
  //else
  //  m_proc.m_debug << (char)ch;
  //m_proc.m_debug << "'" << endl;
  m_proc.ConsoleOut(ch);
}

void NewBDOS::ConsoleDirect()
{
  //m_proc.m_debug << "BDOS 6: direct console ";
  uint8_t ch = m_proc.m_cpu.DE.B.l;
  if (ch != 0xff) {
    //m_proc.m_debug << "output ";
    //debugOutputChar(m_proc.m_debug, ch);
    //m_proc.m_debug << endl;
    m_proc.ConsoleOut(ch);
  }
  else {
    //m_proc.m_debug << "input " << endl;
    if (!m_proc.ConsoleStatus()) {
      //m_proc.m_debug << " nothing" << endl;
      ch = 0x00;
    }
    else {
       ch = m_proc.ConsoleIn();
       //debugOutputChar(m_proc.m_debug, ch);
    }
    m_proc.m_cpu.AF.B.h = ch;
  }
}

void NewBDOS::PrintCPMString(const char * str)
{
  while (*str != '$')
    m_proc.ConsoleOut(*str++);
}

void NewBDOS::PrintString()
{
  //m_proc.m_debug << "BDOS 9: print string" << endl;
  PrintCPMString((char *)m_memory + m_proc.m_cpu.DE.W);
}

void NewBDOS::ConsoleStatus()
{
  m_proc.m_cpu.AF.B.h = m_proc.ConsoleStatus();
}

void NewBDOS::ReadLine()
{
  //m_proc.m_debug << "BDOS 10: read line" << endl;

  // get pointer to input buffer
  uint8_t * buffer = m_memory + m_proc.m_cpu.DE.W;
  int mx = buffer[0];
  uint8_t * nc   = buffer + 1;
  uint8_t * data = buffer + 2;

  uint8_t * ptr = data;

  bool done = false;
  while (!done) {
    int ch = m_proc.ConsoleIn();
    if (ch < 0) {
      m_proc.m_terminal->Update(false);
      m_proc.CheckKeyboard();
      continue;
    }

    //m_proc.m_debug << "consoleIn returned ";
    //debugOutputChar(m_proc.m_debug, ch);
    //m_proc.m_debug << endl;

    switch (ch) {
      case 'C'-0x40:
        if (ptr == data) {
          //m_proc.m_debug << "^C triggered cold boot" << endl;
          Boot();
          done = true;
        }
        break;

      case '\\'-0x40:
        //m_proc.m_sigInt = true;
        done = true;
        break;

      case 0x08:
        if (ptr > data) {
        }
        break;
      case 0x0a:
      case 0x0d:
        done = true;
        break;
      default:
        if ((ch >= 0x20) && (ch <= 0x7e)) {
          *ptr = ch;
          if ((ptr - data) < mx)
            ++ptr;
          m_proc.ConsoleOut(ch);
        }
        break;
    }
  }

  *nc = ptr - data;
}

void NewBDOS::ReturnVersion()
{
  //m_proc.m_debug << "BDOS 12: get version" << endl;
  m_proc.m_cpu.HL.B.h = 0;
  m_proc.m_cpu.HL.B.l = 0x22;

  m_proc.m_cpu.AF.B.h = m_proc.m_cpu.HL.B.l;
  m_proc.m_cpu.BC.B.l = m_proc.m_cpu.HL.B.h;
}

void NewBDOS::ResetDisk()
{
  //m_proc.m_debug << "BDOS 13: reset disk" << endl;
  m_dmaAddress = 0x0080;
  m_currDisk   = 0x00;
  m_currPath = "./";
  if (m_currDisk != 0) {
    m_currPath += (char)('a' + m_currDisk);
    m_currPath += "/";
  }
}


void NewBDOS::SelDisk()
{
  m_currDisk = m_proc.m_cpu.DE.B.l & 0x0f;
  m_currPath = "./";
  if (m_currDisk != 0) {
    m_currPath += (char)('a' + m_currDisk);
    m_currPath += "/";
  }
  //m_proc.m_debug << "BDOS 14: sel disk " << (char)('A' + m_currDisk) << endl;
}

static std::string FCBToFilename(const char * fcb)
{
  // construct possible name of file
  const char * s = fcb + 1;
  const char * p = s;
  while (isgraph(*p) && ((p-s) < 8))
    ++p;
  std::string fn(s, p-s);
  fn += ".";
  s = fcb + 9;
  p = s;
  while (isgraph(*p) && ((p-s) < 3))
    ++p;
  fn += std::string(s, p-s);

  // convert to lower case
  char * t = (char *)&fn[0];
  while (*t != '\0') {
    *t = tolower(*t);
    ++t;
  }

  return fn;
}


void NewBDOS::DeleteFile()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn = FCBToFilename(fcb);

  //m_proc.m_debug << "BDOS 19: delete file '" << fn << "'" << endl;
  m_proc.m_cpu.AF.B.h = 0xff;
}

void NewBDOS::OpenFile()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn = FCBToFilename(fcb);

  // attempt open with raw filename
  int * fd = (int *)(fcb + eFCB_User);
  std::string s = m_currPath + fn;
  *fd = open(s.c_str(), O_RDONLY);
  if (*fd < 0) {
    //m_proc.m_debug << "BDOS 15: open file '" << s << "' failed" << endl;
    m_proc.m_cpu.AF.B.h = 0xff;
    return;
  }

  // save file handle for later use
  //m_proc.m_debug << "BDOS 15: open file '" << fn << "' open returned " << *fd << endl;
  m_proc.m_cpu.AF.B.h = 0x0;
  return;
}


void NewBDOS::SearchFirst()
{
  m_findFCB = m_proc.m_cpu.DE.W;
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn = FCBToFilename(fcb);

  //m_proc.m_debug << "BDOS 17: search first '" << fn << "'" << endl;

  // close any existing open
  if (m_fileFind != NULL) {
    closedir(m_fileFind);
    m_fileFind = NULL;
  }

  // open a new file find handle
  m_fileFind = opendir(m_currPath.c_str());
  if (m_fileFind == NULL) 
    m_proc.m_cpu.AF.B.h = 0xff;
  else
    m_proc.m_cpu.AF.B.h = FindFile(*fcb, fcb+1);
}


void NewBDOS::SearchNext()
{
  char * fcb = (char *)m_memory + m_findFCB;
  std::string fn = FCBToFilename(fcb);

  //m_proc.m_debug << "BDOS 18: search next '" << fn << "'" << endl;

  // open a new file find handle
  if (m_fileFind == NULL)
    m_proc.m_cpu.AF.B.h = 0xff;
  else
    m_proc.m_cpu.AF.B.h = FindFile(*fcb, fcb+1);
}


static void ShortenFilename(const char * in, char * out)
{
  std::string str(in);

  // remove spaces
  size_t pos;
  while ((pos = str.find(' ')) != std::string::npos)
    str = str.substr(0, pos) + str.substr(pos+1);

  // commas and square brackets changed to "_"
  while ((pos = str.find('[')) != std::string::npos)
    str[pos] = '_';
  while ((pos = str.find(']')) != std::string::npos)
    str[pos] = '_';
  while ((pos = str.find(',')) != std::string::npos)
    str[pos] = ',';

  // get length of extension and base
  pos = str.rfind('.');
  size_t extPos = 0;
  size_t extLen = 0;
  if (pos == std::string::npos) {
    extLen = 0;
    pos = str.length();
  }
  else {
    extPos = pos + 1; 
    extLen = strlen(&str[extPos]);
    if (extLen > 3)
      extLen = 3;
  }
  if (pos > 8)
    pos = 8;

  // get extension and base
  memset(out, ' ', 8+3);
  strncpy(out+0, &str[0], pos);
  if (extLen > 0)
    strncpy(out+8, &str[extPos], extLen);

  // convert to upper case
  int i;
  for (i = 0; i < 8+3; ++i)
    out[i] = toupper(out[i] & 0x7f);
}


int NewBDOS::FindFile(uint8_t disk, const char * wildcard)
{
  for (;;) {

    // if end of dir, finish
    dirent * dirEnt = readdir(m_fileFind);
    if (dirEnt == NULL)
      break;

    // do not include . and ..
    char * fn = dirEnt->d_name;
    if ((strcmp(fn, ".") == 0) || (strcmp(fn, "..") == 0))
      continue;

    if (fn[0] == '.')
      continue;

    // get file attributes, ignore files that fail,
    // and ignore any non-regular files
    struct stat attr;
    std::string s = m_currPath + fn;
    if (stat(s.c_str(), &attr) != 0)
      ;
    else if ((attr.st_mode & S_IFMT) == S_IFREG) {

      // convert unix filename to 8.3 in the destination FCB
      uint8_t * outputFCB = &m_memory[m_dmaAddress];
      ShortenFilename(dirEnt->d_name, (char *)(outputFCB+1));

      // compare the wildcard and destination FCB
      int i = 0;
      for (i = 0; i < 8+3; ++i) {
        if ((wildcard[i] != '?') && (wildcard[i] != outputFCB[1+i]))
          break;
      }
      if (i != 8+3)
        continue;

      //m_proc.m_debug << dirEnt->d_name << " shortened to '" << std::string((char *)outputFCB+1, 8+3) << "'" << endl;
      outputFCB[0] = 0;
      return 0;
    }
  }
  return 0xff;
}

void NewBDOS::GetCurrDisk()
{
  //m_proc.m_debug << "BDOS 25: get current disk " << (char)('A' + m_currDisk) << endl;
  m_proc.m_cpu.AF.B.h = m_currDisk;
}

void NewBDOS::SetDMAAddress()
{
  m_dmaAddress = m_proc.m_cpu.DE.W;
  //m_proc.m_debug << "BDOS 26: set DMA address " << hex << m_dmaAddress << dec << endl;
}

void NewBDOS::GetSetUser()
{
  int code = m_proc.m_cpu.DE.B.l;
  if (code == 0xff) {
    m_proc.m_cpu.AF.B.h = m_userCode;
    //m_proc.m_debug << "BDOS 32: set user code to " << dec << (int)m_userCode << endl;
  }
  else {
    m_userCode = code & 0x1f;
    m_proc.m_cpu.AF.B.h = code;
    //m_proc.m_debug << "BDOS 32: get user code " << dec << (int)m_userCode << endl;
  }
}

void NewBDOS::ReadSeq()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  int * fd = (int *)(fcb + eFCB_User);

  uint8_t * p = m_memory + m_dmaAddress;
  int c = read(*fd, p, 128);
  if (c <= 0)
    m_proc.m_cpu.AF.B.h = 0x01;
  else {
    if (c < 128)
      memset(p + c, 0x1a, 128-c);
    m_proc.m_cpu.AF.B.h = 0x00;
  }

  //m_proc.m_debug << "BDOS 20: read from fd " << *fd << " returned " << c << ", s = " << (int)m_proc.m_cpu.AF.B.h << endl;
}

void NewBDOS::ReadRandom()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  int * fd = (int *)(fcb + eFCB_User);

  unsigned int offs = (fcb[eFCB_R0] + (fcb[eFCB_R1] << 8) + (fcb[eFCB_R2] << 16)) << 7;
  unsigned fileEnd = lseek(*fd, 0, SEEK_END);

  int c = -1;
  if (offs > fileEnd) {
    m_proc.m_cpu.AF.B.h = 0x01;
  }
  else {
    unsigned newPos = lseek(*fd, offs, SEEK_SET);
    uint8_t * p = m_memory + m_dmaAddress;
    c = read(*fd, p, 128);
    if (c <= 0)
      m_proc.m_cpu.AF.B.h = 0x01;
    else {
      if (c < 128)
        memset(p + c, 0x1a, 128-c);
      m_proc.m_cpu.AF.B.h = 0x00;
    }
  }

  //m_proc.m_debug << "BDOS 33: random read from fd " << *fd << " at " << (int)offs << " returned " << c << ", s = " << (int)m_proc.m_cpu.AF.B.h << endl;
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
      m_newBDOS->OnBDOSCommand();
      break;

    // console output
    case zed80_conout:
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