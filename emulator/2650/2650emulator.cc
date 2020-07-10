#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <memory.h>

#include "common/misc.h"
#include "2650/2650emulator.h"

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

S2650Emulator::S2650Emulator(const EmulatorInfo * info)
  : Emulator(info)
{
  // don't do anything in constructor as this is created to instantiate devices using Instantiate
  // do it Open instead
}

bool S2650Emulator::Open(const Options & options)
{
  if (!Emulator::Open(options))
    return false;

  m_cpu.reset(new Our2650(*this));
  return true;
}

int S2650Emulator::GetMemorySize() const
{
  return 0x8000;
}

std::string S2650Emulator::GetName() const
{
  return "2650";
}

bool S2650Emulator::Start(int addr)
{
  if (addr < 0)
    addr = GetCPUInfo()->m_resetAddr;

  cout << "resetting 2650 to " << HEXFORMAT0x4(addr) << endl;
  Reset(addr);
  return true;
}

void S2650Emulator::Reset(int addr)
{
  if (addr < 0)
    addr = GetCPUInfo()->m_resetAddr;

  Emulator::Reset(addr);

  if (m_options.m_verbose)
    cerr << "info: setting PC to " << HEXFORMAT0x4(addr) << endl;

  m_cpu->registers.ap = addr;
  m_cpu->registers.na = addr;
}

unsigned S2650Emulator::GetPC() const
{
  return m_cpu->registers.ap;
}

void S2650Emulator::SetPC(unsigned v)
{
  m_cpu->registers.ap = v;
}

void S2650Emulator::SetTrace(bool v)
{
}

int S2650Emulator::Exec(int cycles)
{
  // execute instructions
  // 3 clock cycles per CPU cycle 
  while (cycles > 0) {
    m_cpu->cpu();
    if (m_cpu->cpucycles == 0) {
      cerr << "error: CPU executed zero cycles" << endl;
      return 0;
    }
    cycles -= m_cpu->cpucycles * 3;
  } 

  // return remaining cycles (may be negative)
  return cycles;
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
  //cout << "2650: unknown write to port C" << endl;
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

/*
unsigned char S2650Emulator::ReadPort(unsigned char port)
{
  cout << "2650: unknown read from extended port " << HEXFORMAT0x2(port) << endl;
  return 0x00;
}

void S2650Emulator::WritePort(unsigned char port, unsigned char data)
{
  cout << "2650: unknown write to extended port " << HEXFORMAT0x2(port) << endl;
}
*/

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
  cerr << "PC : " << HEXFORMAT0x4(m_cpu.PC.W) << endl;
  cerr << "SP : " << HEXFORMAT0x4(m_cpu.SP.W) << endl;
  int i = 0;
  for (auto & r : stack) {
    cerr << "STACK + " << dec << setw(2) << i << " : " << HEXFORMAT0x2(r) << endl;
    i += 2; 
  }
  */
}

unsigned S2650Emulator::GetOpcode(unsigned addr, std::vector<uint8_t> & opcodes) const
{
  return 1;
}

std::string S2650Emulator::DecodeOpcode(const std::vector<uint8_t> & code) const
{
  return "";
}

/////////////////////////////////////////////////////////////////////////////////////


