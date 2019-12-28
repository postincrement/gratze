#ifndef CASSETTE_H_
#define CASSETTE_H_

#include <string>
#include <vector>

class VirtualCassetteFile
{
  public:
    // update g_formatNames in fdc.cc if this is changed
    enum class Format {
      eUnknown,
      eCAS,     // bytes being encoded
      eCPT,     // pulse train with 1ms resolution
      eWAV,     // wav file recorded from tape
      eCount
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

    std::string GetFilename() const;
    static Format FormatFromExtension(const std::string & name);

  protected:
    size_t FindHeader() const;
    std::string m_name;
    bool m_reading;
    int m_fd;
    Format m_format;
    std::vector<uint8_t> m_rawFile;
    size_t m_readPtr;
};

#endif // CASSETTE_H_
