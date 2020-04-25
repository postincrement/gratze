
#include "6800.h"

#include <strings.h>
#include <stdio.h>

#define IRQ_VECTOR      0xfff8      
#define SWI_VECTOR      0xfffa      
#define NMI_VECTOR      0xfffc      
#define RESET_VECTOR    0xfffe      

// e = EXT
// x = INDEX
// i = IMM
// d = DIR

typedef enum {
  // 0x00
  x00,   NOP,   x02,   x03,   x04,   x05,   TAP,   TPA,  
  INX,   DEX,   CLV,   SEV,   CLC,   SEC,   CLI,   SEI,

  // 0x10
  SBA,   CBA,   x12,   x13,   x14,   x15,   TAB,   TBA,  
  x18,   DAA,   x1a,   ABA,   x1c,   x1d,   x1e,   x1f,

  // 0x20
	BRA,   x21,   BHI,   BLS,   BCC,   BCS,   BNE,   BEQ,  
  BVC,   BVS,   BPL,   BMI,   BGE,   BLT,   BGT,   BLE,

  // 0x30
  TSX,   INS,   PULA,  PULB,  DES,   TXS,   PSHA,  PSHB, 
  x38,   RTS,   x3a,   RTI,   x3c,   x3d,   WAI,   SWI,

  // 0x40
  NEGA,  x41,   x42,   COMA,  LSRA,  x45,   RORA,  ASRA, 
  ASLA,  ROLA,  DECA,  x4b,   INCA,  TSTA,  x4e,   CLRA,

  // 0x50
  NEGB,  x51,   x52,   COMB,  LSRB,  x55,   RORB,  ASRB, 
  ASLB,  ROLB,  DECB,  x5b,   INCB,  TSTB,  x5e,   CLRB,

  // 0x60
  NEGi,  x61,   x62,   COMi,  LSRi,  x65,   RORx,  ASRx, 
  ASLx,  ROLx,  DECx,  x6b,   INCx,  TSTx,  JMPx,  CLRx,

  // 0x70
  NEGe,  x71,   x72,   COMe,  LSRe,  x75,   RORe,  ASRe, 
  ASLe,  ROLe,  DECe,  x7b,   INCe,  TSTe,  JMPe,  CLRe,

  // 0x80
  SUBAi, CMPAi, SBCAi, x83,   ANDAi, BITAi, LDAAi, x87,  
  EORAi, ADCAi, ORAAi, ADDAi, CPXAi, BSR,   LDSi,  x8f,

  // 0x90
  SUBAd, CMPAd, SBCAd, x93,   ANDAd, BITAd, LDAAd, x97,  
  EORAd, ADCAd, ORAAd, ADDAd, CPXAd, x9d,   LDSd,  x9f,

  // 0xa0
  SUBAx, CMPAx, SBCAx, xa3,   ANDAx, BITAx, LDAAx, xa7,  
  EORAx, ADCAx, ORAAx, ADDAx, CPXAx, JSRx,  LDSx,  xaf,

  // 0xb0
  SUBAe, CMPAe, SBCAe, xb3,   ANDAe, BITAe, LDAAe, xb7,  
  EORAe, ADCAe, ORAAe, ADDAe, CPXAe, JSRe,  LDSe,  xbf,

  // 0xc0
  SUBBi, CMPBi, SBBAi, xc3,   ANDBi, BITBi, LDABi, xc7,  
  EORBi, ADCBi, ORABi, ADDBi, xcc,   xcd,   LDXi,  xcf,

  // 0xd0
  SUBBd, CMPBd, SBCBd, xd3,   ANDBd, BITBd, LDABd, xd7,  
  EORBd, ADCBd, ORABd, ADDBd, xdc,   xdd,   LDXd,  STXd,

  // 0xe0
  SUBBx, CMPBx, SBCBx, xe3,   ANDBx, BITBx, LDABx, xe7,  
  EORBx, ADCBx, ORABx, ADDBx, xec,   xed,   LDXx,  xef,

  // 0xf0
  SUBBe, CMPBe, SBCBe, xf3,   ANDBe, BITBe, LDABe, xf7,  
  EORBe, ADCBe, ORABe, ADDBe, xfc,   xfd,   LDXe,  xff
} Opcode;

static uint8_t g_lenAndCycles[256][2] = {

  // 0x00
  { 0, 0 }, { 1, 2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 2 }, { 1, 2 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x10
  { 1, 2 }, { 1, 2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 2 }, { 1, 2 },
  { 0, 0 }, { 1, 2 }, { 0, 0 }, { 1, 2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x20
  { 2, 4 }, { 0, 0 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 },
  { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 }, { 2, 4 },

  // 0x30
  { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 }, { 1, 4 },
  { 0, 0 }, { 1, 4 }, { 0, 0 }, { 1, 4 }, { 0, 0 }, { 0, 0 }, { 1, 4 }, { 1, 12 },

  // 0x40
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x50
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x60
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x70
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },

  // 0x80
  { 2, 2 }, { 2, 2 }, { 2, 2 }, { 0, 0 }, { 2, 2 }, { 2, 2 }, { 2, 2 }, { 0, 0 },
  { 2, 2 }, { 2, 2 }, { 2, 2 }, { 2, 2 }, { 3, 3 }, { 2, 8 }, { 3, 3 }, { 0, 0 },

  // 0x90
  { 2, 3 }, { 2, 3 }, { 2, 3 }, { 0, 0 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 4 },
  { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 0, 0 }, { 2, 3 }, { 2, 5 },

  // 0xa0
  { 2, 5 }, { 2, 5 }, { 2, 5 }, { 0, 0 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 },
  { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 3, 3 }, { 2, 8 }, { 2, 5 }, { 2, 5 },

  // 0xb0
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 3, 9 }, { 0, 0 }, { 0, 0 },

  // 0xc0
  { 2, 2 }, { 2, 2 }, { 2, 2 }, { 0, 0 }, { 2, 2 }, { 2, 2 }, { 2, 2 }, { 0, 0 },
  { 2, 2 }, { 2, 2 }, { 2, 2 }, { 2, 2 }, { 0, 0 }, { 0, 0 }, { 3, 3 }, { 0, 0 },

  // 0xd0
  { 2, 3 }, { 2, 3 }, { 2, 3 }, { 0, 0 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 4 },
  { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 0, 0 }, { 0, 0 }, { 2, 3 }, { 2, 5 },

  // 0xe0
  { 2, 5 }, { 2, 5 }, { 2, 5 }, { 0, 0 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 },
  { 2, 5 }, { 2, 5 }, { 2, 5 }, { 2, 5 }, { 0, 0 }, { 0, 0 }, { 2, 5 }, { 2, 5 },

  // 0xf0
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};

static const char * g_mnemonic[256] = {
  "x00",   "NOP",   "x02",   "x03",   "x04",   "x05",   "TAP",   "TPA",  
  "INX",   "DEX",   "CLV",   "SEV",   "CLC",   "SEC",   "CLI",   "SEI",
  "SBA",   "CBA",   "x12",   "x13",   "x14",   "x15",   "TAB",   "TBA" , 
  "x18",   "DAA",   "x1a",   "ABA",   "x1c",   "x1d",   "x1e",   "x1f",
	"BRA",   "x21",   "BHI",   "BLS",   "BCC",   "BCS",   "BNE",   "BEQ" , 
  "BVC",   "BVS",   "BPL",   "BMI",   "BGE",   "BLT",   "BGT",   "BLE",
  "TSX",   "INS",   "PULA",  "PULB",  "DES",   "TXS",   "PSHA",  "PSHB", 
  "x38",   "RTS",   "x3a",   "RTI",   "x3c",   "x3d",   "WAI",   "SWI",
  "NEGA",  "x41",   "x42",   "COMA",  "LSRA",  "x45",   "RORA",  "ASRA", 
  "ASLA",  "ROLA",  "DECA",  "x4b",   "INCA",  "TSTA",  "x4e",   "CLRA",
  "NEGB",  "x51",   "x52",   "COMB",  "LSRB",  "x55",   "RORB",  "ASRB", 
  "ASLB",  "ROLB",  "DECB",  "x5b",   "INCB",  "TSTB",  "x5e",   "CLRB",
  "NEGi",  "x61",   "x62",   "COMi",  "LSRi",  "x65",   "RORx",  "ASRx", 
  "ASLx",  "ROLx",  "DECx",  "x6b",   "INCx",  "TSTx",  "JMPx",  "CLRx",
  "NEGe",  "x71",   "x72",   "COMe",  "LSRe",  "x75",   "RORe",  "ASRe", 
  "ASLe",  "ROLe",  "DECe",  "x7b",   "INCe",  "TSTe",  "JMPe",  "CLRe",
  "SUBAi", "CMPAi", "SBCAi", "x83",   "ANDAi", "BITAi", "LDAAi", "x87",  
  "EORAi", "ADCAi", "ORAAi", "ADDAi", "CPXAi", "BSR",   "LDSi",  "x8f",
  "SUBAd", "CMPAd", "SBCAd", "x93",   "ANDAd", "BITAd", "LDAAd", "STAAd",  
  "EORAd", "ADCAd", "ORAAd", "ADDAd", "CPXAd", "x9d",   "LDSd",  "STSd",
  "SUBAx", "CMPAx", "SBCAx", "xa3",   "ANDAx", "BITAx", "LDAAx", "STAAx",  
  "EORAx", "ADCAx", "ORAAx", "ADDAx", "CPXAx", "JSRx",  "LDSx",  "STXx",
  "SUBAe", "CMPAe", "SBCAe", "xb3",   "ANDAe", "BITAe", "LDAAe", "STAAe",  
  "EORAe", "ADCAe", "ORAAe", "ADDAe", "CPXAe", "JSRe",  "LDSe",  "STSe",
  "SUBBi", "CMPBi", "SBBAi", "xc3",   "ANDBi", "BITBi", "LDABi", "xc7",  
  "EORBi", "ADCBi", "ORABi", "ADDBi", "xcc",   "xcd",   "LDXi",  "xcf",
  "SUBBd", "CMPBd", "SBCBd", "xd3",   "ANDBd", "BITBd", "LDABd", "STABd",  
  "EORBd", "ADCBd", "ORABd", "ADDBd", "xdc",   "xdd",   "LDXd",  "STXd",
  "SUBBx", "CMPBx", "SBCBx", "xe3",   "ANDBx", "BITBx", "LDABx", "STABx",  
  "EORBx", "ADCBx", "ORABx", "ADDBx", "xec",   "xed",   "LDXx",  "STXx",
  "SUBBe", "CMPBe", "SBCBe", "xf3",   "ANDBe", "BITBe", "LDABe", "STABe",  
  "EORBe", "ADCBe", "ORABe", "ADDBe", "xfc",   "xfd",   "LDXe",  "STXe"
};

#define LOBYTE(v)   ((v) & 0xff)
#define HIBYTE(v)   ((v) >> 8)

#define WRITE_MEM8(addr, v) cpu->m_writeMem((addr), (v))
#define READ_MEM8(addr)     cpu->m_readMem((addr))

#define PUSH_8(v) \
  WRITE_MEM8(cpu->m_regs.m_SP--, (v));

#define PUSH_16(v) \
  PUSH_8(LOBYTE(cpu->m_regs.m_PC)); \
  PUSH_8(HIBYTE(cpu->m_regs.m_PC)); \

#define READ_IMMED16() \
      uint16 = (cpu->m_ins[1] << 8) + cpu->m_ins[2];

#define READ_INDEX() \
      uint16 = cpu->m_ins[1];

#define READ_MEM16(addr)\
      (READ_MEM8(addr+0) << 8) + READ_MEM8(addr+1)

#define WRITE_MEM16(addr, val)\
      WRITE_MEM8(addr+0, HIBYTE(val)); \
      WRITE_MEM8(addr+1, LOBYTE(val));

///////////////////////////////////////////////////////

static int Exec(M6800 * cpu);
static int ExecReset(M6800 * cpu);

void M6800_Init(M6800 * cpu)
{
  cpu->m_regs.m_PC = 0;
  cpu->m_regs.m_status = M6800_STATUS_ONES;
  cpu->m_readMem  = 0;
  cpu->m_writeMem = 0;
  cpu->m_trap     = 0;
  cpu->m_exec     = &ExecReset;
}

static int ExecReset(M6800 * cpu)
{
  uint16_t addr = (cpu->m_readMem(RESET_VECTOR) << 8) +
                  (cpu->m_readMem(RESET_VECTOR+1));
  printf("Reset addr 0x%04x\n", addr);                
  cpu->m_regs.m_PC   = addr;
  cpu->m_exec = &Exec;
  return Exec(cpu);
}

uint8_t * M6800_GetInstruction(M6800 * cpu, int * len)
{
  *len = cpu->m_insLen;
  return cpu->m_ins;
}

void M6800_Disassemble(uint8_t * code, int len, char * text, int textLen)
{
  text[0] = '\0';
  strcpy(text, g_mnemonic[code[0]]);
}

int M6800_Run_Instruction(M6800 * cpu)
{
  return cpu->m_exec(cpu);
}

int M6800_Run(M6800 * cpu, int instructionCount)
{
  int cycles = 0;
  while (instructionCount-- > 0)
    cycles += cpu->m_exec(cpu);
  return cycles;  
}

static int Exec(M6800 * cpu)
{
  // get opcode
  cpu->m_insAddr = cpu->m_regs.m_PC;
  cpu->m_ins[0]  = (*cpu->m_readMem)(cpu->m_regs.m_PC++);
  cpu->m_insLen  = g_lenAndCycles[cpu->m_ins[0]][0];
  cpu->m_cycles  = g_lenAndCycles[cpu->m_ins[0]][1];
  if (cpu->m_insLen > 1) {
    cpu->m_ins[1] = (*cpu->m_readMem)(cpu->m_regs.m_PC++);
    if (cpu->m_insLen > 2)
      cpu->m_ins[2] = (*cpu->m_readMem)(cpu->m_regs.m_PC++);
  }

  int8_t relOffs;
  uint16_t uint16;

  switch (cpu->m_ins[0]) {
    case SWI:
      PUSH_16(cpu->m_regs.m_PC);
      PUSH_16(cpu->m_regs.m_X);
      PUSH_8(cpu->m_regs.m_A);
      PUSH_8(cpu->m_regs.m_B);
      PUSH_8(cpu->m_regs.m_status);
      cpu->m_regs.m_PC = (cpu->m_readMem(SWI_VECTOR) << 8) +
                         (cpu->m_readMem(SWI_VECTOR+1));
      break;

    case BSR:
      PUSH_16(cpu->m_regs.m_PC);
      cpu->m_regs.m_PC += (int8_t)(cpu->m_ins[1]);
      break;

    case CLRA:
      cpu->m_regs.m_A = 0;
      cpu->m_regs.m_status &= ~M6800_STATUS_MASK;
      cpu->m_regs.m_status |= M6800_STATUS_ZERO;
      break;

    case CLRB:
      cpu->m_regs.m_B = 0;
      cpu->m_regs.m_status &= ~M6800_STATUS_MASK;
      cpu->m_regs.m_status |= M6800_STATUS_ZERO;
      break;

    case LDAAi:
      cpu->m_regs.m_A = cpu->m_ins[1];
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    case LDABi:
      cpu->m_regs.m_B = cpu->m_ins[1];
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    case LDSi:
      READ_IMMED16();
      cpu->m_regs.m_SP = uint16;
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    case LDXi:
      READ_IMMED16();
      cpu->m_regs.m_X = uint16;
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    case STXd:
      READ_INDEX();
      WRITE_MEM16(uint16, cpu->m_regs.m_X)
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    case LDXe:
      READ_IMMED16();
      cpu->m_regs.m_X = READ_MEM16(uint16);
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;
      break;

    default:
      if (cpu->m_trap)
        (*cpu->m_trap)(cpu->m_regs.m_PC, cpu->m_ins[0]);
  }

  // return cycles
  return cpu->m_cycles;
}
