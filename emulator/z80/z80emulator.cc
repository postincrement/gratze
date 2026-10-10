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

  if (m_options.m_verbose)
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
  m_cpu.IAutoReset = 1;
  m_cpu.IRequest   = INT_NONE;
}

void Z80Emulator::SetTrace(bool v)
{
  
  //m_cpu.Trace = v ? 1 : 0;
}

int Z80Emulator::Exec(int cycles)
{
  ServicePendingInterrupt();
  m_inExec = true;
  int left;
  if (!m_options.m_trace && !m_options.m_logPC && (m_options.m_traceLength == 0))
    left = ExecZ80(&m_cpu, cycles);
  else {
    int toDo = cycles;
    while (toDo > 0) {
      if (m_options.m_traceLength > 0)
        AddTraceInfo(GetPC());
      if (m_options.m_logPC)
        cerr << "pc: " << HEXFORMAT0x4(GetPC()) << endl;
      if (m_options.m_trace)
        cout << Emulator::Disassemble(GetPC()) <<endl;
      int remaining = ExecZ80(&m_cpu, 1);
      int cyclesDone = 1 - remaining;
      toDo -= cyclesDone;
    }
    left = toDo;
  }
  m_inExec = false;
  ServicePendingInterrupt();
  return left;
}

void Z80Emulator::NMI()
{
  IntZ80(&m_cpu, INT_NMI);
}

void Z80Emulator::ServicePendingInterrupt()
{
  if (m_cpu.IRequest == INT_NONE || m_cpu.IRequest == INT_QUIT)
    return;
  // The instruction after EI still has to finish. ExecZ80 takes IRequest
  // when that instruction ends.
  if (m_cpu.IFF & IFF_EI)
    return;
  if ((m_cpu.IFF & IFF_1) == 0)
    return;

  MFZ::word vector = m_cpu.IRequest;
  m_cpu.IRequest = INT_NONE;
  IntZ80(&m_cpu, vector);
}

void Z80Emulator::Interrupt(uint16_t vector)
{
  // A strobe from inside OUT or IN must be taken after that instruction.
  // IntZ80 here would push the PC of the instruction still in progress.
  if (m_inExec) {
    m_cpu.IRequest = vector;
    m_cpu.ICount = 0;
    return;
  }
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

void Z80Emulator::SetPC(unsigned v)
{
  m_cpu.PC.W = v;
}


static uint8_t g_op1_len[256] = {
  /* 0x00 */  1, 3, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 2, 1,
  /* 0x10 */  2, 3, 1, 1,   1, 1, 1, 1,   2, 1, 1, 1,   1, 1, 1, 1,
  /* 0x20 */  2, 3, 3, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x30 */  2, 3, 3, 1,   1, 1, 1, 1,   1, 1, 3, 1,   1, 1, 2, 1,
  /* 0x40 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x50 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x60 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x70 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x80 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0x90 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0xa0 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0xb0 */  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0xc0 */  1, 1, 1, 3,   1, 1, 1, 1,   1, 1, 1, 1,   1, 3, 1, 1,
  /* 0xd0 */  1, 1, 1, 2,   1, 1, 2, 1,   1, 1, 3, 2,   1, 1, 1, 1,
  /* 0xe0 */  1, 1, 1, 1,   1, 1, 2, 1,   1, 1, 1, 1,   1, 1, 1, 1,
  /* 0xf0 */  1, 1, 1, 1,   1, 1, 2, 1,   1, 1, 1, 1,   1, 1, 2, 1
};

static uint8_t g_ixiyOp_len[256] = {
  /* 0x00 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x10 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x20 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x30 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x40 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x50 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x60 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x70 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x80 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x90 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xa0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xb0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xc0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xd0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xe0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xf0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2
};

static uint8_t g_edOp_len[256] = {
  /* 0x00 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x10 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x20 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x30 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x40 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x50 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x60 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x70 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x80 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0x90 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xa0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xb0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xc0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xd0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xe0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,
  /* 0xf0 */  2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2
};

unsigned Z80Emulator::GetOpcode(unsigned addr, std::vector<uint8_t> & opcodes) const
{
  uint8_t op1 = ReadMemory(addr++);
  opcodes.push_back(op1);
  int len;

  uint8_t op2;

  switch (op1) {
    case 0xcb:
      len = 2;
      break;

    case 0xed:
      op2 = ReadMemory(addr);
      opcodes.push_back(op2);
      len = g_edOp_len[op2]-1;
      break;

    case 0xdd:
    case 0xfd:
      op2 = ReadMemory(addr);
      opcodes.push_back(op2);
      len = g_ixiyOp_len[op2]-1;
      break;

    default:
      len = g_op1_len[op1];
  }

  while (--len > 0)
    opcodes.push_back(ReadMemory(addr++));

  return opcodes.size();  
}

std::string Z80Emulator::DecodeOpcode(const std::vector<uint8_t> & code) const
{
  char str[20];
  MFZ::DAsm(str, (uint8_t *)&code[0]);
  return str;
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
