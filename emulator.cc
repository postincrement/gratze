#include "memory.h"

// must include before any STL files due to use of "pair"
#include "zed80.h"

#include "emulator.h"


/////////////////////////////////////////////////////////////////////////////////////

// Needed for Z80 emulator


Emulator * Emulator::g_z80Instance = NULL;

extern "C" {

void PatchZ80(register Z80 *R) { }
word LoopZ80(register Z80 *R)  { return INT_NONE; }
byte DebugZ80(register Z80 *R) { return 0; }

void WrZ80(register word Addr,register byte Value)
{ Emulator::g_z80Instance->WrZ80(Addr, Value); }

byte RdZ80(register word Addr)
{ return Emulator::g_z80Instance->RdZ80(Addr); }

void OutZ80(register word Port,register byte Value)
{ Emulator::g_z80Instance->OutZ80(Port, Value); }

byte InZ80(register word Port)
{ return Emulator::g_z80Instance->InZ80(Port); }

};

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator()
{
  // initialize emulator
  memset(&m_cpu, 0, sizeof(m_cpu)); 

  // allow us to find ourselves
  m_cpu.User = (void *)this;
}

bool Emulator::Open()
{
  return true;
}

bool Emulator::OpenVideo(int rows, int cols, MemoryMappedVideo::Font * font)
{
  m_video.reset(new MemoryMappedVideo(rows, cols, 2, font));
  if (!m_video->Open()) {
    return false;
  }

  return true;
}

void Emulator::WriteVideoChar(unsigned int offset, uint8_t ch)
{
  if (m_video)
    m_video->WriteChar(offset, ch);
}


bool Emulator::Run()
{
  return true;
}

void Emulator::WrZ80(register uint16_t Addr,register uint8_t Value)
{

}

uint8_t Emulator::RdZ80(register uint16_t Addr)
{
  return 0;
}

void Emulator::OutZ80(register uint16_t Port, register uint8_t Value)
{
}

uint8_t Emulator::InZ80(register uint16_t Port)
{
  return 0;
}
