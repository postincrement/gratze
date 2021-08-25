#ifndef BINFILE_H_
#define BINFILE_H_

#include <stdint.h>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <sstream>

#include "config.h"
#include "factory.h"

class BINFile;
class VirtualCassetteFile;

using BINFileFactory = Factory<BINFile, std::string, std::string>;


class BINFileIdentifier
{
  public:
    BINFileIdentifier();
    BINFile * Open(const std::string & fn, bool readOnly);
};

/////////////////////////////////////////////////////

class BINFile
{
  public:
    BINFile(const std::string & fn)
      : m_fn(fn)
    {}

    struct Data 
    {
      struct Block {
        Block(size_t len) : m_data(len) {}
        
        uint16_t m_addr;
        std::vector<uint8_t> m_data;
      };

      std::vector<Block> m_blocks;
      unsigned m_dataSize = 0;

      bool SaveBlock(unsigned addr, const uint8_t * data, unsigned len);
    };

    virtual bool Load(Data & data);

    virtual bool Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver) = 0;

    template <class Type>
    static void AddFormat(const std::string & key)
    {
      m_binFileFactory.AddConcreteClass<Type>(key);
    }

    static void GetExtensions(std::vector<std::string> & list);

    virtual bool GetExecAddr(uint16_t & addr);

    static BINFileFactory m_binFileFactory;

  protected:
    std::string m_fn;
    unsigned m_execAddr = 0;
    bool m_hasExec = false;
};

class GRZFile : public BINFile
{
  public:
    GRZFile(const std::string & fn);
    virtual bool Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver);
};

class HEXFile : public BINFile
{
  public:
    HEXFile(const std::string & fn);
    virtual bool Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver);
};

class CASBINFile : public BINFile
{
  public:
    CASBINFile(const std::string & fn);
    virtual bool Load(std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver);
    bool ReadTRS80SystemFile(VirtualCassetteFile & casFile, 
         std::function<bool (unsigned addr, const uint8_t * ptr, unsigned len)> saver); 
};


#endif // BINFILE_H_
