#include <iostream>
#include <iomanip>
#include <unistd.h>


#include "cpu/z80emulator.h"

using namespace std;

Z80Emulator::Z80Emulator()
{
  g_z80Instance = this;

  // initialize emulator
  memset(&m_cpu, 0, sizeof(m_cpu));

  // allow us to find ourselves
  m_cpu.User = (void *)this;
}

bool Z80Emulator::Start(int addr)
{
  if (addr < 0)
    addr = GetStartAddress();
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

  m_cycleCounter = 0;
  m_cpuDelayTimer = std::chrono::system_clock::now();
}

void Z80Emulator::SetTrace(bool v)
{
  m_cpu.Trace = v ? 1 : 0;
}


bool Z80Emulator::Run(int cycles)
{
  if (m_cpu.Trace) {
    ExecZ80(&m_cpu, 1);
    return false;
  }

  // full speed
#if 0  
  m_cycleCounter += cycles - ExecZ80(&m_cpu, cycles);
#endif  

#define INC  4

  while (cycles > 0) {

    int done = INC - ExecZ80(&m_cpu, INC);
    m_cycleCounter += done;
    cycles -= done;

    if (m_cycleCounter > 100) {
      double interval = std::chrono::duration<double>(std::chrono::system_clock::now() - m_cpuDelayTimer).count();

      if (interval >= 0.001) {
        m_actualCPUClock_Hz = m_cycleCounter / interval;
        m_cpuDelayRepeat = m_cpuDelayRepeat * m_actualCPUClock_Hz / m_targetCPUClock_Hz;

        if (m_cpuDelayRepeat < 1)
          m_cpuDelayRepeat = 1;

        m_cycleCounter = 0;

        m_cpuDelayTimer = std::chrono::system_clock::now();
      }
    }

    for (int i = 0; i < m_cpuDelayRepeat; ++i)
      memset(m_delayBuffer, 0, sizeof(m_delayBuffer));
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
    Z80Emulator::g_z80Instance->WriteMemory(Addr, Value);
  }

  byte RdZ80(register word Addr)
  {
    return Z80Emulator::g_z80Instance->ReadMemory(Addr);
  }

  void OutZ80(register word Port, register byte Value)
  {
    Z80Emulator::g_z80Instance->OutZ80(Port, Value);
  }

  byte InZ80(register word Port)
  {
    return Z80Emulator::g_z80Instance->InZ80(Port);
  }
};

void Z80Emulator::OutZ80(register uint16_t Port, register uint8_t Value)
{
}

uint8_t Z80Emulator::InZ80(register uint16_t Port)
{
  return 0xff;
}
