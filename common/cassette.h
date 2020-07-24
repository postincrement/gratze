#ifndef CASSETTE_H_
#define CASSETTE_H_

#include <string>
#include <vector>

#include "common/config.h"

class VirtualCassetteFile
{
  public:
    // update g_formatNames in fdc.cc if this is changed
    enum class Encoding {
      eUnknown,
      eCAS,     // bytes being encoded
      eCPT,     // pulse train with 1ms resolution
      eWAV,     // wav file recorded from tape
      eCount
    };

    enum class Format {
      eUnknown,
      eTRS80System,
      eTRS80BAS,
    };

    VirtualCassetteFile();
    ~VirtualCassetteFile();

    bool ReadOpen(const std::string & fn);
    bool WriteOpen();
    bool WriteClose(const std::string & fn);

    void ReadCAS(std::stringstream & formatError);
    void ReadCPT(std::stringstream & formatError);
    void ReadWAV(std::stringstream & formatError);

    bool IsReading() const;

    void WriteByte(uint8_t val);
    uint8_t ReadByte();

    const std::vector<uint8_t> & GetRawData() const;

    std::string GetFilename() const;
    Format GetFormat() const;
    size_t FindHeader() const;

  protected:
    Format IdentifyFormat() const;
    static Encoding EncodingFromExtension(const std::string & name);

    std::string m_name;
    bool m_reading;
    int m_fd;
    Encoding m_encoding;
    Format m_format;
    std::vector<uint8_t> m_rawFile;
    size_t m_readPtr;
};

#endif // CASSETTE_H_
