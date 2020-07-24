#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "common/config.h"
#include "cassette.h"

using namespace std;


/*

Level 2 CSAVE format
--------------------
  0x00 x 255    leader
  0xa5          sync char
  0xd3 x 3      CSAVE ID
  0x??          filename (alphabetic char)
  ....          copy of BASIC data

Level 2 SYSTEM tape format
--------------------------
  0x00 x 255    leader
  0xa5          sync char
  0x55          SYSTEM ID
  0x?? x 6      filename (alphabetic char)

  ....zero or more blocks

  0x78          end ID
  0x??          LSB of execute address
  0x??          MSB of execute address

SYSTEM tape data block
-----------------
  0x3c          data ID
  0x??          length of block (0 = 256 bytes)
  0x??          LSB of block address
  0x??          MSB of block address
  ....          data (1 to 256 bytes)
  0x??          checksum (sum of LSB, MSB, and data)

*/

static const char * g_encodingNames[(int)VirtualCassetteFile::Encoding::eCount] = {
  "Unknown",
  "CAS",
  "CPT",
  "WAV"
};


VirtualCassetteFile::VirtualCassetteFile()
{
  m_fd = -1;
  m_encoding = Encoding::eUnknown;
  m_format = Format::eUnknown;
}

VirtualCassetteFile::~VirtualCassetteFile()
{
}

VirtualCassetteFile::Format VirtualCassetteFile::GetFormat() const
{
  return m_format;
}

VirtualCassetteFile::Encoding VirtualCassetteFile::EncodingFromExtension(const std::string & name)
{
  Encoding encoding = Encoding::eUnknown;

  // identify the file encoding from the name
  std::string extension;
  size_t pos = name.rfind('.');
  if (pos != std::string::npos) {
    extension = name.substr(pos+1);

    for (auto & r : extension) r = tolower(r);

    if (extension == "wav")
      encoding     = Encoding::eWAV;
    else if (extension == "cpt")
      encoding     = Encoding::eCPT;
    else if (extension == "cas")
      encoding     = Encoding::eCAS;
  }

  return encoding;
}

size_t VirtualCassetteFile::FindHeader() const
{
  size_t len = m_rawFile.size();

  // ignore implausibly short files
  if (len < (256 + 10)) {
    return std::string::npos;
  }

  size_t i;
  for (i = 0; i < len-1; ++i) {
    if (m_rawFile[i] != 0x00)
      break;
  }

  // it's all headers!
  if (i >= (len-1)) {
    return std::string::npos;
  }

  // it's all headers!
  if (m_rawFile[i] != 0xa5) {
    return std::string::npos;
  }

  return i+1;
}

VirtualCassetteFile::Format VirtualCassetteFile::IdentifyFormat() const
{
  Format format = Format::eUnknown;

  size_t pos = FindHeader();
  if (pos == std::string::npos) {
    cerr << "error: could not find end of header" << endl;
  }
  else {
    size_t remaining = m_rawFile.size() - pos;
    if (remaining > 8) {
      if (
          (m_rawFile[pos+0] == 0xd3) &&
          (m_rawFile[pos+1] == 0xd3) &&
          (m_rawFile[pos+2] == 0xd3) &&
          isalpha(m_rawFile[pos+3])
         ) {
         format = Format::eTRS80BAS;
      }
      else if (m_rawFile[pos+0] == 0x55) {
        int i = 0;
        while (i < 6) {
          if (!isalnum(m_rawFile[pos+1+i]))
            break;
          i++;
        }
        if (i > 0) {
          format = Format::eTRS80System;
        }
      }
    }
  }

  return format;
}

std::string VirtualCassetteFile::GetFilename() const
{
  std::stringstream name;

  switch (m_format) {
    case Format::eTRS80System:
      {
        size_t pos = FindHeader();
        int i = 0;
        while (i < 6) {
          if (!isalnum(m_rawFile[pos+1+i]))
            break;
          i++;
        }
        if (i > 0) {
          name << std::string((char *)&m_rawFile[pos+1], i) << "_sys.cas";
        }
      }
      break;
    case Format::eTRS80BAS:
      {
        size_t pos = FindHeader();
        name << (char)m_rawFile[pos+3] << "_bas.cas";
      }
      break;
  }

  string str = name.str();
  for (auto & r : str) r = tolower(r);

  return str;
}

bool VirtualCassetteFile::IsReading() const
{
  return m_reading;
}

bool VirtualCassetteFile::WriteOpen()
{
  m_reading = false;
  m_rawFile.clear();
  return true;
}

bool VirtualCassetteFile::ReadOpen(const std::string & name)
{
  m_name       = name;
  m_reading    = true;
  m_encoding   = Encoding::eUnknown;
  m_format     = Format::eUnknown;
  m_readPtr    = 0;

  int fd = ::open(name.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "error: cannot open '" << name << "' - " << strerror(errno) << endl;
    return false;
  }

  // get length
  struct stat stbuf;
  if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    cerr << "error: '" << name << "' inaccessible or not a regular file" << endl;
    return false;
  }  
  off_t len = stbuf.st_size;  

  // read file
  m_rawFile.resize(len);
  ::read(fd, &m_rawFile[0], len);
  ::close(fd);

  if (len < 4) {
    cerr << "error: file is implausibly short" << endl;
    return false;
  }

  m_encoding = EncodingFromExtension(name);
  if (m_encoding != Encoding::eUnknown) {
    cerr << "info: file '" << name << "' set to format '" << g_encodingNames[(int)m_encoding] << "' using file extension" << endl;
  }

  // check if CAS file is masquerading as some other format
  if (m_encoding == Encoding::eCAS) {

    Encoding oldEncoding = m_encoding;

    // see if WAV file (RIFF header)
    if (memcmp(&m_rawFile[0], "RIFF", 4) == 0)
      m_encoding = Encoding::eWAV;

    else if (FindHeader() != std::string::npos)
      m_encoding = Encoding::eCAS;

    if (m_encoding != oldEncoding)
      cerr << "info: file '" << name << "' is '" << g_encodingNames[(int)m_encoding] << "' masquerading as CAS" << endl;
  }

  std::stringstream formatError;
  switch (m_encoding) {
    case Encoding::eCAS:
      ReadCAS(formatError);
      break;
    case Encoding::eCPT:
      ReadCPT(formatError);
      break;
    case Encoding::eWAV:
      ReadWAV(formatError);
      break;
    case Encoding::eUnknown:
      cerr << "error: cannot identify encoding of file '" << name << "'" << endl;
      return false;
  }

  if (formatError.str().length() > 0) {
    cerr << "error: file '" << name << "' is not valid for " << g_encodingNames[(int)m_encoding] << " - " << formatError.str() << endl;
    return false;
  }

  m_format = IdentifyFormat();
  if (m_format == Format::eUnknown) {
    cerr << "error: cannot identify format of file '" << name << "'" << endl;
    return false;
  }

  return true;
}

const std::vector<uint8_t> & VirtualCassetteFile::GetRawData() const
{
  return m_rawFile;
}

void VirtualCassetteFile::WriteByte(uint8_t val)
{
  if (m_reading)
    return;

  m_rawFile.push_back(val);
}

uint8_t VirtualCassetteFile::ReadByte()
{
  if (!m_reading)
    return 0;

  if (m_readPtr >= m_rawFile.size())
    return 0x00;

  return m_rawFile[m_readPtr++];
}


bool VirtualCassetteFile::WriteClose(const std::string & filename)
{
  int fd = ::open(filename.c_str(), O_BINARY | O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    cerr << "error: cannot create file '" << filename << "' - " << strerror(errno) << endl;
    return false;
  }

  int len = ::write(fd, &m_rawFile[0], m_rawFile.size());
  ::close(fd);

  if (len != m_rawFile.size()) {
    cerr << "error: cannot write file '" << filename << "' - " << strerror(errno) << endl;
    return false;
  }

  return true;
}

void VirtualCassetteFile::ReadCAS(std::stringstream & formatError)
{

}

void VirtualCassetteFile::ReadWAV(std::stringstream & formatError)
{
  formatError << "not supported";
}

void VirtualCassetteFile::ReadCPT(std::stringstream & formatError)
{
  formatError << "not supported";
}
