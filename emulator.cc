#include "memory.h"

#include <fstream>
#include <iostream>
#include <iomanip>

#include "emulator.h"
#include "fdc.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////

// Needed for Z80 emulator


Emulator * Emulator::g_z80Instance = NULL;

extern "C" {

void PatchZ80(register Z80 *R) { }
word LoopZ80(register Z80 *R)  { return INT_NONE; }

void WrZ80(register word Addr,register byte Value)
{ Emulator::g_z80Instance->WrZ80(Addr, Value); }

byte RdZ80(register word Addr)
{ return Emulator::g_z80Instance->RdZ80(Addr); }

void OutZ80(register word Port,register byte Value)
{ Emulator::g_z80Instance->OutZ80(Port, Value); }

byte InZ80(register word Port)
{ return Emulator::g_z80Instance->InZ80(Port); }

extern byte DebugZ80(Z80 *R);

};

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator()
{
  g_z80Instance = this;

  // initialize emulator
  memset(&m_cpu, 0, sizeof(m_cpu)); 

  // allow us to find ourselves
  m_cpu.User = (void *)this;
}

bool Emulator::Open(int argc, char * argv[])
{
  return true;
}

bool Emulator::Start(uint16_t addr)
{
  cout << "resetting Z80 to " << hex << addr << endl;
  ResetZ80(&m_cpu);
  m_cpu.PC.W = addr;
  m_cpu.TrapBadOps = 1;
  return true;
}

bool Emulator::Run()
{
  if(m_cpu.Trace)
    DebugZ80(&m_cpu);

  ExecZ80(&m_cpu);
}

void Emulator::SetTrace(bool v)
{ 
  m_cpu.Trace = v ? 1 : 0; 
}


/////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::OnKeyDown(SDL_Keysym & keysym)
{}

void Emulator::OnKeyUp(SDL_Keysym & keysym)
{}


/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::MountDrive(int driveNum, VirtualDrive * drive, bool readOnly)
{
  return m_fdc.MountDrive(driveNum, drive, readOnly);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::NMI()
{
  IntZ80(&m_cpu, INT_NMI);

}

void Emulator::Interrupt(uint16_t vector)
{
  IntZ80(&m_cpu, vector);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::WrZ80(register uint16_t Addr,register uint8_t Value)
{ }

uint8_t Emulator::RdZ80(register uint16_t Addr)
{
  return 0;
}

void Emulator::OutZ80(register uint16_t Port, register uint8_t Value)
{ }

uint8_t Emulator::InZ80(register uint16_t Port)
{
  return 0xff;
}

uint8_t Emulator::ReadNull(uint16_t)
{
  return 0x00;
}

void Emulator::WriteNull(uint16_t, uint8_t)
{ }

uint8_t Emulator::ReadLog(uint16_t addr)
{
  cerr << "READ 0x" << std::setw(4) << std::setfill('0') << hex << addr << endl;
  return 0x00;
}

void Emulator::WriteLog(uint16_t addr, uint8_t val)
{ 
  cerr << "WRITE 0x" << std::setw(4) << std::setfill('0') << hex << addr << " 0x" << std::setw(2) << hex <<  (int) val << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

//bool Emulator::ReadROM(const std::string & filename, int addr, int len = -1)
//{
//  return ReadROM(filename, (unsigned char *)&m_memory[addr], len);
//}

bool Emulator::ReadROMFromFile(const std::string & filename, unsigned char * ptr, int len)
{
  ifstream file(filename.c_str(), ifstream::in | ifstream::binary);
  if (!file.is_open()) {
    cerr << "error: cannot read ROM file '" << filename << "'" << endl;
    return false;
  }

  if (len <= 0) {
    file.seekg (0, ios::end);
    len = file.tellg();
    file.seekg (0, ios::beg);
  }

  file.read((char *)ptr, len);

  return !file.fail();
}
