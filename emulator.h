#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>

extern "C" {
#define LSB_FIRST
#include "mfz80/Z80.h"
};

#include "sdl_video.h"
#include <string>

class Emulator
{
  public:
    Emulator();

    virtual bool Open();
    virtual bool Start() = 0;

    virtual bool Run();

    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);

    static Emulator * g_z80Instance;

    virtual void WrZ80(register uint16_t Addr,register uint8_t Value);

    virtual uint8_t RdZ80(register uint16_t Addr);

    virtual void OutZ80(register uint16_t Port, register uint8_t Value);

    virtual uint8_t InZ80(register uint16_t Port);

  protected:
    bool OpenVideo(int rows, int cols, MemoryMappedVideo::Font * font);
    std::unique_ptr<MemoryMappedVideo> m_video;

    Z80 m_cpu;
};



#endif // EMULATOR_H_