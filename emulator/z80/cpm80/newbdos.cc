#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

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

#define ColdBootTitle "\rCP/M 2.2 NewBDOS\r\n$"

extern unsigned char z80_cpm80_newbdos_bin[2398];

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
  &NewBDOS::WriteSeq,       // 21 - Write sequential
  &NewBDOS::MakeFile,       // 22 - Create file
  NULL,                     // 23 - Rename file
  &NewBDOS::ReturnLoginVector, // 24 - Return login vector
  &NewBDOS::GetCurrDisk,    // 25 - Return current disk
  &NewBDOS::SetDMAAddress,  // 26 - Set DMA address
  &NewBDOS::GetAllocVector, // 27 - Get allocation vector
  NULL,                     // 28 - Write protect disk
  &NewBDOS::ReturnReadOnlyVector, // 29 - Get read/only vector
  NULL,                     // 30 - Set file attributes
  &NewBDOS::GetDiskParams,  // 31 - Get disk parm addr
  &NewBDOS::GetSetUser,     // 32 - Set/get user code
  &NewBDOS::ReadRandom,     // 33 - Read random
  &NewBDOS::WriteRandom,    // 34 - Write random
  &NewBDOS::ComputeFileSize, // 35 - Compute file size
  &NewBDOS::SetRandomRecord // 36 - Set random record
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
  m_drives[drive].Refresh(&m_debug);
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
    
  if (m_drives[drive].m_kind != CpmDrive::Kind::eHost)
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
  CpmDrive::HostNames & driveInfo = m_drives[drive].m_host;
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
}

void NewBDOS::OnBDOSCommand(uint8_t code)
{
  if (code == 0xff) {
    //m_debug << "BDOS " << code << "0xff" << endl;
    Boot();
    return;
  }
  if (code == 0xfe) {
    CcpCommand();
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

static const struct {
  const char * name;
  NewBDOS::Function func;
} CcpCommands[] = {
  { "EXIT", &NewBDOS::CcpExit },
  { "LCD",  &NewBDOS::CcpLcd },
  { "LCP",  &NewBDOS::CcpLcp },
  { "LLS",  &NewBDOS::CcpLls },
  { "LPWD", &NewBDOS::CcpLpwd },
};

void NewBDOS::CcpExit()
{
  m_debug << "CCP exit" << endl;
  exit(0);
}

// newbdos.asm places the 130-byte INBUFF at CCPB+6 and INPOINT right after it.
// CONVFST leaves that word on the first character after the command name.
static const uint16_t kCcpInPoint = CCPB + 136;

static std::string DefaultHostRoot(const CpmDrive & slot, int drive)
{
  if (slot.m_configured && !slot.m_path.empty())
    return slot.m_path;
  if (drive == 0)
    return ".";
  return std::string("./") + char('a' + drive);
}

static char * CanonicalPath(const std::string & path)
{
#if __linux__ || __APPLE__
  return realpath(path.c_str(), NULL);
#endif
#if __WIN32
  return _fullpath(NULL, path.c_str(), 0);
#endif
}

// CONVFST leaves INPOINT on the first character after the command name.
// Park it on the terminating null so the CCP does not parse the tail.
static std::string TakeCommandTail(uint8_t * memory)
{
  uint16_t point = memory[kCcpInPoint] | (memory[kCcpInPoint + 1] << 8);
  int guard = 0;
  while (guard < 128 && memory[point] == ' ') {
    ++point;
    ++guard;
  }
  uint16_t start = point;
  while (guard < 128 && memory[point] != 0) {
    ++point;
    ++guard;
  }
  uint16_t end = point;
  while (end > start && memory[end - 1] == ' ')
    --end;
  memory[kCcpInPoint] = point & 0xff;
  memory[kCcpInPoint + 1] = point >> 8;

  std::string arg;
  for (uint16_t i = start; i < end; ++i)
    arg.push_back((char)memory[i]);
  return arg;
}

// CCP commands print their first line with a leading CR/LF. The command
// loop supplies the CR/LF before the next prompt.
static void PrintCommandResult(NewBDOS & bdos, const std::string & text)
{
  std::string msg = "\r\n" + text + "$";
  bdos.PrintCPMString(msg.c_str());
}

static void ParseDrivePrefix(const std::string & arg, int & drive, std::string & path)
{
  path = arg;
  if (arg.size() < 2 || arg[1] != ':')
    return;
  char letter = arg[0];
  if (letter >= 'a' && letter <= 'z')
    letter -= 'a' - 'A';
  if (letter < 'A' || letter > 'P')
    return;
  drive = letter - 'A';
  path = arg.substr(2);
  while (!path.empty() && path[0] == ' ')
    path.erase(path.begin());
}

void NewBDOS::CcpLpwd()
{
  TakeCommandTail(m_memory);
  if (m_currDisk >= m_drives.size() || m_drives[m_currDisk].m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }

  int drive = m_currDisk;
  CpmDrive & slot = m_drives[drive];
  UpdateDriveInfo(drive);
  std::string shown = slot.m_host.m_dir;
  if (!shown.empty() && shown.back() == DIR_SEPERATOR)
    shown.pop_back();
  if (shown.empty())
    shown = DefaultHostRoot(slot, drive);
  PrintCommandResult(*this, std::string(1, char('A' + drive)) + ": " + shown);
}

static std::string UnixMode(mode_t mode)
{
  std::string text(10, '-');
  if (S_ISDIR(mode))
    text[0] = 'd';
  else if (S_ISLNK(mode))
    text[0] = 'l';
  else if (S_ISCHR(mode))
    text[0] = 'c';
  else if (S_ISBLK(mode))
    text[0] = 'b';
  else if (S_ISFIFO(mode))
    text[0] = 'p';
  else if (S_ISSOCK(mode))
    text[0] = 's';

  const mode_t flags[9] = {
    S_IRUSR, S_IWUSR, S_IXUSR,
    S_IRGRP, S_IWGRP, S_IXGRP,
    S_IROTH, S_IWOTH, S_IXOTH
  };
  const char letters[9] = { 'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x' };
  for (int i = 0; i < 9; ++i) {
    if (mode & flags[i])
      text[i + 1] = letters[i];
  }
  if (mode & S_ISUID)
    text[3] = (text[3] == 'x') ? 's' : 'S';
  if (mode & S_ISGID)
    text[6] = (text[6] == 'x') ? 's' : 'S';
  if (mode & S_ISVTX)
    text[9] = (text[9] == 'x') ? 't' : 'T';
  return text;
}

static std::string UnixDate(time_t when)
{
  const time_t sixMonths = 183L * 24L * 60L * 60L;
  time_t now = time(NULL);
  bool showYear = when > now + sixMonths || now > when + sixMonths;
  struct tm broken;
#if __WIN32
  localtime_s(&broken, &when);
#else
  localtime_r(&when, &broken);
#endif
  char buf[16];
  strftime(buf, sizeof(buf), showYear ? "%b %e  %Y" : "%b %e %H:%M", &broken);
  return buf;
}

static std::string CpmDisplayName(const std::string & raw)
{
  std::string name = raw;
  if (name.size() < 11)
    name.append(11 - name.size(), ' ');
  return name.substr(0, 8) + "." + name.substr(8, 3);
}

void NewBDOS::CcpLls()
{
  std::string arg = TakeCommandTail(m_memory);
  if (m_currDisk >= m_drives.size() || m_drives[m_currDisk].m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }

  int drive = m_currDisk;
  std::string path;
  ParseDrivePrefix(arg, drive, path);
  CpmDrive & slot = m_drives[drive];
  if (slot.m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }

  UpdateDriveInfo(drive);

  struct Row {
    std::string cpm;
    std::string host;
    std::string mode;
    std::string date;
    long long size;
  };
  std::vector<Row> rows;
  int sizeWidth = 1;
  for (const auto & item : slot.m_host.m_cpmToNative) {
    std::string full = slot.m_host.m_dir + item.second;
    struct stat st;
    if (stat(full.c_str(), &st) != 0)
      continue;
    Row row;
    row.cpm = CpmDisplayName(item.first);
    row.host = full;
    row.mode = UnixMode(st.st_mode);
    row.date = UnixDate(st.st_mtime);
    row.size = (long long)st.st_size;
    int digits = 1;
    for (long long n = row.size < 0 ? -row.size : row.size; n >= 10; n /= 10)
      ++digits;
    if (digits > sizeWidth)
      sizeWidth = digits;
    rows.push_back(row);
  }

  std::sort(rows.begin(), rows.end(), [](const Row & a, const Row & b) {
    return a.host < b.host;
  });

  if (rows.empty()) {
    PrintCommandResult(*this, "No file");
    return;
  }

  for (const auto & row : rows) {
    std::string size(sizeWidth, ' ');
    long long n = row.size < 0 ? 0 : row.size;
    for (int i = sizeWidth - 1; i >= 0; --i) {
      size[i] = (char)('0' + (n % 10));
      n /= 10;
      if (n == 0)
        break;
    }
    std::string line = row.mode + " " + size + " " + row.date + " " + row.cpm + " " + row.host;
    m_proc.ConsoleOut('\r');
    m_proc.ConsoleOut('\n');
    for (char ch : line)
      m_proc.ConsoleOut(ch);
  }
}

struct CopyRef {
  int drive = 0;
  bool hasName = false;
  bool wild = false;
  char name[11];
};

static bool ParseCopyRef(const std::string & token, int currentDrive, CopyRef & ref)
{
  memset(ref.name, ' ', 11);
  ref.drive = currentDrive;
  ref.hasName = false;
  ref.wild = false;

  std::string body = token;
  if (body.size() >= 2 && body[1] == ':') {
    char letter = body[0];
    if (letter >= 'a' && letter <= 'z')
      letter -= 'a' - 'A';
    if (letter < 'A' || letter > 'P')
      return false;
    ref.drive = letter - 'A';
    body = body.substr(2);
  }
  if (body.empty())
    return true;

  ref.hasName = true;
  int pos = 0;
  int limit = 8;
  bool inExt = false;
  for (unsigned char ch : body) {
    if (ch >= 'a' && ch <= 'z')
      ch -= 'a' - 'A';
    if (ch == '.') {
      if (inExt)
        return false;
      inExt = true;
      pos = 8;
      limit = 11;
      continue;
    }
    if (ch == '*') {
      ref.wild = true;
      while (pos < limit)
        ref.name[pos++] = '?';
      continue;
    }
    if (ch < 0x21 || ch == '/' || ch == '\\' || ch == ':' || ch == '=' || ch == '<' || ch == '>' || ch == '|')
      return false;
    if (pos >= limit)
      return false;
    if (ch == '?')
      ref.wild = true;
    ref.name[pos++] = (char)ch;
  }

  bool any = false;
  for (int i = 0; i < 8; ++i) {
    if (ref.name[i] != ' ')
      any = true;
  }
  return any;
}

static bool NamePatternMatches(const char * have, const char * pattern)
{
  for (int i = 0; i < 11; ++i) {
    if (pattern[i] == '?')
      continue;
    if ((have[i] & 0x7f) != (pattern[i] & 0x7f))
      return false;
  }
  return true;
}

static std::string HostBaseFromCpm(const char * name)
{
  std::string base(name, name + 8);
  std::string ext(name + 8, name + 11);
  while (!base.empty() && base.back() == ' ')
    base.pop_back();
  while (!ext.empty() && ext.back() == ' ')
    ext.pop_back();
  if (base.empty())
    return "";
  if (ext.empty())
    return base;
  return base + "." + ext;
}

static std::vector<std::string> CommandTokens(const std::string & arg)
{
  std::vector<std::string> tokens;
  std::string cur;
  for (char ch : arg) {
    if (ch == ' ') {
      if (!cur.empty()) {
        tokens.push_back(cur);
        cur.clear();
      }
    }
    else {
      cur += ch;
    }
  }
  if (!cur.empty())
    tokens.push_back(cur);
  return tokens;
}

void NewBDOS::CcpLcp()
{
  std::string arg = TakeCommandTail(m_memory);
  std::vector<std::string> tokens = CommandTokens(arg);
  if (tokens.size() != 2) {
    PrintCommandResult(*this, "LCP d:file d:[file]");
    return;
  }

  CopyRef source;
  CopyRef dest;
  if (!ParseCopyRef(tokens[0], m_currDisk, source) || !source.hasName ||
      !ParseCopyRef(tokens[1], m_currDisk, dest) || dest.wild) {
    PrintCommandResult(*this, "Syntax error");
    return;
  }
  if (source.wild && dest.hasName) {
    PrintCommandResult(*this, "Bad destination");
    return;
  }

  CpmDrive & sourceSlot = m_drives[source.drive];
  CpmDrive & destSlot = m_drives[dest.drive];
  if (destSlot.m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }
  if (sourceSlot.m_kind == CpmDrive::Kind::eHost)
    UpdateDriveInfo(source.drive);
  UpdateDriveInfo(dest.drive);
  if (destSlot.m_host.m_dir.empty()) {
    PrintCommandResult(*this, "Path not found");
    return;
  }

  struct SrcFile {
    std::string cpm;
    std::string native;
  };
  std::vector<SrcFile> files;
  if (sourceSlot.m_kind == CpmDrive::Kind::eImage) {
    std::vector<std::string> names;
    int entries = sourceSlot.m_disk.m_drm + 1;
    for (int index = 0; index < entries; ++index) {
      size_t off = (size_t)index * 32;
      if (off + 32 > sourceSlot.m_directory.size())
        break;
      const uint8_t * entry = sourceSlot.m_directory.data() + off;
      if (entry[0] == 0xe5)
        continue;
      if ((entry[0] & 0x1f) != (m_userCode & 0x1f))
        continue;
      char have[11];
      for (int i = 0; i < 11; ++i)
        have[i] = (char)(entry[1 + i] & 0x7f);
      if (!NamePatternMatches(have, source.name))
        continue;
      std::string cpm(have, have + 11);
      if (std::find(names.begin(), names.end(), cpm) == names.end())
        names.push_back(cpm);
    }
    std::sort(names.begin(), names.end());
    for (const auto & cpm : names)
      files.push_back(SrcFile{cpm, ""});
  }
  else {
    for (const auto & item : sourceSlot.m_host.m_cpmToNative) {
      std::string cpm = item.first;
      if (cpm.size() < 11)
        cpm.append(11 - cpm.size(), ' ');
      if (!NamePatternMatches(cpm.c_str(), source.name))
        continue;
      std::string full = sourceSlot.m_host.m_dir + item.second;
      struct stat st;
      if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
        continue;
      files.push_back(SrcFile{cpm.substr(0, 11), item.second});
    }
    std::sort(files.begin(), files.end(), [](const SrcFile & a, const SrcFile & b) {
      return a.cpm < b.cpm;
    });
  }

  if (files.empty()) {
    PrintCommandResult(*this, "No file");
    return;
  }

  auto show = [](int drive, const std::string & cpm) {
    return std::string(1, char('A' + drive)) + ":" + CpmDisplayName(cpm);
  };

  bool copied = false;
  for (const auto & file : files) {
    std::string destCpm = dest.hasName ? std::string(dest.name, dest.name + 11) : file.cpm;
    std::string destBase = dest.hasName ? HostBaseFromCpm(dest.name) : file.native;
    if (destBase.empty())
      destBase = HostBaseFromCpm(destCpm.c_str());
    if (destBase.empty()) {
      PrintCommandResult(*this, "Syntax error");
      return;
    }
    std::string destPath = destSlot.m_host.m_dir + destBase;
    std::string sourcePath;
    if (!file.native.empty())
      sourcePath = sourceSlot.m_host.m_dir + file.native;
    if (!sourcePath.empty() && sourcePath == destPath) {
      PrintCommandResult(*this, "Same file");
      return;
    }

    std::vector<uint8_t> data;
    if (sourceSlot.m_kind == CpmDrive::Kind::eImage) {
      const uint8_t * found = nullptr;
      int entries = sourceSlot.m_disk.m_drm + 1;
      for (int index = 0; index < entries && found == nullptr; ++index) {
        const uint8_t * entry = sourceSlot.m_directory.data() + (size_t)index * 32;
        if (entry[0] == 0xe5)
          continue;
        if ((entry[0] & 0x1f) != (m_userCode & 0x1f))
          continue;
        bool same = true;
        for (int i = 0; i < 11; ++i) {
          if ((entry[1 + i] & 0x7f) != (uint8_t)file.cpm[i]) {
            same = false;
            break;
          }
        }
        if (same)
          found = entry;
      }
      if (found == nullptr) {
        PrintCommandResult(*this, "No file");
        return;
      }
      FileInfo info;
      info.m_fd = -1;
      info.m_drive = source.drive;
      BuildImageRecords(source.drive, found, info);
      data.assign(info.m_diskRecords.size() * 128, 0x1a);
      bool ok = true;
      for (size_t rec = 0; rec < info.m_diskRecords.size(); ++rec) {
        uint32_t diskRec = info.m_diskRecords[rec];
        if (diskRec == 0)
          continue;
        if (!ReadLogical(source.drive, diskRec, data.data() + rec * 128)) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        PrintCommandResult(*this, "Read error");
        return;
      }
    }
    else {
      int fd = ::open(sourcePath.c_str(), O_RDONLY | O_BINARY);
      if (fd < 0) {
        PrintCommandResult(*this, "Read error");
        return;
      }
      struct stat st;
      if (fstat(fd, &st) != 0 || st.st_size < 0) {
        ::close(fd);
        PrintCommandResult(*this, "Read error");
        return;
      }
      data.resize((size_t)st.st_size);
      size_t done = 0;
      bool ok = true;
      while (done < data.size()) {
        ssize_t n = ::read(fd, data.data() + done, data.size() - done);
        if (n < 0) {
          if (errno == EINTR)
            continue;
          ok = false;
          break;
        }
        if (n == 0)
          break;
        done += (size_t)n;
      }
      ::close(fd);
      if (!ok || done != data.size()) {
        PrintCommandResult(*this, "Read error");
        return;
      }
    }

    int out = ::open(destPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (out < 0) {
      PrintCommandResult(*this, "Write error");
      return;
    }
    size_t done = 0;
    bool ok = true;
    while (done < data.size()) {
      ssize_t n = ::write(out, data.data() + done, data.size() - done);
      if (n < 0) {
        if (errno == EINTR)
          continue;
        ok = false;
        break;
      }
      done += (size_t)n;
    }
    if (ok)
      ok = ::fsync(out) == 0;
    ::close(out);
    if (!ok) {
      ::unlink(destPath.c_str());
      PrintCommandResult(*this, "Write error");
      return;
    }

    copied = true;
    std::string line = show(source.drive, file.cpm) + " -> " + show(dest.drive, destCpm);
    m_proc.ConsoleOut('\r');
    m_proc.ConsoleOut('\n');
    for (char ch : line)
      m_proc.ConsoleOut(ch);
  }

  if (copied) {
    m_login &= (uint16_t)~(1u << dest.drive);
    UpdateDriveInfo(dest.drive);
  }
}

void NewBDOS::CcpLcd()
{
  std::string arg = TakeCommandTail(m_memory);
  if (m_currDisk >= m_drives.size() || m_drives[m_currDisk].m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }

  int drive = m_currDisk;
  std::string path;
  ParseDrivePrefix(arg, drive, path);
  CpmDrive & slot = m_drives[drive];
  if (slot.m_kind != CpmDrive::Kind::eHost) {
    PrintCommandResult(*this, "Not a directory drive");
    return;
  }

  if (path.empty()) {
    UpdateDriveInfo(drive);
    std::string shown = slot.m_host.m_dir;
    if (!shown.empty() && shown.back() == DIR_SEPERATOR)
      shown.pop_back();
    if (shown.empty())
      shown = DefaultHostRoot(slot, drive);
    PrintCommandResult(*this, std::string(1, char('A' + drive)) + ": " + shown);
    return;
  }

  if (path[0] == '~' && (path.size() == 1 || path[1] == DIR_SEPERATOR)) {
    const char * home = getenv("HOME");
    if (home != NULL)
      path = std::string(home) + path.substr(1);
  }
  if (path.empty() || path[0] != DIR_SEPERATOR) {
    std::string base = DefaultHostRoot(slot, drive);
    if (!base.empty() && base.back() != DIR_SEPERATOR)
      base += DIR_SEPERATOR;
    path = base + path;
  }

  char * canonical = CanonicalPath(path);
  if (canonical == NULL) {
    PrintCommandResult(*this, "Path not found");
    return;
  }
  struct stat st;
  if (stat(canonical, &st) != 0 || !S_ISDIR(st.st_mode)) {
    free(canonical);
    PrintCommandResult(*this, "Not a directory");
    return;
  }

  slot.SetHostRoot(canonical);
  free(canonical);
  // Logging the drive out makes the next select rebuild the directory
  // and allocation buffers for the new host directory.
  m_login &= (uint16_t)~(1u << drive);
  for (auto it = m_fileMap.begin(); it != m_fileMap.end(); ) {
    if (!it->second.m_isImage && it->second.m_drive == drive)
      it = m_fileMap.erase(it);
    else
      ++it;
  }
  UpdateDriveInfo(drive);

  PrintCommandResult(*this, std::string(1, char('A' + drive)) + ": " + slot.m_path);
  m_debug << "CCP lcd " << slot.m_path << endl;
}

void NewBDOS::CcpCommand()
{
  uint16_t fcb = m_proc.m_cpu.DE.W;
  char name[8];
  for (int i = 0; i < 8; ++i)
    name[i] = m_memory[fcb + 1 + i] & 0x7f;

  for (int i = 0; i < 3; ++i) {
    if ((m_memory[fcb + 9 + i] & 0x7f) != ' ') {
      m_proc.m_cpu.AF.B.h = 0;
      return;
    }
  }

  for (const auto & command : CcpCommands) {
    int i = 0;
    bool match = true;
    for (; command.name[i] != 0 && i < 8; ++i) {
      if (name[i] != command.name[i]) {
        match = false;
        break;
      }
    }
    if (!match || command.name[i] != 0)
      continue;
    for (; i < 8; ++i) {
      if (name[i] != ' ') {
        match = false;
        break;
      }
    }
    if (!match)
      continue;
    m_proc.m_cpu.AF.B.h = 0xff;
    (this->*command.func)();
    return;
  }

  m_proc.m_cpu.AF.B.h = 0;
  m_debug << "CCP command not handled" << endl;
}

void NewBDOS::SystemReset()
{
  m_debug << "BDOS 0: system reset" << endl;
  if (m_proc.m_options.m_arg.empty()) {
    PrintCPMString(ColdBootTitle);
    stringstream strm;
    strm << "CCP=" << hex << CCPB << ",BDOS=" << BDOS << ",BIOS=" << BIOS << "\r\n$";
    PrintCPMString(strm.str().c_str());
    PrintDriveMap();
  }

  ClearHostCaches();

  // Cold boot logs in drive A. The BIOS leaves that drive in C.
  m_proc.WriteMemory(4, 0x00);
  m_proc.m_cpu.BC.B.l = 0;

  Boot();
}

void NewBDOS::Boot()
{
  m_debug << "booting" << endl;

  // BIOS warm boot captured the logged-in drive in C before this reload.
  uint8_t logged = m_proc.m_cpu.BC.B.l;

  m_fileMap.clear();

  memset(m_memory, 0, 0x100);
  m_memory[4] = logged;

  // set warm boot vector at 0x0000 to BIOS + 3
  m_memory[0x0000] = 0xc3;
  m_memory[0x0001] = (BIOS + 3) & 0xff;
  m_memory[0x0002] = ((BIOS + 3) >> 8) & 0xff;

  // set BDOS jump vector at 0x0005 to BDOS + 0
  m_memory[0x0005] = 0xc3;
  m_memory[0x0006] = BDOS & 0xff;
  m_memory[0x0007] = (BDOS >> 8) & 0xff;

  // start running CCP, with the preserved drive in C
  m_dmaAddress        = 0x0080;
  m_proc.m_cpu.BC.B.l = logged;
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
    if (m_drives[0].m_kind == CpmDrive::Kind::eHost) {
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
          m_proc.m_cpu.PC.W = 0;
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
  m_login = 0;
  m_readOnly = 0;
  ClearHostCaches();
}

void NewBDOS::SelDisk()
{
  m_currDisk = m_proc.m_cpu.DE.B.l & 0x0f;
  m_debug << "BDOS 14: sel disk " << (char)('A' + m_currDisk) << endl;
  m_proc.WriteMemory(4, m_currDisk);
  uint16_t bit = (uint16_t)(1u << m_currDisk);
  m_login |= bit;
  if (m_drives[m_currDisk].m_kind == CpmDrive::Kind::eImage) {
    m_readOnly |= bit;
    PublishDPB(m_currDisk);
  }
  else {
    m_readOnly &= (uint16_t)~bit;
    UpdateDriveInfo(m_currDisk);
    if (m_drives[m_currDisk].m_disk.m_spt > 0)
      PublishDPB(m_currDisk);
    else {
      m_proc.m_cpu.HL.W = 0;
      m_proc.m_cpu.AF.B.h = 0;
    }
  }
}

static bool NameMatches(const std::string & name, const uint8_t * fcb);

void NewBDOS::DeleteFile()
{
  m_proc.m_cpu.AF.B.h = 0xff;
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  int drive = DriveFromFCB(fcb);
  CpmDrive & slot = m_drives[drive];
  if (slot.m_kind != CpmDrive::Kind::eHost) {
    m_debug << "BDOS 19: delete refused on image drive" << endl;
    return;
  }

  UpdateDriveInfo(drive);
  bool deleted = false;
  std::vector<std::pair<std::string, std::string>> matches;
  for (const auto & item : slot.m_host.m_cpmToNative) {
    if (NameMatches(item.first, fcb))
      matches.push_back(item);
  }
  for (const auto & item : matches) {
    std::string path = slot.m_host.m_dir + item.second;
    if (unlink(path.c_str()) == 0) {
      deleted = true;
      m_debug << "BDOS 19: deleted '" << path << "'" << endl;
    }
    else {
      m_debug << "BDOS 19: delete '" << path << "' failed: " << strerror(errno) << endl;
    }
  }
  if (deleted)
    m_proc.m_cpu.AF.B.h = 0;
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
  if (m_drives[drive].m_kind == CpmDrive::Kind::eImage) {
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

  // A CP/M open is used for reading and writing. Fall back to read-only
  // when the host file cannot be modified.
  FileInfo fileInfo;
  fileInfo.m_fn = fn;
  fileInfo.m_fd = open(fn.c_str(), O_RDWR | O_BINARY);
  if (fileInfo.m_fd < 0)
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
  m_debug << "BDOS 16: close file  " << fd << endl;
  auto r = m_fileMap.find(fd);
  if (r == m_fileMap.end()) {
    m_debug << "error: file not found " << endl;
    return;
  }

  if (!r->second.m_isImage && r->second.m_fd >= 0)
    fsync(r->second.m_fd);
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
  if (m_drives[drive].m_kind == CpmDrive::Kind::eImage)
    return SearchImage(drive, fcb, anyUser);

  UpdateDriveInfo(drive);
  if (!m_drives[drive].m_directory.empty())
    return SearchImage(drive, fcb, anyUser);
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

// 11-character CP/M name, with the host basename CP/M programs should create.
static bool HostNameFromFCB(const uint8_t * fcb, std::string & cpm, std::string & base)
{
  std::string name;
  std::string ext;
  for (int i = 0; i < 8; ++i) {
    unsigned char ch = fcb[1 + i] & 0x7f;
    if (ch == '?')
      return false;
    if (ch != ' ')
      name += (char)toupper(ch);
  }
  for (int i = 0; i < 3; ++i) {
    unsigned char ch = fcb[9 + i] & 0x7f;
    if (ch == '?')
      return false;
    if (ch != ' ')
      ext += (char)toupper(ch);
  }
  if (name.empty())
    return false;

  cpm = name;
  while (cpm.size() < 8)
    cpm += ' ';
  while (ext.size() < 3)
    ext += ' ';
  cpm += ext;

  base = name;
  if (ext.find_first_not_of(' ') != std::string::npos) {
    base += '.';
    base += ext.substr(0, ext.find_last_not_of(' ') + 1);
  }
  return base.find('/') == std::string::npos && base.find('\\') == std::string::npos;
}

static bool SequentialOffset(const uint8_t * fcb, off_t & offs)
{
  unsigned cr = fcb[32] & 0x7f;
  unsigned ex = fcb[12] & 0x1f;
  unsigned s2 = fcb[14] & 0x3f;
  unsigned rec = (s2 * 32u + ex) * 128u + cr;
  if (rec >= 65536u)
    return false;
  offs = (off_t)rec * 128;
  return true;
}

static void AdvanceSequential(uint8_t * fcb)
{
  unsigned cr = (fcb[32] & 0x7f) + 1;
  if (cr < 128) {
    fcb[32] = (uint8_t)cr;
    fcb[15] = (uint8_t)cr;
    return;
  }

  fcb[32] = 0;
  fcb[15] = 0x80;
  unsigned ex = (fcb[12] & 0x1f) + 1;
  if (ex < 32) {
    fcb[12] = (uint8_t)ex;
    return;
  }
  fcb[12] = 0;
  fcb[15] = 0;
  fcb[14] = (uint8_t)(((fcb[14] & 0x3f) + 1) & 0x3f);
}

static bool WriteAt(int fd, off_t offs, const uint8_t * data, size_t len)
{
  if (lseek(fd, offs, SEEK_SET) != offs)
    return false;
  size_t done = 0;
  while (done < len) {
    ssize_t n = ::write(fd, data + done, len - done);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (n == 0)
      return false;
    done += (size_t)n;
  }
  return true;
}

void NewBDOS::MakeFile()
{
  m_proc.m_cpu.AF.B.h = 0xff;
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  int drive = DriveFromFCB(fcb);
  CpmDrive & slot = m_drives[drive];
  if (slot.m_kind != CpmDrive::Kind::eHost) {
    m_debug << "BDOS 22: make refused on image drive" << endl;
    return;
  }

  UpdateDriveInfo(drive);
  if (slot.m_host.m_dir.empty() || slot.m_disk.m_spt <= 0)
    return;

  std::string cpm;
  std::string base;
  if (!HostNameFromFCB(fcb, cpm, base))
    return;

  std::string native = base;
  auto existing = slot.m_host.m_cpmToNative.find(cpm);
  if (existing != slot.m_host.m_cpmToNative.end())
    native = existing->second;

  std::string path = slot.m_host.m_dir + native;
  int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (fd < 0) {
    m_debug << "BDOS 22: create '" << path << "' failed: " << strerror(errno) << endl;
    return;
  }

  fcb[12] = 0;
  fcb[13] = 0;
  fcb[14] = 0;
  fcb[15] = 0;
  fcb[32] = 0;
  memset(fcb + 16, 0, 16);
  *(int *)(fcb + eFCB_User) = fd;

  FileInfo info;
  info.m_fn = path;
  info.m_fd = fd;
  info.m_len = 0;
  info.m_pos = 0;
  info.m_drive = drive;
  m_fileMap[fd] = std::move(info);
  info.m_fd = -1;

  slot.m_host.m_cpmToNative[cpm] = native;
  slot.m_host.m_nativeToCPM[native] = cpm;
  m_proc.m_cpu.AF.B.h = 0;
  m_debug << "BDOS 22: created '" << path << "' fd " << fd << endl;
}

void NewBDOS::WriteSeq()
{
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  WriteFile(fcb, 21, -1);
}

void NewBDOS::WriteRandom()
{
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  unsigned rec = fcb[eFCB_R0] | (fcb[eFCB_R1] << 8) | ((unsigned)fcb[eFCB_R2] << 16);
  if (rec >= 65536u) {
    m_proc.m_cpu.AF.B.h = 6;
    return;
  }
  WriteFile(fcb, 34, (off_t)rec * 128);
}

void NewBDOS::WriteFile(uint8_t * fcb, int code, off_t offs)
{
  m_proc.m_cpu.AF.B.h = 1;
  int fd = *(int *)(fcb + eFCB_User);
  auto r = m_fileMap.find(fd);
  if (r == m_fileMap.end()) {
    m_debug << "BDOS " << code << ": unknown file" << endl;
    return;
  }

  FileInfo & info = r->second;
  if (info.m_isImage || info.m_isText || info.m_fd < 0) {
    m_debug << "BDOS " << code << ": write refused" << endl;
    return;
  }

  bool sequential = offs < 0;
  if (sequential && !SequentialOffset(fcb, offs))
    return;
  if (!WriteAt(info.m_fd, offs, m_memory + m_dmaAddress, 128)) {
    m_debug << "BDOS " << code << ": write failed: " << strerror(errno) << endl;
    return;
  }

  if (offs + 128 > info.m_len)
    info.m_len = offs + 128;
  if (sequential) {
    info.m_pos = offs + 128;
    AdvanceSequential(fcb);
  }
  m_proc.m_cpu.AF.B.h = 0;
  m_debug << "BDOS " << code << ": wrote fd " << info.m_fd << " at " << (int)offs << endl;
}

void NewBDOS::SetRandomRecord()
{
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  unsigned cr = fcb[32] & 0x7f;
  unsigned ex = fcb[12] & 0x1f;
  unsigned s2 = fcb[14] & 0x3f;
  unsigned rec = (s2 * 32u + ex) * 128u + cr;
  fcb[eFCB_R0] = (uint8_t)(rec & 0xff);
  fcb[eFCB_R1] = (uint8_t)((rec >> 8) & 0xff);
  fcb[eFCB_R2] = (uint8_t)((rec >> 16) & 0xff);
  m_proc.m_cpu.AF.B.h = 0;
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


static uint32_t ExtentEnd(const uint8_t * entry, int exm)
{
  int extent = entry[12] & 0x1f;
  int module = entry[14] & 0x3f;
  int baseExtent = module * 32 + (extent & ~exm);
  int rc = entry[15];
  int recs = (rc >= 128) ? ((extent & exm) + 1) * 128 : (extent & exm) * 128 + rc;
  return (uint32_t)baseExtent * 128u + (uint32_t)recs;
}

void NewBDOS::ReturnLoginVector()
{
  m_debug << "BDOS 24: login vector " << hex << m_login << dec << endl;
  m_proc.m_cpu.HL.W = m_login;
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::ReturnReadOnlyVector()
{
  m_debug << "BDOS 29: read-only vector " << hex << m_readOnly << dec << endl;
  m_proc.m_cpu.HL.W = m_readOnly;
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::GetAllocVector()
{
  CpmDrive & slot = m_drives[m_currDisk];
  if (slot.m_kind == CpmDrive::Kind::eHost)
    UpdateDriveInfo(m_currDisk);
  slot.RebuildAllocation();
  m_debug << "BDOS 27: allocation vector " << slot.m_alloc.size() << " bytes" << endl;
  if (slot.m_alloc.empty()) {
    m_proc.m_cpu.HL.W = 0;
    m_proc.m_cpu.AF.B.h = 0;
    return;
  }
  size_t n = slot.m_alloc.size();
  if (n > (size_t)(CCPB - 0x100))
    n = (size_t)(CCPB - 0x100);
  uint16_t addr = (uint16_t)(CCPB - n);
  memcpy(m_memory + addr, slot.m_alloc.data(), n);
  m_proc.m_cpu.HL.W = addr;
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::ComputeFileSize()
{
  uint8_t * fcb = m_memory + m_proc.m_cpu.DE.W;
  int drive = DriveFromFCB(fcb);
  CpmDrive & slot = m_drives[drive];
  if (slot.m_kind == CpmDrive::Kind::eHost)
    UpdateDriveInfo(drive);

  uint32_t records = 0;
  bool found = false;
  int entries = slot.m_disk.m_drm + 1;
  bool anyUser = fcb[0] == '?';
  for (int index = 0; index < entries; ++index) {
    size_t off = (size_t)index * 32;
    if (off + 32 > slot.m_directory.size())
      break;
    const uint8_t * entry = slot.m_directory.data() + off;
    if (entry[0] == 0xe5)
      continue;
    if (!anyUser && (entry[0] & 0x1f) != (m_userCode & 0x1f))
      continue;
    bool same = true;
    for (int i = 1; i <= 11; ++i) {
      if ((entry[i] & 0x7f) != (fcb[i] & 0x7f)) {
        same = false;
        break;
      }
    }
    if (!same)
      continue;
    found = true;
    uint32_t end = ExtentEnd(entry, slot.m_disk.m_exm);
    if (end > records)
      records = end;
  }

  fcb[eFCB_R0] = (uint8_t)(records & 0xff);
  fcb[eFCB_R1] = (uint8_t)((records >> 8) & 0xff);
  fcb[eFCB_R2] = (uint8_t)((records >> 16) & 0xff);
  m_proc.m_cpu.AF.B.h = found ? 0 : 0xff;
  m_debug << "BDOS 35: file size " << records << " records" << endl;
}

void NewBDOS::PrintDriveMap()
{
  for (int drive = 0; drive < 16; ++drive) {
    const CpmDrive & slot = m_drives[drive];
    if (!slot.m_configured && drive != 0)
      continue;

    if (m_drives[drive].m_kind == CpmDrive::Kind::eHost)
      UpdateDriveInfo(drive);

    stringstream strm;
    strm << (char)('A' + drive) << ": ";
    if (slot.m_kind == CpmDrive::Kind::eImage) {
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
    if (slot.m_kind == CpmDrive::Kind::eHost && slot.m_disk.m_spt > 0) {
      int block = 128 << slot.m_disk.m_bsh;
      int capacityKb = (slot.m_disk.m_dsm + 1) * block / 1024;
      strm << ", " << (block / 1024) << "kb blocks, " << capacityKb << "kb";
    }
    strm << "\r\n$";
    PrintCPMString(strm.str().c_str());
    m_debug << "drive map " << strm.str() << endl;
  }
}

bool NewBDOS::ConfigureDrives()
{
  std::string error;
  if (!m_drives.Mount(m_proc.m_options.m_cpmDrives, error, &m_debug)) {
    cerr << "error: " << error << endl;
    return false;
  }
  return true;
}

void NewBDOS::ClearHostCaches()
{
  for (auto & slot : m_drives) {
    if (slot.m_kind != CpmDrive::Kind::eHost)
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
  static const uint16_t kDpbAddress = 0xff80;
  uint8_t bytes[16] = {};
  m_drives[drive].Dpb(bytes);
  memcpy(m_memory + kDpbAddress, bytes, 16);
  m_proc.m_cpu.HL.W = kDpbAddress;
  m_proc.m_cpu.AF.B.h = 0;
}

void NewBDOS::GetDiskParams()
{
  m_debug << "BDOS 31: get disk parameters for " << (char)('A' + m_currDisk) << endl;
  if (m_drives[m_currDisk].m_kind == CpmDrive::Kind::eHost)
    UpdateDriveInfo(m_currDisk);
  if (m_drives[m_currDisk].m_disk.m_spt <= 0) {
    m_proc.m_cpu.HL.W = 0;
    m_proc.m_cpu.AF.B.h = 0;
    return;
  }
  PublishDPB(m_currDisk);
}

bool NewBDOS::ReadLogical(int drive, uint32_t diskRec, uint8_t * dest)
{
  return m_drives[drive].ReadRecord(diskRec, dest);
}

bool NewBDOS::OpenImage(int drive, uint8_t * fcb)
{
  CpmDrive & slot = m_drives[drive];
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
  const CpmDrive & slot = m_drives[drive];
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
  const CpmDrive & slot = m_drives[drive];
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
