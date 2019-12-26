#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>

#include <SDL2/SDL_keyboard.h> 

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

    virtual bool Open(int argc, char *argv[]);

    virtual bool Start(uint16_t addr = 0);

    virtual bool Run();

    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);

    virtual void OnKeyDown(SDL_Keysym & keysym);
    virtual void OnKeyUp(SDL_Keysym & keysym);

    static Emulator * g_z80Instance;

    virtual uint8_t RdZ80(register uint16_t Addr);
    virtual void WrZ80(register uint16_t Addr,register uint8_t Value);

    virtual uint8_t InZ80(register uint16_t Port);
    virtual void OutZ80(register uint16_t Port, register uint8_t Value);

    virtual uint8_t ReadNull(uint16_t);
    virtual void WriteNull(uint16_t, uint8_t);

    //bool ReadROM(const std::string & filename, int addr, int len = -1);

    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

  protected:
    bool OpenVideo(int rows, int cols, MemoryMappedVideo::Font * font);
    std::unique_ptr<MemoryMappedVideo> m_video;

    Z80 m_cpu;
};



#endif // EMULATOR_H_