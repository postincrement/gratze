#include <limits.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>

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

extern unsigned char z80_cpm80_newbdos_bin[2394];

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
  &NewBDOS::CloseFile,      // 16 - Close file
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
  &NewBDOS::GetDiskParams,  // 31 - Get disk parm addr
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
        //m_debug << "could not open file '" << m_fn << "'" << endl;
        return false;
      }

      // get length
      struct stat stbuf;
      if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        //m_debug << "error: '" << m_fn << "' inaccessible or not a regular file" << endl;
        return false;
      }  
      off_t len = stbuf.st_size;  

      // read file
      std::vector<uint8_t> buffer;
      buffer.resize(len);

      int readLen = read(fd, &buffer[0], len);

      close(fd);
      if (readLen != len) {
        //m_debug << "could not read " << len << " - got " << readLen << endl;
        return false;
      }

      if (!saver(0x100, &buffer[0], len)) {
        //m_debug << "could not save data of length " << len << endl;
        return false;
      }

      m_execAddr = 0x100;
      m_hasExec = true;

      return true; 
    }
};

//////////////////////////////////////////////////////////////

void DebugOutputChar(ostream & strm, uint8_t ch)
{
  if (ch < 0x20)
    strm << "'^" << (char)(ch + 0x40) << "'";
  else
    strm << "'" << (char)ch << "'";
}

static void Replace(std::string & str, char from, char to)
{
  size_t pos = 0;
  while ((pos = str.find(from)) != std::string::npos)
    str[pos] = to;
}

static std::string ShortenFilename(const std::string & in)
{
  std::string str(in);

  // convert to upper case
  for (auto & r : str)
    r = toupper(r);

  // commas and square brackets changed to "_"
  Replace(str, '[', '_');
  Replace(str, ']', '_');
  Replace(str, ',', '_');

  // get length of extension and base
  size_t pos = str.rfind('.');
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
  Replace(str, '.', '_');
  while (extension.length() < 3)
    extension += ' ';  
  Replace(extension, '.', '_');
  str += extension;

  return str;
}

void NewBDOS::UpdateDriveInfo(int drive)
{
  DriveSlot & slot = m_drives[drive];
  if (slot.m_kind == DriveSlot::Kind::eImage)
    return;

  // A configured host drive uses its own directory. Otherwise A is the
  // current directory and every other letter is ./ plus that letter.
  std::string root = "./";
  bool appendLetter = drive != 0;
  if (slot.m_configured) {
    root = slot.m_path;
    appendLetter = false;
  }

  char * canonicalPath;

#if __linux__ || __APPLE__ 
  canonicalPath = realpath(root.c_str(), NULL);
#endif
#if __WIN32
  canonicalPath  = _fullpath(NULL, root.c_str(), 0);
#endif

  if (canonicalPath == NULL) {
    m_debug << "cannot get real path for '" << root << "'" << endl;
    return;
  }

  m_debug << "'" << root << "' resolved to '" << canonicalPath << "'" << endl;
  std::string path(canonicalPath);
  free(canonicalPath);

  size_t len = path.length();
  if ((len > 0) && (path[len-1] != DIR_SEPERATOR))
    path += DIR_SEPERATOR;

  if (appendLetter) {
    path += (char)('a' + drive);
    path += "/";
  }

  // create a map for the new drive
  DriveInfo & driveInfo = slot.m_host;
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
    if (stat(s.c_str(), &attr) != 0) {
      m_debug << "error: stat error " << strerror(errno) << " - " << s << endl;
      continue;
    }

    if ((attr.st_mode & S_IFMT) != S_IFREG) {
      //m_debug << "not a regular file: " << s << endl;
      continue;
    }

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
      ++index;
    }

    if (index >= 1000) {
      m_debug << "warning: exhausted suffixes to disambiguate '" << dirEnt->d_name << "'" << endl;
      continue;
    }

    driveInfo.m_cpmToNative[cpmFilename]    = dirEnt->d_name;
    driveInfo.m_nativeToCPM[dirEnt->d_name] = cpmFilename;

    m_debug << "native '" << dirEnt->d_name << "' mapped to CP/M '" << cpmFilename << "'" << endl;
  }

  m_debug << "finished mapping" << endl;
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
    drive = m_currDisk;
  else
    --drive;
    
  if (m_drives[drive].m_kind != DriveSlot::Kind::eHost)
    return false;
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
  DriveInfo & driveInfo = m_drives[drive].m_host;
  if (driveInfo.m_cpmToNative.count(fn) == 0)
    return false;

  fn = driveInfo.m_dir + driveInfo.m_cpmToNative[fn];
  return true;
}

//////////////////////////////////////////////////////////////

NewBDOS::FileInfo::FileInfo()
  : m_fd(-1)
{}

NewBDOS::FileInfo::~FileInfo()
{ 
  if (m_fd >= 0) {
    ::close(m_fd);
    //m_debug << "closed " << m_fd << endl;
  }
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
  m_nextFileId = 0x40000000;

  m_debug.open("bdos_debug.txt", std::ofstream::out | std::ofstream::trunc);

  if (!ConfigureDrives())
    exit(1);
}

NewBDOS::~NewBDOS()
{
  if (m_fileFind != NULL)
    closedir(m_fileFind);
  for (auto & slot : m_drives) {
    if (slot.m_fd >= 0) {
      ::close(slot.m_fd);
      slot.m_fd = -1;
    }
  }
}

void NewBDOS::OnBDOSCommand()
{
  int code = m_proc.m_cpu.BC.B.l;
  if (code == 0xff) {
    //m_debug << "BDOS " << code << "0xff" << endl;
    Boot();
    return;
  }
  if (code >= sizeof(NewBDOSCommands)/sizeof(NewBDOSCommands[0])) {
    m_debug << "BDOS " << code << ": not handled" << endl;
    m_proc.m_cpu.AF.B.h = 0;
    return;
  }

  Function func = NewBDOSCommands[code];
  if (func == NULL) {
    m_proc.m_cpu.AF.B.h = 0;
    m_debug << "BDOS " << dec << code << ": NULL handler" << endl;
  }
  else {
    m_debug << "BDOS " << dec << code << " called" << endl;
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
    PrintDriveMap();
    PrintCPMString(ColdBootTitle);
  }

  ClearHostCaches();

  // set current disk to A
  m_proc.WriteMemory(4, 0x00);

  Boot();
}

void NewBDOS::Boot()
{
  m_debug << "booting" << endl;

  m_fileMap.clear();

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
    m_debug << "no file to load" << endl;
    return;
  }

  // if we already loaded the file, exit
  if (m_proc.m_loadFileDone) {
    m_debug << "exiting" << endl;
    exit(0);
  }

  std::string loadFile = m_proc.m_options.m_arg[0];

  m_debug << "load file = " << loadFile << endl;

  BINFileIdentifier binFile;
  BINFile::AddFormat<CPMCOMFile>("com");

  // note this sets the PC if it loads
  if (!m_proc.LoadFile(loadFile)) {
    m_debug << "error: load of '" << loadFile << "' failed" << endl;
    return;
  }

  m_debug << "loaded '" << loadFile << "'" << endl;

  std::vector<std::string> args;

  int fcbOffs = 0x5c;
  for (int pass = 1; pass < 3; ++pass) {
    if (pass >= m_proc.m_options.m_arg.size())
      break;

    m_debug << "info: arg " << pass << " " << m_proc.m_options.m_arg[pass] << endl;

    Filename fn(m_proc.m_options.m_arg[pass]);

    BINFileIdentifier binFile;
    BINFile::AddFormat<CPMCOMFile>("com");

    std::string filename = fn.GetFilename();
    std::string cpm;
    if (m_drives[0].m_kind == DriveSlot::Kind::eHost) {
      UpdateDriveInfo(0);
      auto r = m_drives[0].m_host.m_nativeToCPM.find(filename);
      if (r != m_drives[0].m_host.m_nativeToCPM.end())
        cpm = r->second;
    }
    if (cpm.empty()) {
      cpm = ShortenFilename(filename);
      m_debug << "warning: filename '" << filename << "' does not map to native file - using '" << cpm << "'" << endl;
    }
    m_memory[fcbOffs] = 1;
    memcpy(m_memory+fcbOffs+1, cpm.c_str(), 8+3);
    fcbOffs += 16;

    std::string argName(cpm.substr(0, 8));
    while ((argName.length() > 1) && isspace(argName[argName.length()-1]))
      argName = argName.substr(0, argName.length()-1);
    argName += ".";
    argName += cpm.substr(8);

    args.push_back(argName);
  }

  std::string cmdLine;
  std::string prefix;
  for (int i = 1; i < m_proc.m_options.m_arg.size(); ++i) {
    std::string str = cmdLine + prefix;
    if ((i-1) < args.size())
      str += args[i-1];
    else  
      str += m_proc.m_options.m_arg[i];
    if (str.length() > 64)
      break;
    cmdLine = str;  
    prefix = ' ';
  }

  if (cmdLine.length() > 0) {
    m_memory[0x80] = cmdLine.length();
    memcpy(m_memory+0x81, cmdLine.c_str(), cmdLine.length());
  }

  m_debug << "info: cmdline = " << cmdLine << endl;

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
  char * ptr = (char *)m_memory + m_proc.m_cpu.DE.W;
  m_debug << "output string: ";
  for (int i = 0; ptr[i] != '$'; ++i)
    DebugOutputChar(m_debug, ptr[i]);
  m_debug << endl;

  PrintCPMString(ptr);
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
        // ^C on an empty command line is the CP/M warm boot.
        if (ptr == data) {
          m_debug << "^C triggered warm boot" << endl;
          m_proc.ConsoleOut('^');
          m_proc.ConsoleOut('C');
          m_proc.ConsoleOut(0x0d);
          m_proc.ConsoleOut(0x0a);
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
          --ptr;
          m_proc.ConsoleOut(0x08);
          m_proc.ConsoleOut(' ');
          m_proc.ConsoleOut(0x08);
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
  m_findIndex  = 0;
  ClearHostCaches();
}

void NewBDOS::SelDisk()
{
  m_currDisk = m_proc.m_cpu.DE.B.l & 0x0f;
  m_debug << "BDOS 14: sel disk " << (char)('A' + m_currDisk) << endl;
  m_proc.WriteMemory(4, m_currDisk);
  if (m_drives[m_currDisk].m_kind == DriveSlot::Kind::eImage) {
    PublishDPB(m_currDisk);
  }
  else {
    UpdateDriveInfo(m_currDisk);
    m_proc.m_cpu.HL.W = 0;
    m_proc.m_cpu.AF.B.h = 0;
  }
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

bool IsATextFile(const Filename & fn)
{
  return (fn.GetExtension() == ".bas");
}

void NewBDOS::OpenFile()
{
  m_proc.m_cpu.AF.B.h = 0xff;

  uint8_t * fcbBytes = m_memory + m_proc.m_cpu.DE.W;
  int drive = DriveFromFCB(fcbBytes);
  if (m_drives[drive].m_kind == DriveSlot::Kind::eImage) {
    if (OpenImage(drive, fcbBytes))
      m_proc.m_cpu.AF.B.h = 0;
    else
      m_debug << "BDOS 15: image file not found" << endl;
    return;
  }

  char * fcb = (char *)fcbBytes;
  std::string fn;
  if (!FCBToFilename(fn, fcb)) {
    m_debug << "BDOS 15: open file '" << fn << "' not found" << endl;
    return;
  }

  // attempt open with raw filename
  FileInfo fileInfo;
  fileInfo.m_fn = fn;
  fileInfo.m_fd = open(fn.c_str(), O_RDONLY | O_BINARY);
  if (fileInfo.m_fd < 0) {
    m_debug << "BDOS 15: open file '" << fn << "' failed" << endl;
    return;
  }

  struct stat stbuf;
  if ((fstat(fileInfo.m_fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    m_debug << "error: '" << fn << "' inaccessible or not a regular file" << endl;
    return;
  }
  fileInfo.m_len = stbuf.st_size;

  // if file is a text file, convert newlines
  if (IsATextFile(fn)) {

    std::vector<uint8_t> rawData(fileInfo.m_len);
    if (read(fileInfo.m_fd, &rawData[0], fileInfo.m_len) != fileInfo.m_len) {
      m_debug << "error: '" << fn << "' could not read all data" << endl;
      return;
    }

    const uint8_t * src = &rawData[0];
    std::vector<uint8_t> & dst = fileInfo.m_data;
    dst.reserve(fileInfo.m_len);

    for (off_t i = 0; i < fileInfo.m_len; ++i) {
      switch (*src) {
        case 0x0d:
          dst.push_back(0x0d);
          dst.push_back(0x0a);
          ++i;
          if (i < fileInfo.m_len) {
            if (*++src != 0x0a)
              fileInfo.m_isText = true;  
          }
          break;  

        case 0x0a:
          dst.push_back(0x0d);
          dst.push_back(0x0a);
          fileInfo.m_isText = true;  
          break;

        default:
          dst.push_back(*src);
          break;
      }
      ++src;
    }
    if (!fileInfo.m_isText) {
      fileInfo.m_data.resize(0);
    }
    else {
      fileInfo.m_len = fileInfo.m_data.size(); 
    }
  }

  // save file handle for later use
  *(int *)(fcb + eFCB_User) = fileInfo.m_fd;
  m_debug << "BDOS 15: open file '" << fn << "' open using fd " << fileInfo.m_fd << endl;

  // put file into map
  m_fileMap[fileInfo.m_fd] = std::move(fileInfo);
  fileInfo.m_fd = -1;

  m_proc.m_cpu.AF.B.h = 0x0;
  return;
}

void NewBDOS::CloseFile()
{
  m_proc.m_cpu.AF.B.h = 0xff;
  char * fcb = (char *)m_memory + m_proc.m_cpu.DE.W;
  int fd = *(int *)(fcb + eFCB_User);
  m_debug << "BDOS 15: close file  " << fd << endl;
  auto r = m_fileMap.find(fd);
  if (r == m_fileMap.end()) {
    m_debug << "error: file not found " << endl;
    return;
  }

  m_fileMap.erase(r);
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::SearchFirst()
{
  m_findFCB = m_proc.m_cpu.DE.W;
  uint8_t * fcb = m_memory + m_findFCB;
  m_debug << "BDOS 17: search first '" << FCBToRegex((char *)fcb) << "'" << endl;

  m_findIndex = 0;
  m_proc.m_cpu.AF.B.h = FindFile(fcb);
}

void NewBDOS::SearchNext()
{
  uint8_t * fcb = m_memory + m_findFCB;
  m_debug << "BDOS 18: search next '" << FCBToRegex((char *)fcb) << "'" << endl;
  m_proc.m_cpu.AF.B.h = FindFile(fcb);
}

uint8_t NewBDOS::FindFile(const uint8_t * fcb)
{
  bool anyUser = fcb[0] == '?';
  int drive = DriveFromFCB(fcb);
  if (m_drives[drive].m_kind == DriveSlot::Kind::eImage)
    return SearchImage(drive, fcb, anyUser);

  UpdateDriveInfo(drive);
  return SearchHost(drive, fcb, anyUser);
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
  uint8_t * fcb = (uint8_t *)m_memory + m_proc.m_cpu.DE.W;
  ReadFile(fcb, 20, -1);
}

void NewBDOS::ReadRandom()
{
  uint8_t * fcb = (uint8_t *)m_memory + m_proc.m_cpu.DE.W;
  unsigned int offs = (fcb[eFCB_R0] + (fcb[eFCB_R1] << 8) + (fcb[eFCB_R2] << 16)) << 7;
  ReadFile(fcb, 33, offs);
}

void NewBDOS::ReadFile(uint8_t * fcb, int code, off_t offs)
{
  m_proc.m_cpu.AF.B.h = 0xff;

  int fd = *(int *)(fcb + eFCB_User);

  auto r = m_fileMap.find(fd);
  if (r == m_fileMap.end()) {
    m_debug << "error: unknown file" << endl;
    return;
  }  

  FileInfo & fileInfo = r->second;
  if (fileInfo.m_isImage) {
    ReadImageRecord(fileInfo, code, offs);
    return;
  }

  if (offs < 0)
    offs = fileInfo.m_pos;

  int c = -1;
  if (offs > fileInfo.m_len) {
    m_proc.m_cpu.AF.B.h = 0x01;
  }
  else {
    uint8_t * p = m_memory + m_dmaAddress;
    int len = std::min<int>(128, fileInfo.m_len - offs);
    if (fileInfo.m_isText) {
      memcpy(p, &fileInfo.m_data[offs], len);
      c = 128;
    }
    else {
      off_t o = lseek(fd, offs, SEEK_SET);
      if (o != offs) {
        m_debug << "error: " << o << " seeking " << offs << " : " << strerror(errno) << endl;
        return;
      }
      c = read(fd, p, len);
      if (c < len) {
        m_debug << "error: " << c << " reading " << len << " : " << strerror(errno) << endl;
        return;
      }
    } 
    memset(p+len, 0x1a, 128-len);
    fileInfo.m_pos = offs + 128;
    m_proc.m_cpu.AF.B.h = 0x00;
  }

  m_debug << "BDOS " << code << ": read from fd " << fd << " at " << (int)offs << " returned " << c << ", s = " << (int)m_proc.m_cpu.AF.B.h << endl;

  if (m_proc.m_cpu.AF.B.h == 0x00)
    m_debug << DumpMemory(m_memory + m_dmaAddress, 128);

}

static bool NameMatches(const std::string & name, const uint8_t * fcb)
{
  for (int i = 0; i < 11; ++i) {
    if (fcb[i + 1] == '?')
      continue;
    uint8_t have = (i < (int)name.size()) ? (uint8_t)name[i] : ' ';
    if ((have & 0x7f) != (fcb[i + 1] & 0x7f))
      return false;
  }
  return true;
}

static bool EntryMatches(const uint8_t * entry, const uint8_t * fcb, bool anyUser, uint8_t user, int exm)
{
  if (entry[0] == 0xe5)
    return false;
  if (!anyUser && (entry[0] & 0x1f) != (user & 0x1f))
    return false;
  for (int i = 1; i <= 11; ++i) {
    if (fcb[i] == '?')
      continue;
    if ((entry[i] & 0x7f) != (fcb[i] & 0x7f))
      return false;
  }
  // EXM bits count logical extents packed into one directory entry, so
  // extent 1 on a 2K-block disk is still the file's first entry.
  if (fcb[12] != '?') {
    int mask = (~exm) & 0x1f;
    if ((entry[12] & mask) != (fcb[12] & mask))
      return false;
  }
  if (fcb[14] != '?' && (entry[14] & 0x3f) != (fcb[14] & 0x3f))
    return false;
  return true;
}

static int Allocation(const uint8_t * entry, int index, bool words)
{
  if (words) {
    if (index < 0 || index >= 8)
      return 0;
    return entry[16 + index * 2] | (entry[17 + index * 2] << 8);
  }
  if (index < 0 || index >= 16)
    return 0;
  return entry[16 + index];
}

void NewBDOS::PrintDriveMap()
{
  for (int drive = 0; drive < 16; ++drive) {
    const DriveSlot & slot = m_drives[drive];
    if (!slot.m_configured && drive != 0)
      continue;

    stringstream strm;
    strm << (char)('A' + drive) << ": ";
    if (slot.m_kind == DriveSlot::Kind::eImage) {
      strm << "image " << slot.m_path;
      if (!slot.m_detail.empty())
        strm << " (" << slot.m_detail << ")";
      int sectors = 0;
      int tracks = 0;
      int capacityKb = 0;
      if (CpmDiskGeometry(slot.m_disk, sectors, tracks, capacityKb))
        strm << ", " << sectors << " sectors/track, " << tracks << " tracks, " << capacityKb << "kb";
    }
    else if (slot.m_configured) {
      strm << "directory " << slot.m_path;
    }
    else {
      strm << "directory .";
    }
    strm << "\r\n$";
    PrintCPMString(strm.str().c_str());
    m_debug << "drive map " << strm.str() << endl;
  }
}

bool NewBDOS::ConfigureDrives()
{
  for (const auto & spec : m_proc.m_options.m_cpmDrives) {
    std::string error;
    CpmDriveRequest request;
    if (!ParseCpmDriveRequest(spec, request, error)) {
      cerr << "error: " << error << endl;
      return false;
    }
    DriveSlot & slot = m_drives[request.m_drive];
    if (slot.m_configured) {
      cerr << "error: drive " << (char)('A' + request.m_drive) << " is listed more than once" << endl;
      return false;
    }
    if (!request.m_image) {
      slot.m_kind = DriveSlot::Kind::eHost;
      slot.m_configured = true;
      slot.m_path = request.m_path;
      continue;
    }
    if (!OpenImageDrive(request))
      return false;
  }
  return true;
}

bool NewBDOS::OpenImageDrive(const CpmDriveRequest & request)
{
  std::string error;
  std::shared_ptr<VirtualDrive> image;
  bool raw = false;
  if (!OpenCpmImage(request.m_path, request.m_disk, image, raw, error)) {
    cerr << "error: " << error << endl;
    return false;
  }

  int fd = -1;
  if (raw) {
    fd = ::open(request.m_path.c_str(), O_RDONLY | O_BINARY);
    if (fd < 0) {
      cerr << "error: cannot open CP/M image '" << request.m_path << "'" << endl;
      return false;
    }
  }
  else {
    cerr << "info: drive " << (char)('A' + request.m_drive) << ": " << image->GetFormat() << " image" << endl;
  }

  DriveSlot & slot = m_drives[request.m_drive];
  slot.m_kind = DriveSlot::Kind::eImage;
  slot.m_configured = true;
  slot.m_path = request.m_path;
  slot.m_detail = request.m_disk.m_description.empty() ? request.m_disk.m_name : request.m_disk.m_description;
  slot.m_disk = request.m_disk;
  slot.m_fd = fd;
  slot.m_image = image;
  slot.m_blockValid = false;
  slot.m_block.clear();

  int records = ((slot.m_disk.m_drm + 1) * 32 + 127) / 128;
  slot.m_directory.assign(records * 128, 0xe5);
  for (int i = 0; i < records; ++i) {
    uint32_t diskRec = (uint32_t)slot.m_disk.m_off * slot.m_disk.m_spt + i;
    if (!ReadLogical(request.m_drive, diskRec, slot.m_directory.data() + i * 128)) {
      cerr << "error: cannot read the directory of '" << request.m_path << "'" << endl;
      if (fd >= 0)
        ::close(fd);
      slot.m_fd = -1;
      slot.m_image.reset();
      slot.m_configured = false;
      slot.m_kind = DriveSlot::Kind::eHost;
      return false;
    }
  }
  return true;
}

void NewBDOS::ClearHostCaches()
{
  for (auto & slot : m_drives) {
    if (slot.m_kind != DriveSlot::Kind::eHost)
      continue;
    slot.m_host.m_cpmToNative.clear();
    slot.m_host.m_nativeToCPM.clear();
  }
}

int NewBDOS::DriveFromFCB(const uint8_t * fcb) const
{
  if (fcb[0] == '?')
    return m_currDisk;
  int drive = fcb[0] & 0x1f;
  if (drive == 0)
    return m_currDisk;
  --drive;
  if (drive < 0 || drive > 15)
    return m_currDisk;
  return drive;
}

void NewBDOS::PublishDPB(int drive)
{
  const CpmDiskDef & disk = m_drives[drive].m_disk;
  static const uint16_t kDpbAddress = 0xff80;
  uint8_t * p = m_memory + kDpbAddress;
  p[0] = disk.m_spt & 0xff;
  p[1] = (disk.m_spt >> 8) & 0xff;
  p[2] = disk.m_bsh;
  p[3] = disk.m_blm;
  p[4] = disk.m_exm;
  p[5] = disk.m_dsm & 0xff;
  p[6] = (disk.m_dsm >> 8) & 0xff;
  p[7] = disk.m_drm & 0xff;
  p[8] = (disk.m_drm >> 8) & 0xff;
  p[9] = disk.m_al0;
  p[10] = disk.m_al1;
  p[11] = disk.m_cks & 0xff;
  p[12] = (disk.m_cks >> 8) & 0xff;
  p[13] = disk.m_off & 0xff;
  p[14] = (disk.m_off >> 8) & 0xff;
  p[15] = 0;
  m_proc.m_cpu.HL.W = kDpbAddress;
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::GetDiskParams()
{
  m_debug << "BDOS 31: get disk parameters for " << (char)('A' + m_currDisk) << endl;
  if (m_drives[m_currDisk].m_kind != DriveSlot::Kind::eImage) {
    m_proc.m_cpu.HL.W = 0;
    m_proc.m_cpu.AF.B.h = 0;
    return;
  }
  PublishDPB(m_currDisk);
}

static void MapCylinder(const CpmDiskDef & disk, uint32_t logicalTrack, int & side, int & cylinder)
{
  side = 0;
  cylinder = (int)logicalTrack;
  if (disk.m_sides == CpmSides::eSingle)
    return;

  int sectors = 0;
  int tracks = 0;
  int capacityKb = 0;
  if (!CpmDiskGeometry(disk, sectors, tracks, capacityKb) || tracks < 2)
    return;

  uint32_t cylinders = (uint32_t)tracks / 2;
  if (disk.m_sides == CpmSides::eCylinder) {
    side = (int)(logicalTrack & 1);
    cylinder = (int)(logicalTrack >> 1);
    return;
  }
  if (logicalTrack < cylinders) {
    side = 0;
    cylinder = (int)logicalTrack;
  }
  else {
    side = 1;
    cylinder = (int)((uint32_t)tracks - 1 - logicalTrack);
  }
}

static bool ReadDriveRecord(NewBDOS::DriveSlot & slot, uint32_t diskRec, uint8_t * dest)
{
  const CpmDiskDef & disk = slot.m_disk;
  VirtualDrive & image = *slot.m_image;
  if (disk.m_spt <= 0)
    return false;
  uint32_t logicalTrack = diskRec / (uint32_t)disk.m_spt;
  uint32_t logical = diskRec % (uint32_t)disk.m_spt;
  if (logical >= disk.m_translate.size())
    return false;

  int side = 0;
  int cylinder = 0;
  MapCylinder(disk, logicalTrack, side, cylinder);

  int sectorSize = image.GetSectorSize();
  if (sectorSize < 128 || (sectorSize % 128) != 0)
    return false;
  int recordsPerSector = sectorSize / 128;
  if ((disk.m_spt % recordsPerSector) != 0)
    return false;

  // A translate table names physical sectors. Several 128-byte records share
  // one sector, so the table is applied after those records are grouped.
  int sectorId = 0;
  int offset = 0;
  int physicalSectors = disk.m_spt / recordsPerSector;
  int span = (int)disk.m_sectorIds.size();
  if (span > 0 && (physicalSectors % span) == 0) {
    int physical = (int)logical / recordsPerSector;
    offset = ((int)logical % recordsPerSector) * 128;
    int index = physical % span;
    int base = (physical / span) * span;
    sectorId = base + disk.m_sectorIds[index];
  }
  else {
    uint32_t record = disk.m_translate[logical];
    sectorId = (int)(record / recordsPerSector) + 1;
    offset = (int)(record % recordsPerSector) * 128;
  }

  const VirtualDrive::TrackInfo * track = image.GetTrack(side, cylinder);
  if (track == nullptr)
    return false;
  const VirtualDrive::SectorInfo * info = track->GetSector(sectorId);
  if (info == nullptr && sectorId > 0)
    info = track->GetSector(sectorId - 1);
  if (info == nullptr && sectorId > 0 && sectorId - 1 < track->GetSectorCount())
    info = &track->GetSectors()[sectorId - 1];
  if (info == nullptr)
    return false;

  bool cached = slot.m_blockValid
    && slot.m_blockSide == side
    && slot.m_blockCylinder == cylinder
    && slot.m_blockId == info->m_id
    && (int)slot.m_block.size() >= offset + 128;
  if (!cached) {
    slot.m_blockValid = false;
    slot.m_block.assign(sectorSize, 0);
    VirtualDrive::SectorInfo got;
    int n = image.ReadSector(side, cylinder, info->m_id, got, slot.m_block.data(), sectorSize);
    if (n < offset + 128)
      return false;
    if (n < sectorSize)
      slot.m_block.resize(n);
    slot.m_blockSide = side;
    slot.m_blockCylinder = cylinder;
    slot.m_blockId = info->m_id;
    slot.m_blockValid = true;
  }
  memcpy(dest, slot.m_block.data() + offset, 128);
  return true;
}

bool NewBDOS::ReadLogical(int drive, uint32_t diskRec, uint8_t * dest)
{
  DriveSlot & slot = m_drives[drive];
  const CpmDiskDef & disk = slot.m_disk;
  if (slot.m_image)
    return ReadDriveRecord(slot, diskRec, dest);
  if (slot.m_fd < 0 || disk.m_spt <= 0)
    return false;
  uint32_t track = diskRec / (uint32_t)disk.m_spt;
  uint32_t logical = diskRec % (uint32_t)disk.m_spt;
  if (logical >= disk.m_translate.size())
    return false;
  uint32_t physical = (uint32_t)disk.m_translate[logical];
  uint32_t imageTrack = CpmImageTrack(disk, track);
  off_t byteOff = ((off_t)imageTrack * disk.m_spt + physical) * 128;
  ssize_t n = ::pread(slot.m_fd, dest, 128, byteOff);
  if (n < 0)
    return false;
  if (n < 128)
    memset(dest + n, 0x1a, (size_t)(128 - n));
  return n > 0;
}

bool NewBDOS::OpenImage(int drive, uint8_t * fcb)
{
  DriveSlot & slot = m_drives[drive];
  int entries = slot.m_disk.m_drm + 1;
  const uint8_t * match = nullptr;
  bool anyUser = fcb[0] == '?';
  for (int index = 0; index < entries; ++index) {
    const uint8_t * entry = slot.m_directory.data() + index * 32;
    if (!EntryMatches(entry, fcb, anyUser, m_userCode, slot.m_disk.m_exm))
      continue;
    match = entry;
    break;
  }
  if (match == nullptr)
    return false;

  uint8_t driveByte = fcb[0];
  memcpy(fcb, match, 32);
  fcb[0] = driveByte;
  fcb[32] = 0;

  FileInfo info;
  info.m_isImage = true;
  info.m_drive = drive;
  info.m_fd = -1;
  BuildImageRecords(drive, match, info);

  int id = m_nextFileId++;
  memcpy(fcb + eFCB_User, &id, sizeof(id));
  m_debug << "BDOS 15: opened image file as " << id << " (" << info.m_diskRecords.size() << " records)" << endl;
  m_fileMap[id] = std::move(info);
  return true;
}

void NewBDOS::BuildImageRecords(int drive, const uint8_t * wanted, FileInfo & info)
{
  const DriveSlot & slot = m_drives[drive];
  const CpmDiskDef & disk = slot.m_disk;
  bool words = disk.m_dsm >= 256;
  int exm = disk.m_exm;
  int recordsPerBlock = 1 << disk.m_bsh;
  int entries = disk.m_drm + 1;
  info.m_diskRecords.clear();

  for (int index = 0; index < entries; ++index) {
    const uint8_t * entry = slot.m_directory.data() + index * 32;
    if (entry[0] == 0xe5)
      continue;
    if ((entry[0] & 0x1f) != (wanted[0] & 0x1f))
      continue;
    bool sameName = true;
    for (int i = 1; i <= 11; ++i) {
      if ((entry[i] & 0x7f) != (wanted[i] & 0x7f)) {
        sameName = false;
        break;
      }
    }
    if (!sameName)
      continue;

    int extent = entry[12] & 0x1f;
    int module = entry[14] & 0x3f;
    int baseExtent = module * 32 + (extent & ~exm);
    int rc = entry[15];
    int recs = (rc >= 128) ? ((extent & exm) + 1) * 128 : (extent & exm) * 128 + rc;
    size_t start = (size_t)baseExtent * 128;
    if (info.m_diskRecords.size() < start + (size_t)recs)
      info.m_diskRecords.resize(start + (size_t)recs, 0);
    for (int rec = 0; rec < recs; ++rec) {
      int block = Allocation(entry, rec >> disk.m_bsh, words);
      uint32_t diskRec = 0;
      if (block > 0 && block <= disk.m_dsm)
        diskRec = (uint32_t)disk.m_off * disk.m_spt + (uint32_t)block * recordsPerBlock + (rec & disk.m_blm);
      info.m_diskRecords[start + rec] = diskRec;
    }
  }
  info.m_len = (off_t)info.m_diskRecords.size() * 128;
}

void NewBDOS::ReadImageRecord(FileInfo & info, int code, off_t offs)
{
  bool sequential = offs < 0;
  size_t record = sequential ? (size_t)(info.m_pos / 128) : (size_t)(offs / 128);
  if (record >= info.m_diskRecords.size()) {
    m_proc.m_cpu.AF.B.h = 0x01;
    m_debug << "BDOS " << code << ": image end of file at record " << record << endl;
    return;
  }
  uint32_t diskRec = info.m_diskRecords[record];
  if (diskRec == 0) {
    m_proc.m_cpu.AF.B.h = 0xff;
    m_debug << "BDOS " << code << ": image hole at record " << record << endl;
    return;
  }
  if (!ReadLogical(info.m_drive, diskRec, m_memory + m_dmaAddress)) {
    m_proc.m_cpu.AF.B.h = 0xff;
    m_debug << "BDOS " << code << ": image read failed at record " << record << endl;
    return;
  }
  if (sequential)
    info.m_pos += 128;
  m_proc.m_cpu.AF.B.h = 0x00;
  m_debug << "BDOS " << code << ": image record " << record << " from disk record " << diskRec << endl;
}

uint8_t NewBDOS::SearchHost(int drive, const uint8_t * fcb, bool anyUser)
{
  if (!anyUser && m_userCode != 0)
    return 0xff;
  if (fcb[12] != '?' && (fcb[12] & 0x1f) != 0)
    return 0xff;
  if (fcb[14] != '?' && (fcb[14] & 0x3f) != 0)
    return 0xff;

  std::vector<std::string> names;
  for (const auto & entry : m_drives[drive].m_host.m_cpmToNative)
    names.push_back(entry.first);

  int start = m_findIndex < 0 ? 0 : m_findIndex;
  for (int index = start; index < (int)names.size(); ++index) {
    if (!NameMatches(names[index], fcb))
      continue;

    int sector = index / 4;
    int slot = index % 4;
    uint8_t * dma = m_memory + m_dmaAddress;
    memset(dma, 0xe5, 128);
    for (int s = 0; s < 4; ++s) {
      int entryIndex = sector * 4 + s;
      if (entryIndex >= (int)names.size())
        break;
      uint8_t * dest = dma + s * 32;
      memset(dest, 0, 32);
      size_t length = std::min<size_t>(11, names[entryIndex].size());
      memcpy(dest + 1, names[entryIndex].data(), length);
    }
    m_findIndex = index + 1;
    m_debug << names[index] << " matches in slot " << slot << endl;
    return (uint8_t)slot;
  }

  m_debug << "no more host matches" << endl;
  return 0xff;
}

uint8_t NewBDOS::SearchImage(int drive, const uint8_t * fcb, bool anyUser)
{
  const DriveSlot & slot = m_drives[drive];
  int entries = slot.m_disk.m_drm + 1;
  int start = m_findIndex < 0 ? 0 : m_findIndex;
  for (int index = start; index < entries; ++index) {
    const uint8_t * entry = slot.m_directory.data() + index * 32;
    if (!EntryMatches(entry, fcb, anyUser, m_userCode, slot.m_disk.m_exm))
      continue;

    int slotIndex = index % 4;
    const uint8_t * sector = slot.m_directory.data() + (index / 4) * 128;
    memcpy(m_memory + m_dmaAddress, sector, 128);
    m_findIndex = index + 1;
    m_debug << "image directory entry " << index << " in slot " << slotIndex << endl;
    return (uint8_t)slotIndex;
  }

  m_debug << "no more image matches" << endl;
  return 0xff;
}
