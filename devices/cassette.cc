#include <functional>
#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
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

static const char * g_formatNames[(int)VirtualCassetteFile::Format::eCount] = {
  "Unknown",
  "CAS",
  "CPT",
  "WAV"
};


VirtualCassetteFile::VirtualCassetteFile()
{
  m_fd = -1;
}

VirtualCassetteFile::~VirtualCassetteFile()
{
}

VirtualCassetteFile::Format VirtualCassetteFile::FormatFromExtension(const std::string & name)
{ 
  Format format = Format::eUnknown;
  // identify the file format from the name
  std::string extension;
  size_t pos = name.rfind('.');
  if (pos != std::string::npos) {
    extension = name.substr(pos+1);

    for (auto & r : extension) r = tolower(r);

    if (extension == "wav")
      format     = Format::eWAV;
    else if (extension == "cpt")  
      format     = Format::eCPT;
    else if (extension == "cas")  
      format     = Format::eCAS;
  }

  return format;
}

size_t VirtualCassetteFile::FindHeader() const
{
  size_t len = m_rawFile.size();

  // ignore implausibly short files
  if (len < (256 + 10)) {
    cerr << "too short" << endl;
    return std::string::npos;
  }

  size_t i;
  for (i = 0; i < len-1; ++i) {
    if (m_rawFile[i] != 0x00)
      break;
  }

  // it's all headers!
  if (i >= (len-1)) {
    cerr << "all header " << (int)len << endl;
    return std::string::npos;
  }
  
  // it's all headers!
  if (m_rawFile[i] != 0xa5) {
    cerr << "no sync" << endl;
    return std::string::npos;
  }

  return i+1;
}

std::string VirtualCassetteFile::GetFilename() const
{
  std:stringstream name;

  size_t pos = FindHeader();
  if (pos != std::string::npos) {
    size_t remaining = m_rawFile.size() - pos;
    if (remaining > 8) {
      if (
          (m_rawFile[pos+0] == 0xd3) &&
          (m_rawFile[pos+1] == 0xd3) &&
          (m_rawFile[pos+2] == 0xd3) &&
          isalpha(m_rawFile[pos+3])
         ) {
        name << (char)m_rawFile[pos+3] << "_bas.cas";
      }
      else if (m_rawFile[pos+0] == 0x55) {
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
    }
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
  m_name    = name;
  m_reading = true;
  m_format  = Format::eUnknown;
  m_readPtr = 0;

  int fd = ::open(name.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "error: cannot open '" << name << "' - " << strerror(errno) << endl;
    return false;
  }

  // read file
  off_t len = lseek(fd, 0, SEEK_END);
  if (len < 0) {
    cerr << "error: cannot get length of '" << name << "'" << endl;
    return false;
  }
  m_rawFile.resize(len);
  lseek(fd, 0, SEEK_SET);
  ::read(fd, &m_rawFile[0], len);
  ::close(fd);

  if (len < 4) {
    cerr << "error: file is implausibly short" << endl;
    return false;
  }

  m_format = FormatFromExtension(name);
  if (m_format != Format::eUnknown) {
    cerr << "info: file '" << name << "' set to format '" << g_formatNames[(int)m_format] << "' using file extension" << endl;
  }

  // check if CAS file is masquerading as some other format
  if (m_format == Format::eCAS) {

    Format oldFormat = m_format;

    // see if WAV file (RIFF header)
    if (memcmp(&m_rawFile[0], "RIFF", 4) == 0)
      m_format = Format::eWAV;

    else if (FindHeader() != std::string::npos)
      m_format = Format::eCAS;

    if (m_format != oldFormat)
      cerr << "info: file '" << name << "' is really '" << g_formatNames[(int)m_format] << "'" << endl;
  }

  std::stringstream formatError;
  switch (m_format) {
    case Format::eCAS:
      ReadCAS(formatError);
      break;
    case Format::eCPT:
      ReadCPT(formatError);
      break;
    case Format::eWAV:
      ReadWAV(formatError);
      break;
    case Format::eUnknown:
      cerr << "error: cannot identify format of file '" << name << "'" << endl;
      return false;
  }

  if (formatError.str().length() > 0) {
    cerr << "error: file '" << name << "' is not valid for " << g_formatNames[(int)m_format] << " - " << formatError.str() << endl;
    return false;
  }

  return true;
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
