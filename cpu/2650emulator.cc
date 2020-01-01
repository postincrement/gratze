#include <iostream>
#include <iomanip>
#include <unistd.h>

#include "cpu/2650emulator.h"

using namespace std;

class Our2650 : public CPU2650
{
  public:
    Our2650(S2650Emulator & emulator)
      : m_emulator(emulator)
    {}

    virtual unsigned char ReadPortC() override
    {
      return m_emulator.ReadPortC();
    }

    virtual void WritePortC(unsigned char data) override
    {
      m_emulator.WritePortC(data); 
    }

    virtual unsigned char ReadPortD()
    {
      return m_emulator.ReadPortD();
    }

    virtual void WritePortD(unsigned char data) override
    {
      m_emulator.WritePortD(data); 
    }

    virtual unsigned char ReadExtPort(unsigned char port)
    {
      return m_emulator.ReadPort(port);
    }

    virtual void WritePortExt(unsigned char port, unsigned char data) override
    {
      m_emulator.WritePort(port, data); 
    }

    virtual unsigned char ReadMemory(unsigned short addr)
    { 
      return m_emulator.ReadMemory(addr); 
      }

    virtual void WriteMemory(unsigned short addr, unsigned char data)
    { 
      m_emulator.WriteMemory(addr, data); 
    }

    S2650Emulator & m_emulator;
};

S2650Emulator::S2650Emulator()
{
  m_cpu.reset(new Our2650(*this));
}

bool S2650Emulator::Start(int addr)
{
  if (addr < 0)
    addr = GetStartAddress();
  cout << "resetting 2650 to " << hex << addr << endl;
  Reset(addr);
  return true;
}

void S2650Emulator::Reset(uint16_t addr)
{
  m_cpu->registers.ap = addr;

  m_cycleCounter = 0;
  m_cpuDelayTimer = std::chrono::system_clock::now();
}

void S2650Emulator::SetTrace(bool v)
{
}

bool S2650Emulator::Run(int cycles)
{
  m_cpu->cpu();
  return true;
}

void S2650Emulator::NMI()
{
}

void S2650Emulator::Interrupt(uint16_t vector)
{
}

uint16_t S2650Emulator::ReadMemoryWord(uint16_t addr)
{
  uint8_t msb = ReadMemory(addr + 0);
  uint8_t lsb = ReadMemory(addr + 1);
  return lsb + (msb << 8);
}

unsigned char S2650Emulator::ReadPortC()
{
  cout << "2650: unknown read from port C" << endl;
  return 0x00;
}

void S2650Emulator::WritePortC(unsigned char data)
{
  cout << "2650: unknown write to port C" << endl;
}

unsigned char S2650Emulator::ReadPortD()
{
  cout << "2650: unknown read from port d" << endl;
  return 0x00;
}

void S2650Emulator::WritePortD(unsigned char data)
{
  cout << "2650: unknown write to port D" << endl;
}

unsigned char S2650Emulator::ReadPort(unsigned char port)
{
  cout << "2650: unknown read from port 0x" << hex << (int)port << endl;
  return 0x00;
}

void S2650Emulator::WritePort(unsigned char port, unsigned char data)
{
  cout << "2650: unknown write to port 0x" << hex << (int)port << endl;
}


void S2650Emulator::GetStack(std::vector<uint16_t> & stack)
{
  /*
  uint16_t sp = m_cpu.SP.W;
  for (auto & r : stack) {
    r = ReadMemoryWord(sp);
    sp += 2;
  }
  */

}

void S2650Emulator::DumpStackInternal(const std::vector<uint16_t> & stack)
{
  /*
  cerr << "----------" << endl;
  cerr << "PC : 0x" << setw(4) << setfill('0') << hex << m_cpu.PC.W << endl;
  cerr << "SP : 0x" << setw(4) << setfill('0') << hex << m_cpu.SP.W << endl;
  int i = 0;
  for (auto & r : stack) {
    cerr << "STACK + " << dec << setw(2) << i << " : 0x" << setw(4) << setfill('0') << hex << r << endl;
    i += 2; 
  }
  */
}

/////////////////////////////////////////////////////////////////////////////////////


