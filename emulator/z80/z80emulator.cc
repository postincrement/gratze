#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <memory.h>

#define DISABLE_CEREAL

#include "common/misc.h"
#include "z80/z80emulator.h"

#include <fstream>

using namespace std;

Z80Emulator::Z80Emulator(const EmulatorInfo * info)
  : Emulator(info)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool Z80Emulator::Open(const Options & options)
{
  if (!Emulator::Open(options))
    return false;

  g_z80Instance = this;

  // initialize emulator
  memset(&m_cpu, 0, sizeof(m_cpu));

  // allow us to find ourselves
  m_cpu.User = (void *)this;

  return true;
}

int Z80Emulator::GetMemorySize() const
{
  return 0x10000;
}

std::string Z80Emulator::GetName() const
{
  return "z80";
}

std::string Z80Emulator::DumpRegs() const
{
  std::stringstream strm;
  strm << "PC = " << HEXFORMAT0x4(m_cpu.PC.W) << endl << endl;
  return strm.str();
}

bool Z80Emulator::Start(int addr)
{
  if (addr < 0)
    addr = GetCPUInfo()->m_resetAddr;

  cout << "resetting Z80 to " << HEXFORMAT0x4(addr) << endl;
  Reset(addr);
  return true;
}

void Z80Emulator::Reset(int addr)
{
  Emulator::Reset(addr);
  
  if (addr < 0)
    addr = GetCPUInfo()->m_resetAddr;
    
  ResetZ80(&m_cpu);
  m_cpu.PC.W       = addr;
  m_cpu.TrapBadOps = 1;
  m_cpu.Trap       = 0xffff;
}

void Z80Emulator::SetTrace(bool v)
{
  m_cpu.Trace = v ? 1 : 0;
}

int Z80Emulator::Exec(int cycles)
{
  return ExecZ80(&m_cpu, cycles);
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
  uint8_t lsb = ReadMemory(addr + 0);
  uint8_t msb = ReadMemory(addr + 1);
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
  cerr << "PC : " << HEXFORMAT0x4(m_cpu.PC.W) << endl;
  cerr << "SP : " << HEXFORMAT0x4(m_cpu.SP.W) << endl;
  int i = 0;
  for (auto & r : stack) {
    cerr << "STACK + " << dec << setw(2) << i << " :" << HEXFORMAT0x4(r) << endl;
    i += 2; 
  }
}

unsigned Z80Emulator::GetPC() const
{
  return m_cpu.PC.W;
}

/////////////////////////////////////////////////////////////////////////////////////

Z80Emulator * Z80Emulator::g_z80Instance = NULL;

extern "C"
{
  void PatchZ80(register MFZ::Z80 *R) {}

  MFZ::word LoopZ80(register MFZ::Z80 *R)
  {
    cerr << "loop called" << endl;
    return INT_NONE;
  }

  void WrZ80(register MFZ::word Addr, register MFZ::byte Value)
  {
    Z80Emulator::g_z80Instance->WriteMemory(Addr, Value);
  }

  MFZ::byte RdZ80(register MFZ::word Addr)
  {
    return Z80Emulator::g_z80Instance->ReadMemory(Addr);
  }

  void OutZ80(register MFZ::word Port, register MFZ::byte Value)
  {
    Z80Emulator::g_z80Instance->WritePort(Port, Value);
  }

  MFZ::byte InZ80(register MFZ::word Port)
  {
    return Z80Emulator::g_z80Instance->ReadPort(Port);
  }
};
