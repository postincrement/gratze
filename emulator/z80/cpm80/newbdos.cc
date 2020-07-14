#include "common/misc.h"

#include "z80/cpm80/cpm80.h"
#include "z80/cpm80/newbdos.h"

using namespace std;

// CP/M FCB offsets
enum {
  eFCB_User = 16,
  eFCB_R0   = 33,
  eFCB_R1   = 34,
  eFCB_R2   = 35
};

#define ColdBootTitle "CP/M 2.2 NewBDOS\r\n$"

extern unsigned char z80_cpm80_newbdos_bin[2390];

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

////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////

static void DebugOutputChar(ostream & strm, uint8_t ch)
{
  if (ch < 0x20)
    strm << "'^" << (char)(ch + 0x40) << "'";
  else
    strm << "'" << (char)ch << "'";
}

static std::string ShortenFilename(const std::string & in)
{
  std::string str(in);

  // convert to upper case
  for (auto & r : str)
    r = toupper(r);

  // commas and square brackets changed to "_"
  size_t pos = 0;
  while ((pos = str.find('[')) != std::string::npos)
    str[pos] = '_';
  while ((pos = str.find(']')) != std::string::npos)
    str[pos] = '_';
  while ((pos = str.find(',')) != std::string::npos)
    str[pos] = ',';

  // get length of extension and base
  pos = str.rfind('.');
  std::string extension;
  if (pos == std::string::npos) {
    pos = str.length();
  }
  else {
    size_t extPos = pos + 1; 
    size_t extLen = strlen(&str[extPos]);
    if (extLen > 3)
      extLen = 3;
    extension = str.substr(extPos, extLen);  
  }

  if (pos > 8)
    pos = 8;
  str = str.substr(0, pos);
  while (str.length() < 8)
    str += ' ';
  while (extension.length() < 3)
    extension += ' ';  
  str += extension;

  return str;
}

void NewBDOS::UpdateDriveInfo(int drive)
{  
  // create path for drive
  // A    = ./
  // else = ./drive

  std::string path = "./";
  if (drive != 0) {
    path += (char)('a' + drive);
    path += "/";
  }

  // create a map for the new drive
  DriveInfo & driveInfo = m_driveInfoMap[drive];
  driveInfo.m_cpmToNative.clear();
  driveInfo.m_nativeToCPM.clear();
  driveInfo.m_dir = path;

  // open a new file find handle
  DIR * dir = opendir(path.c_str());
  if (dir == NULL)
    return; 

  for (;;) {

    // if end of dir, finish
    dirent * dirEnt = readdir(dir);
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
    std::string s = path + fn;
    if (stat(s.c_str(), &attr) != 0)
      continue;

    if ((attr.st_mode & S_IFMT) != S_IFREG)
      continue;

    // convert native filename to 8.3
    std::string cpmFilename = ShortenFilename(dirEnt->d_name);

    // disambiguate
    int index = 1;
    while (driveInfo.m_cpmToNative.count(cpmFilename) > 0) {
      if (index < 10)
        cpmFilename[7] = '0' + index;
      else if (index < 100) {
        cpmFilename[7] = '0' + (index % 10);
        cpmFilename[6] = '0' + (index / 10);
      }  
      else if (index < 1000) {
        cpmFilename[7] = '0' + (index % 10);
        cpmFilename[6] = '0' + ((index / 10) % 10);
        cpmFilename[5] = '0' + (index / 100);
      }  
      else {
        break;
      }
    }

    if (index >= 1000) {
      m_debug << "warning: exhausted suffixes to disambiguate '" << dirEnt->d_name << "'" << endl;
      continue;
    }

    driveInfo.m_cpmToNative[cpmFilename]    = dirEnt->d_name;
    driveInfo.m_nativeToCPM[dirEnt->d_name] = cpmFilename;

    m_debug << "native '" << dirEnt->d_name << "' mapped to CP/M '" << cpmFilename << "'" << endl;
  }
}

std::string NewBDOS::FCBToRegex(const char * fcb)
{
  // construct possible name of file
  const char * s = fcb + 1;
  std::string fn;
  for (int i = 0; i < 8+3; ++i) {
    char ch = *s++ & 0x7f;
    fn += toupper(ch);
  }

  return fn;
}

bool NewBDOS::FCBToFilename(std::string & fn, const char * fcb)
{
  // get drive number
  int drive = fcb[0] & 0xf;
  if (drive == 0)
    drive == m_currDisk;
  else
    --drive;
    
  if (m_driveInfoMap.count(drive) == 0)
    UpdateDriveInfo(drive);

  // construct possible name of file
  const char * s = fcb + 1;
  fn.clear();
  for (int i = 0; i < 8+3; ++i) {
    char ch = *s++ & 0x7f;
    if (isprint(ch)) {
      fn += toupper(ch);
    }
  }

  // see if maps to a real file
  DriveInfo & driveInfo = m_driveInfoMap[drive];
  if (driveInfo.m_cpmToNative.count(fn) == 0)
    return false;

  fn = driveInfo.m_dir + driveInfo.m_cpmToNative[fn];
  return true;
}

//////////////////////////////////////////////////////////////

NewBDOS::NewBDOS(CPM80_Emulator & proc)
  : m_proc(proc)
  , m_fileFind(NULL)
  , m_userCode(0x00)
  , m_currDisk(0x00)
  , m_dmaAddress(0x80)
{
  m_memory = m_proc.GetMainMemoryPtr();

  m_debug.open("bdos_debug.txt", std::ofstream::out | std::ofstream::trunc);
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
  m_debug << "BDOS 0: system reset" << endl;
  if (m_proc.m_options.m_arg.empty()) {
    stringstream strm;
    strm << "CCP=" << hex << CCPB << ",BDOS=" << BDOS << ",BIOS=" << BIOS << "\r\n$";
    PrintCPMString(strm.str().c_str());
    PrintCPMString(ColdBootTitle);
  }

  m_driveInfoMap.clear();

  // set current disk to A
  m_proc.WriteMemory(4, 0x00);

  Boot();
}

void NewBDOS::Boot()
{
  memset(m_memory, 0, 0x100);

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

  // if no load file, nothing to do
  if (m_proc.m_options.m_arg.empty()) {
    return;
  }

  // if we already loaded the file, exit
  if (m_proc.m_loadFileDone)
    exit(0);

  std::string loadFile = m_proc.m_options.m_arg[0];

  BINFileIdentifier binFile;
  BINFile::AddFormat<CPMCOMFile>("com");

  // note this sets the PC if it loads
  if (!m_proc.LoadFile(loadFile)) {
    cerr << "error: load of '" << loadFile << "' failed" << endl;
    return;
  }

  int fcbOffs = 0x5c;
  for (int pass = 1; pass < 3; ++pass) {
    if (pass >= m_proc.m_options.m_arg.size())
      break;  
    Filename fn(m_proc.m_options.m_arg[pass]);

    BINFileIdentifier binFile;
    BINFile::AddFormat<CPMCOMFile>("com");

    std::string filename = fn.GetFilename();
    if (m_driveInfoMap.count(0) == 0)
      UpdateDriveInfo(0);
    DriveInfo & driveA = m_driveInfoMap[0];
    auto r = driveA.m_nativeToCPM.find(filename);
    std::string cpm;
    if (r != driveA.m_nativeToCPM.end()) {
      cpm = r->second;
    }
    else {
      cpm = ShortenFilename(filename);
      cerr << "warning: filename '" << loadFile << "' does not map to native file - using '" << cpm << "'" << endl;
    }
    m_memory[fcbOffs] = 1;
    memcpy(m_memory+fcbOffs+1, cpm.c_str(), 8+3);
    fcbOffs += 16;
  }

  std::string cmdLine;
  std::string prefix;
  for (int i = 1; i < m_proc.m_options.m_arg.size(); ++i) {
    std::string str = cmdLine + prefix + m_proc.m_options.m_arg[i];
    if (cmdLine.length() > 64)
      break;
    cmdLine = str;  
    prefix = ' ';
  }

  if (cmdLine.length() > 0) {
    m_memory[0x80] = cmdLine.length();
    memcpy(m_memory+0x81, cmdLine.c_str(), cmdLine.length());
  }

  m_proc.m_loadFileDone = true;
}

void NewBDOS::ConsoleInput()
{
  m_debug << "BDOS 1: console input" << endl;
  m_proc.m_cpu.AF.B.h = m_proc.ConsoleIn();
}

void NewBDOS::ConsoleOutput()
{
  int ch = m_proc.m_cpu.DE.B.l;
/*
  m_debug << "BDOS 2: console output '";
  if (ch < 0x20)
    m_debug << "^" << (char)(ch + 0x40);
  else
    m_debug << (char)ch;
  m_debug << "'" << endl;
*/  
  m_proc.ConsoleOut(ch);
}

void NewBDOS::ConsoleDirect()
{
  m_debug << "BDOS 6: direct console ";
  uint8_t ch = m_proc.m_cpu.DE.B.l;
  if (ch != 0xff) {
    m_debug << "output ";
    DebugOutputChar(m_debug, ch);
    m_debug << endl;
    m_proc.ConsoleOut(ch);
  }
  else {
    m_debug << "input " << endl;
    if (!m_proc.ConsoleStatus()) {
      m_debug << " nothing" << endl;
      ch = 0x00;
    }
    else {
       ch = m_proc.ConsoleIn();
       DebugOutputChar(m_debug, ch);
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
  m_debug << "BDOS 9: print string" << endl;
  PrintCPMString((char *)m_memory + m_proc.m_cpu.DE.W);
}

void NewBDOS::ConsoleStatus()
{
  m_proc.m_cpu.AF.B.h = m_proc.ConsoleStatus();
}

void NewBDOS::ReadLine()
{
  m_debug << "BDOS 10: read line" << endl;

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
      m_proc.RunPollers();
      continue;
    }

    switch (ch) {
      case 'C'-0x40:
        if (ptr == data) {
          m_debug << "^C triggered cold boot" << endl;
          Boot();
          done = true;
        }
        break;

      case '\\'-0x40:
        //m_sigInt = true;
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
  m_debug << "BDOS 12: get version" << endl;
  m_proc.m_cpu.HL.B.h = 0;
  m_proc.m_cpu.HL.B.l = 0x22;

  m_proc.m_cpu.AF.B.h = m_proc.m_cpu.HL.B.l;
  m_proc.m_cpu.BC.B.l = m_proc.m_cpu.HL.B.h;
}

void NewBDOS::ResetDisk()
{
  m_debug << "BDOS 13: reset disk" << endl;
  m_dmaAddress = 0x0080;
  m_currDisk   = 0x00;
  m_driveInfoMap.clear();
}

void NewBDOS::SelDisk()
{
  m_currDisk = m_proc.m_cpu.DE.B.l & 0x0f;
  m_debug << "BDOS 14: sel disk " << (char)('A' + m_currDisk) << endl;
  UpdateDriveInfo(m_currDisk);
}

void NewBDOS::DeleteFile()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn;
  bool result = FCBToFilename(fn, fcb);
  m_debug << "BDOS 19: delete file '" << fn << "'" << endl;

  if (!result)
    m_proc.m_cpu.AF.B.h = 0xff;

  m_proc.m_cpu.AF.B.h = 0xff;
}

void NewBDOS::OpenFile()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn;
  if (!FCBToFilename(fn, fcb)) {
    m_debug << "BDOS 15: open file '" << fn << "' not found" << endl;
    m_proc.m_cpu.AF.B.h = 0xff;
    return;
  }

  // attempt open with raw filename
  int * fd = (int *)(fcb + eFCB_User);
  *fd = open(fn.c_str(), O_RDONLY | O_BINARY);
  if (*fd < 0) {
    m_debug << "BDOS 15: open file '" << fn << "' failed" << endl;
    m_proc.m_cpu.AF.B.h = 0xff;
    return;
  }

  // save file handle for later use
  m_debug << "BDOS 15: open file '" << fn << "' open using fd " << *fd << endl;
  m_proc.m_cpu.AF.B.h = 0x0;
  return;
}

void NewBDOS::SearchFirst()
{
  m_findFCB = m_proc.m_cpu.DE.W;
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  std::string fn = FCBToRegex(fcb);

  m_debug << "BDOS 17: search first '" << fn << "'" << endl;

  m_findIndex = 0;
  m_proc.m_cpu.AF.B.h = FindFile(fcb[0], fn);
}

void NewBDOS::SearchNext()
{
  char * fcb = (char *)m_memory + m_findFCB;
  std::string fn = FCBToRegex(fcb);
  m_debug << "BDOS 18: search next '" << fn << "'" << endl;
  m_proc.m_cpu.AF.B.h = FindFile(fcb[0], fn);
}

uint8_t NewBDOS::FindFile(int disk, const std::string & regex)
{
  if (m_driveInfoMap.count(disk) == 0)
    UpdateDriveInfo(m_currDisk);

  DriveInfo & driveInfo = m_driveInfoMap[m_currDisk];

  uint8_t * outputFCB = &m_memory[m_dmaAddress];

  int i = 0;
  StringMap::iterator r = driveInfo.m_cpmToNative.begin();  
  while (r != driveInfo.m_cpmToNative.end()) {
    if (i <= m_findIndex) {
      ++i;
      ++r;
      continue;
    }

    m_findIndex++;

    // compare the wildcard and destination FCB
    int j = 0;
    for (j = 0; j < 8+3; ++j) {
      if ((regex[j] != '?') && (regex[j] != outputFCB[j+i]))
        break;
    }
    if (j != 8+3) {
      m_debug << r->first << " did not match '" << regex << "'" << endl;
      ++r;
      continue;
    }

    m_debug << r->first << " matches '" << regex << "'" << endl;
    outputFCB[0] = 0;
    memcpy(outputFCB+1, &r->first[0], 8+3);

    return 0;
  }

  m_debug << "no more matches to '" << regex << "'" << endl;

  return 0xff;
}

void NewBDOS::GetCurrDisk()
{
  m_debug << "BDOS 25: get current disk " << (char)('A' + m_currDisk) << endl;
  m_proc.m_cpu.AF.B.h = m_currDisk;
}

void NewBDOS::SetDMAAddress()
{
  m_dmaAddress = m_proc.m_cpu.DE.W;
  m_debug << "BDOS 26: set DMA address " << hex << m_dmaAddress << dec << endl;
}

void NewBDOS::GetSetUser()
{
  int code = m_proc.m_cpu.DE.B.l;
  if (code == 0xff) {
    m_proc.m_cpu.AF.B.h = m_userCode;
    m_debug << "BDOS 32: set user code to " << dec << (int)m_userCode << endl;
  }
  else {
    m_userCode = code & 0x1f;
    m_proc.m_cpu.AF.B.h = code;
    m_debug << "BDOS 32: get user code " << dec << (int)m_userCode << endl;
  }
}

void NewBDOS::ReadSeq()
{
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  int * fd = (int *)(fcb + eFCB_User);

  uint8_t * p = m_memory + m_dmaAddress;
  int c = read(*fd, p, 128);
  if (c <= 0) {
    m_debug << "read error " << strerror(errno) << endl;
    m_proc.m_cpu.AF.B.h = 0x01;
  }
  else {
    if (c < 128)
      memset(p + c, 0x1a, 128-c);
    c = 128;
    m_proc.m_cpu.AF.B.h = 0x00;
  }

  m_debug << "BDOS 20: read from fd " << *fd << " returned " << c << ", s = " << (int)m_proc.m_cpu.AF.B.h << endl;
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
      c = 128;
    }
  }

  m_debug << "BDOS 33: random read from fd " << *fd << " at " << (int)offs << " returned " << c << ", s = " << (int)m_proc.m_cpu.AF.B.h << endl;
}
