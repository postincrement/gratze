#ifndef TRS80_H_
#define TRS80_H_

#include "emulator.h"

class TRS80Emulator : public Emulator
{
  public:
    TRS80Emulator();

    virtual bool Open() override;
    virtual bool Start() override;

  protected:  
    std::unique_ptr<MemoryMappedVideo::Font> m_font;

};

#endif // TRS80_H_
