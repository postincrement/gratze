#include "memory.h"

#include <fstream>
#include <iostream>
#include <iomanip>

#include "config.h"
#include "emulator.h"
#include "fdc.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator()
{
}

bool Emulator::Open(const Options & options)
{
  for (auto &r : options.m_driveFns) {
    std::string fn(r.second);
    VirtualDriveFile *drive = new VirtualDriveFile();
    if (!drive->Open(fn, true))
      return false;
    if (!MountDrive(r.first, drive, true))
      return false;
    cerr << "info: mounted '" << fn << " as drive " << r.first << endl;
  }

  return true;
}

bool Emulator::SetRAMSize_k(int len)
{
  m_ram.resize(len * 1024);
  m_ramSize = m_ram.size();
  m_ramMask = (m_ramSize - 1);

  cout << "info: RAM size " << len << " k, " << m_ramSize << " bytes, " << hex << m_ramMask << endl;
} 

int Emulator::GetRAMSize_k() const
{
  return (m_ramSize + 1023) / 1024;
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::OpenVideo(int rows, int cols, MemoryMappedVideo::Font *font)
{
  m_video.reset(new MemoryMappedVideo(rows, cols, 2, font));
  if (!m_video->Open(GetTitle()))
  {
    return false;
  }

  m_video->SetLazyUpdate(true);

  return true;
}

void Emulator::WriteVideoChar(unsigned int offset, uint8_t ch)
{
  if (m_video)
    m_video->WriteChar(offset, ch);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::OnKeyDown(SDL_Keysym &keysym)
{
}

void Emulator::OnKeyUp(SDL_Keysym &keysym)
{
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::MountDrive(int driveNum, VirtualDrive *drive, bool readOnly)
{
  return m_fdc.MountDrive(driveNum, drive, readOnly);
}

/////////////////////////////////////////////////////////////////////////////////////

uint8_t Emulator::ReadNull(uint16_t)
{
  return 0x00;
}

void Emulator::WriteNull(uint16_t, uint8_t)
{
}

uint8_t Emulator::ReadLog(uint16_t addr)
{
  cerr << "READ 0x" << std::setw(4) << std::setfill('0') << hex << addr << endl;
  return 0x00;
}

void Emulator::WriteLog(uint16_t addr, uint8_t val)
{
  cerr << "WRITE 0x" << std::setw(4) << std::setfill('0') << hex << addr << " 0x" << std::setw(2) << hex << (int)val << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::ReadROMFromFile(const std::string &filename, unsigned char *ptr, int len)
{
  ifstream file(filename.c_str(), ifstream::in | ifstream::binary);
  if (!file.is_open())
  {
    cerr << "error: cannot read ROM file '" << filename << "'" << endl;
    return false;
  }

  if (len <= 0)
  {
    file.seekg(0, ios::end);
    len = file.tellg();
    file.seekg(0, ios::beg);
  }

  file.read((char *)ptr, len);

  return !file.fail();
}

void Emulator::DumpStack(const std::vector<uint16_t> & stack)
{
  DumpStackInternal(stack);
}

void Emulator::DumpStack(int count)
{
  std::vector<uint16_t> stack;
  stack.resize(count);
  GetStack(stack);
  DumpStack(stack);
}

void Emulator::Poll()
{
}

/////////////////////////////////////////////////////////////////////////////////////

Z80Emulator::Z80Emulator()
{
  g_z80Instance = this;

  // initialize emulator
  memset(&m_cpu, 0, sizeof(m_cpu));

  // allow us to find ourselves
  m_cpu.User = (void *)this;
}

bool Z80Emulator::Start(uint16_t addr)
{
  cout << "resetting Z80 to " << hex << addr << endl;
  Reset(addr);
  return true;
}

void Z80Emulator::Reset(uint16_t addr)
{
  ResetZ80(&m_cpu);
  m_cpu.PC.W       = addr;
  m_cpu.TrapBadOps = 1;
  m_cpu.Trap       = 0xffff;

  m_fdc.Reset();
}

bool Z80Emulator::Run()
{
  //if (m_cpu.Trace)
  //  DebugZ80(&m_cpu);

  ExecZ80(&m_cpu, 3);
}

void Z80Emulator::SetTrace(bool v)
{
  m_cpu.Trace = v ? 1 : 0;
}

void Z80Emulator::Execute()
{
  // run emulator
  int count = 0;
  for (;;) {
    //if (emulator->m_cpu.PC.W == options.m_breakpoint)
    //  emulator->SetTrace(true);

    // give CPU some time
    Run();

    // look for events
    if (count++ > 40) {
      Poll();
      m_video->Update(false);
      count = 0;
      SDL_Event event;
      if (SDL_PollEvent(&event)) {
        switch (event.type) { 
          case SDL_KEYDOWN:
            if (event.key.repeat == 0)
              OnKeyDown(event.key.keysym);
            break;

          case SDL_KEYUP:
            if (event.key.repeat == 0)
              OnKeyUp(event.key.keysym);
            break;

          default:
            break;
        }
      }
    }
  }
}

void Z80Emulator::NMI()
{
  IntZ80(&m_cpu, INT_NMI);
}

void Z80Emulator::Interrupt(uint16_t vector)
{
  IntZ80(&m_cpu, vector);
}

uint16_t Z80Emulator::ReadMemoryWord(uint16_t addr)
{
  uint8_t lsb = RdZ80(addr + 0);
  uint8_t msb = RdZ80(addr + 1);
  return lsb + (msb << 8);
}

void Z80Emulator::GetStack(std::vector<uint16_t> & stack)
{
  uint16_t sp = m_cpu.SP.W;
  for (auto & r : stack) {
    r = ReadMemoryWord(sp);
    sp += 2;
  }
}

void Z80Emulator::DumpStackInternal(const std::vector<uint16_t> & stack)
{
  cerr << "----------" << endl;
  cerr << "PC : 0x" << setw(4) << setfill('0') << hex << m_cpu.PC.W << endl;
  cerr << "SP : 0x" << setw(4) << setfill('0') << hex << m_cpu.SP.W << endl;
  int i = 0;
  for (auto & r : stack) {
    cerr << "STACK + " << dec << setw(2) << i << " : 0x" << setw(4) << setfill('0') << hex << r << endl;
    i += 2; 
  }
}

/////////////////////////////////////////////////////////////////////////////////////


Z80Emulator * Z80Emulator::g_z80Instance = NULL;

extern "C"
{
  void PatchZ80(register Z80 *R) {}

  word LoopZ80(register Z80 *R)
  {
    cerr << "loop called" << endl;
    return INT_NONE;
  }

  void WrZ80(register word Addr, register byte Value)
  {
    Z80Emulator::g_z80Instance->WrZ80(Addr, Value);
  }

  byte RdZ80(register word Addr)
  {
    return Z80Emulator::g_z80Instance->RdZ80(Addr);
  }

  void OutZ80(register word Port, register byte Value)
  {
    Z80Emulator::g_z80Instance->OutZ80(Port, Value);
  }

  byte InZ80(register word Port)
  {
    return Z80Emulator::g_z80Instance->InZ80(Port);
  }

  extern byte DebugZ80(Z80 *R);
};


void Z80Emulator::WrZ80(register uint16_t Addr, register uint8_t Value)
{
}

uint8_t Z80Emulator::RdZ80(register uint16_t Addr)
{
  return 0;
}

void Z80Emulator::OutZ80(register uint16_t Port, register uint8_t Value)
{
}

uint8_t Z80Emulator::InZ80(register uint16_t Port)
{
  return 0xff;
}
