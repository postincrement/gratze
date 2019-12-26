#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <sys/types.h>

#include <SDL2/SDL_keyboard.h> 

extern "C" {
#define LSB_FIRST
#include "mfz80/Z80.h"
};

#include "sdl_video.h"
#include "fdc.h"
#include "options.h"

#include <string>

class Emulator
{
  public:
    Emulator();

    virtual bool Open(const Options & options);

    virtual bool Start(uint16_t addr = 0);

    virtual bool Run();

    virtual void Poll();

    virtual bool MountDrive(int driveNum, VirtualDrive * drive, bool readOnly);

    virtual void WriteVideoChar(unsigned int offset, uint8_t ch);

    virtual void OnKeyDown(SDL_Keysym & keysym);
    virtual void OnKeyUp(SDL_Keysym & keysym);

    static Emulator * g_z80Instance;

    virtual uint8_t RdZ80(register uint16_t Addr);
    virtual void WrZ80(register uint16_t Addr,register uint8_t Value);

    virtual uint8_t InZ80(register uint16_t Port);
    virtual void OutZ80(register uint16_t Port, register uint8_t Value);

    void NMI();
    void Interrupt(uint16_t vector = 0);

    virtual uint8_t ReadNull(uint16_t);
    virtual void WriteNull(uint16_t, uint8_t);

    virtual uint8_t ReadLog(uint16_t);
    virtual void WriteLog(uint16_t, uint8_t);

    //bool ReadROM(const std::string & filename, int addr, int len = -1);

    bool ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len = -1);

    void SetTrace(bool v);

    Z80 m_cpu;

  protected:
    bool OpenVideo(int rows, int cols, MemoryMappedVideo::Font * font);
    std::unique_ptr<MemoryMappedVideo> m_video;

    WD_FDC m_fdc;
    uint8_t m_drvSel;

};



#endif // EMULATOR_H_