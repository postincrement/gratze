#ifndef ROMDEF_H_
#define ROMDEF_H_

class SHA256
{
  public:
    SHA256()
    {
    }

    bool Open()
    {
      m_hash = mhash_init(MHASH_SHA256);
      return m_hash != MHASH_FAILED;
    }

    bool Add(const uint8_t * data, int len)
    {
      mhash(m_hash, data len);
      return true;
    }

    bool Close()
    {
      mhash_deinit(hash, m_result);

    }

       printf("Hash:");
       for (i = 0; i < mhash_get_block_size(MHASH_MD5); i++) {
               printf("%.2x", hash[i]);
       }
       printf("\n");


       }

       mhash_deinit(td, hash);

    }



  protected: 
    MHASH m_hash;
    uint8_t m_result[256 / 8];
};


struct ROMDefinition
{
  const char * m_title;
  const char * m_shortName;
  unsigned     m_size_bytes;
  const char * m_sha256;

  const char * m_baseFilename;
  const char * m_sourceURL;
  const char * m_datasheetURL;
};

struct DownloadedROM 
{
  ROMDefinition * m_def = nullptr;
  std::shared_ptr<std::vector<uint8_t>> m_data;

  bool Load();
};

bool DownloadedROM::Load()
{
  // 
}

static Factory<std::string, DownloadedROM> g_downloadedROMs;

std::unique_ptr<DownloadedROM> GetROM(const ROMDefinition & def)
{
  if (g_downloadedROMs.Has)

}



struct ROMDefinition g_mcm6574 = {
  "MCM6574 Character ROM",
  "MCM6574",
  "mcm6574"
  ""
};



#endif // ROMDEF_H_
