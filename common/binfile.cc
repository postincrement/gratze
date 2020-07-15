#include <fstream>
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;

#include "binfile.h"
#include "grz.h"
#include "config.h"
#include "misc.h"

class BINFile;

BINFileFactory BINFile::m_binFileFactory;

BINFileIdentifier::BINFileIdentifier()
{
  BINFile::AddFormat<GRZFile>("grz");
  BINFile::AddFormat<HEXFile>("hex");
}

BINFile * BINFileIdentifier::Open(const std::string & path, bool readOnly)
{
  // get extension of file
  std::string fn(path);
  size_t pos = fn.rfind('.');
  if (pos == std::string::npos) {
    cerr << "info: fn '" << fn << "' has no extension" << endl;
    return nullptr;
  }

  std::string extension = fn.substr(pos+1);

  BINFile * file = BINFile::m_binFileFactory.CreateInstance(extension, path);
  if (file == nullptr) {
    cerr << "unknown extension '" << extension << "'" << endl;
    return nullptr;
  }

  return file;  
}

/////////////////////////////////////////////////////////////////////

bool BINFile::Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver)
{
  return false;
}

void BINFile::GetExtensions(std::vector<std::string> & list)
{
  BINFile::m_binFileFactory.GetKeys(list);
}

bool BINFile::GetExecAddr(unsigned & addr)
{
  addr = m_execAddr;
  return m_hasExec;
}

/////////////////////////////////////////////////////////////////////

GRZFile::GRZFile(const std::string & fn)
  : BINFile(fn)
{
}

bool GRZFile::Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver)
{
  std::string json;
  {
    std::ifstream file(m_fn);
    if (!file.is_open()) {
      cout << "cannot open " << m_fn << endl;
      return false;
    }
    std::string line;
    while ( getline(file,line)) {
      json += line;
    }
    file.close();
  }
  cout << "json: " << json << endl;
  GRZ grz;
  std::stringstream strm(json);
  try {
    cereal::JSONInputArchive ar(strm);
    grz.load(ar);
  }
  catch(const std::exception& e) {
    cerr << "load failed - " << e.what() << endl;
    return false;
  }

  std::string dir(m_fn);

  size_t pos = dir.rfind(DIR_SEPERATOR);
  if (pos != std::string::npos)
    dir = dir.substr(0, pos+1);

  std::string fn = dir + grz.m_filename;

  // open file
  int fd = ::open(fn.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "error: cannot open '" << m_fn << "' - " << strerror(errno) << endl;
    return false;    
  }

  // get length
  struct stat stbuf;
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    cerr << "error: '" << m_fn << "' inaccessible or not a regular file" << endl;
    return false;
  }
  off_t len = stbuf.st_size;

  // check offset
  unsigned offset = grz.m_hasOffs ? grz.m_offs : 0;
  if (grz.m_hasOffs && (len < offset)) {
    cerr << "error: file too short for offset" << endl;
    return false;
  }

  // check length
  signed length = (grz.m_hasLength ? grz.m_length : len) - offset;
  if ((length < 0) || ((offset + length) > len)) {
    cerr << "error: file too short for offset + length" << endl;
    return false;
  }

  // allocate and read data
  std::vector<uint8_t> m_data;
  m_data.resize(length);
  lseek(fd, offset, SEEK_SET);
  ::read(fd, &m_data[0], length);
  ::close(fd);

  // copy to memory
  if (!saver(grz.m_addr, &m_data[0], m_data.size()))
    return false;

   cout << "info: loaded " << HEXFORMAT0x4(length) << " bytes to " << HEXFORMAT0x4(grz.m_addr) << endl;
   return true;
}

/////////////////////////////////////////////////////////////////////

HEXFile::HEXFile(const std::string & fn)
  : BINFile(fn)
{
}

static bool ReadHex(unsigned int & value, const char * data, int len)
{
  value = 0;
  while (len-- > 0) {
    char ch = tolower(*data++);
    value = value * 16;
    if ((ch >= '0') && (ch <= '9'))
      value += (unsigned)(ch - '0');
    else if ((ch >= 'a') || (ch <= 'f'))
      value += (unsigned)(ch - 'a' + 10);
    else
      return false;
  }
  return true;
}

static int ReadData(int count, std::vector<uint8_t> & data, const char * str, int len)
{
  if (len < count*2)
    return -1;

  data.resize(count);  

  uint8_t * ptr = &data[0];
  while (count-- > 0) {
    unsigned val;
    if (!ReadHex(val, str, 2))
      return -1;
    *ptr++ = val;
    str += 2;
  }
  return data.size();
}

bool HEXFile::Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver)
{
  std::ifstream file(m_fn);
  if (!file.is_open()) {
    cerr << "error: cannot open " << m_fn << endl;
    return false;
  }

  m_execAddr = 0;
  m_hasExec = false;

  std::string line;
  unsigned lineNumber = 0;
  bool done = false;
  while (!done && getline(file,line)) {
    ++lineNumber;
    if (line.length() < 9) {
      cerr << "error: HEX file error - line " << lineNumber << " too short" << endl;
      continue;
    }
    if (line[0] != ':') {
      cerr << "error: HEX file error - line " << lineNumber << " does not start with ':'" << endl;
      return false;
    }
    else {
      unsigned byteCount;
      if (!ReadHex(byteCount, &line[1], 2)) {
        cerr << "error: HEX file error - line " << lineNumber << " has bad byte count" << endl;
        return false;
      }
      unsigned address;
      if (!ReadHex(address, &line[3], 4)) {
        cerr << "error: HEX file error - line " << lineNumber << " has bad address" << endl;
        return false;
      }
      unsigned recordType;
      if (!ReadHex(recordType, &line[7], 2)) {
        cerr << "error: HEX file error - line " << lineNumber << " has bad record type" << endl;
        return false;
      }

      // read data
      std::vector<uint8_t> data(byteCount);
      int lineLen = line.length()-9;
      char * ptr = &line[9];
      unsigned loadAddr = 0;
      switch (recordType) {
        case 0x00: // data
          if (!ReadData(byteCount, data, ptr, lineLen)) {
            cerr << "error: HEX file error - line " << lineNumber << " has bad data" << endl;
            return false;
          }
          saver(address, &data[0], byteCount);
          break;
        case 0x01: // end of file
          done = true;
          break;
        case 0x05: // exec address
          if (lineLen < 8) {
            cerr << "error: HEX file error - line " << lineNumber << " has dwarf exec address" << endl;
            return false;
          }
          if (!ReadHex(m_execAddr, ptr, 8)) {
            cerr << "error: HEX file error - line " << lineNumber << " has bad data" << endl;
            return false;
          }
          cerr << "info: loaded exec address " << HEXFORMAT0x4(m_execAddr) << endl;
          m_hasExec = true;
          break;
        default:
          cerr << "info: ignoring HEX record " << recordType << endl;
          break;  
      }
    }
  }
  file.close();

  return true;
}
