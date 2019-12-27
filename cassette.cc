#include <functional>
#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cassette.h"

using namespace std;

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
  Close();
}

struct Guard
{
  Guard(std::function<void ()> handler)
    : m_handler(handler)
  { }

  void Cancel()
  {
    m_handler = nullptr;
  }

  ~Guard()
  {
    if (m_handler)
      m_handler();
  }

  std::function<void ()> m_handler;
};

bool VirtualCassetteFile::Open(const std::string & name, bool reading)
{
  Guard guard([&]() { Close(); });

  m_name    = name;
  m_reading = reading;
  m_format  = Format::eUnknown;

  // see if file exists
  bool exists = ::access(name.c_str(), 0) == 0;
  if (reading != exists) {
    cerr << "error: '" << name << "'" << (reading ? " does not exist" : "already exists") << endl;
    return false;
  }
  
  m_fd = ::open(name.c_str(), O_BINARY | (reading ? O_RDONLY : O_WRONLY | O_CREAT));
  if (m_fd < 0) {
    cerr << "error: cannot " << (reading ? "open" : "create") << " '" << name << "' - " << strerror(errno) << endl;
    return false;
  }

  // identify the file format from the name
  std::string extension;
  size_t pos = name.rfind('.');
  if (pos != std::string::npos) {
    extension = name.substr(pos+1);
    for (auto & r : extension) r = tolower(r);
    if (extension == "wav")
      m_format     = Format::eWAV;
    else if (extension == "cpt")  
      m_format     = Format::eCPT;
  }
  if (m_format != Format::eUnknown) {
    cerr << "info: file '" << name << "' set to format '" << g_formatNames[(int)m_format] << "' using file extension" << endl;
  }

  off_t len = 0;

  if (!reading) {
    if (m_format == Format::eUnknown) {
      m_format = Format::eCAS;
      cerr << "warning: cannot identify filename extension " << extension << " assuming " << g_formatNames[(int)m_format] << endl;
    }
  }
  else {
    // get size of file
    len = lseek(m_fd, 0, SEEK_END);
    lseek(m_fd, 0, SEEK_SET);

    m_rawFile.resize(len);
    ::read(m_fd, &m_rawFile[0], len);

    if (m_format == Format::eUnknown) {

      // read first 512 bytes of file
      if (len < 4) {
        cerr << "error: file is implausibly short" << endl;
        return false;
      }

      // see if WAV file (RIFF header)
      if (memcmp(&m_rawFile[0], "RIFF", 4) == 0) {
        m_format = Format::eWAV;
      }
      else if (len > (256 + 10)) {
        int i;
        for (i = 0; i < len-1; ++i) {
          if (m_rawFile[i] != 0x00)
            break;
        }
        if ((i < (len-1)) && (i > 200) && (m_rawFile[i] == 0xa5))
          m_format = Format::eCAS;
      }

      if (m_format != Format::eUnknown) {
        cerr << "info: file '" << name << "' set to format '" << g_formatNames[(int)m_format] << "' using inspection" << endl;
      }
    }
    lseek(m_fd, 0, SEEK_SET);

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
  }

  if (!reading)
    guard.Cancel();

  return true;
}

void VirtualCassetteFile::WriteByte(int val)
{
  if (m_reading)
    return;

  m_rawFile.push_back(val);
}

void VirtualCassetteFile::Close()
{
  if (m_fd >= 0) {
    if (!m_reading && m_rawFile.size() > 0)
      ::write(m_fd, &m_rawFile[0], m_rawFile.size());
    ::close(m_fd);
    m_fd = -1;
  }
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
