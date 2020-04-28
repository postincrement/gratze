
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
  SUBAd, CMPAd, SBCAd, x93,   ANDAd, BITAd, LDAAd, STAAd,  
  EORAd, ADCAd, ORAAd, ADDAd, CPXAd, x9d,   LDSd,  STSd,

  // 0xa0
  SUBAx, CMPAx, SBCAx, xa3,   ANDAx, BITAx, LDAAx, STAAx,  
  EORAx, ADCAx, ORAAx, ADDAx, CPXAx, JSRx,  LDSx,  STSx,

  // 0xb0
  SUBAe, CMPAe, SBCAe, xb3,   ANDAe, BITAe, LDAAe, STAAe,  
  EORAe, ADCAe, ORAAe, ADDAe, CPXAe, JSRe,  LDSe,  STSe,

  // 0xc0
  SUBBi, CMPBi, SBBAi, xc3,   ANDBi, BITBi, LDABi, xc7,  
  EORBi, ADCBi, ORABi, ADDBi, xcc,   xcd,   LDXi,  xcf,

  // 0xd0
  SUBBd, CMPBd, SBCBd, xd3,   ANDBd, BITBd, LDABd, STABd,  
  EORBd, ADCBd, ORABd, ADDBd, xdc,   xdd,   LDXd,  STXd,

  // 0xe0
  SUBBx, CMPBx, SBCBx, xe3,   ANDBx, BITBx, LDABx, STABx,  
  EORBx, ADCBx, ORABx, ADDBx, xec,   xed,   LDXx,  STXx,

  // 0xf0
  SUBBe, CMPBe, SBCBe, xf3,   ANDBe, BITBe, LDABe, STABe,  
  EORBe, ADCBe, ORABe, ADDBe, xfc,   xfd,   LDXe,  STXe
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

#define REG_A      cpu->m_regs.m_A
#define REG_B      cpu->m_regs.m_B
#define REG_X      cpu->m_regs.m_X
#define REG_PC     cpu->m_regs.m_PC
#define REG_SP     cpu->m_regs.m_SP
#define REG_STATUS cpu->m_regs.m_status

///////////////////////////////////////////

#define LOBYTE(v)   ((v) & 0xff)
#define HIBYTE(v)   ((v) >> 8)

#define READ_MEM8(addr) \
      cpu->m_readMem((addr))

#define READ_MEM16(addr)\
      (READ_MEM8(addr+0) << 8) + READ_MEM8(addr+1)

#define WRITE_MEM8(addr, v)\
      cpu->m_writeMem((addr), (v))

#define WRITE_MEM16(addr, val)\
      WRITE_MEM8(addr+0, HIBYTE(val)); \
      WRITE_MEM8(addr+1, LOBYTE(val));

#define READ_IMMED8() \
      (cpu->m_ins[1])

#define READ_IMMED16() \
      ((cpu->m_ins[1] << 8) + cpu->m_ins[2])

#define READ_DIR8()  READ_MEM8((uint16_t)READ_IMMED8())

inline uint8_t ReadExt8(M6800 * cpu)
{
  uint16_t addr = READ_IMMED16();
  return READ_MEM8(addr);
}

#define READ_EXT8() ReadExt8(cpu)

inline uint16_t ReadExt16(M6800 * cpu)
{
  uint16_t addr = READ_IMMED16();
  return READ_MEM16(addr);
}

#define READ_EXT16() ReadExt16(cpu)

#define PUSH_8(v) \
  WRITE_MEM8(REG_SP--, (v));

#define PUSH_16(v) \
  PUSH_8(LOBYTE(REG_PC)); \
  PUSH_8(HIBYTE(REG_PC)); \


///////////////////////////////////////////

#define SET_Z(v)  cpu->m_regs.m_status = (v == 0) ? (cpu->m_regs.m_status & ~M6800_STATUS_ZERO) \
                                                  : (cpu->m_regs.m_status | M6800_STATUS_ZERO) \
                                                  ;

#define SET_S8(v)  cpu->m_regs.m_status = (v & 0x80) ? (cpu->m_regs.m_status | M6800_STATUS_SIGN) \
                                                    : (cpu->m_regs.m_status & ~M6800_STATUS_SIGN) \
                                                    ;
#define SET_S16(v)  cpu->m_regs.m_status = (v & 0x8000) ? (cpu->m_regs.m_status | M6800_STATUS_SIGN) \
                                                        : (cpu->m_regs.m_status & ~M6800_STATUS_SIGN) \
                                                        ;
#define ZERO_OVF() \
      cpu->m_regs.m_status &= ~M6800_STATUS_OVERFLOW;

#define SET_SZ8(v) SET_S8(v); SET_Z(v)

#define SET_SZ16(v) SET_S16(v); SET_Z(v)

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
    //////////////////////////////////////////////////////////////////////
    //
    //  load
    //  
    case LDAAx: 
    case LDABx:
      break;

    case LDAAi: REG_A = READ_IMMED8(); SET_SZ8(REG_A); ZERO_OVF(); break;
    case LDABi: REG_B = READ_IMMED8(); SET_SZ8(REG_B); ZERO_OVF(); break;

    case LDAAe: REG_A = READ_EXT8();   SET_SZ8(REG_A); ZERO_OVF(); break;
    case LDABe: REG_B = READ_EXT8();   SET_SZ8(REG_B); ZERO_OVF(); break;

    case LDAAd: REG_A = READ_DIR8();   SET_SZ8(REG_A); ZERO_OVF(); break;
    case LDABd: REG_B = READ_DIR8();   SET_SZ8(REG_B); ZERO_OVF(); break; 
      break;

    case LDSd : 
    case LDXd : 

    case LDSx : 
    case LDXx : 
      break;
      
    case LDSi : REG_SP = READ_IMMED16(); SET_SZ16(REG_SP); ZERO_OVF(); break;
    case LDXi : REG_X  = READ_IMMED16(); SET_SZ16(REG_X);  ZERO_OVF(); break;

    case LDSe : REG_SP = READ_EXT16(); SET_SZ16(REG_SP); ZERO_OVF(); break; 
    case LDXe : REG_X  = READ_EXT16(); SET_SZ16(REG_X);  ZERO_OVF(); break;

    case STXd :
      READ_IMMED8();
      WRITE_MEM16(uint16, cpu->m_regs.m_X)
      ZERO_OVF();
      break;

    case STXx :
    case STXe :
      break;

    case STSd :
    case STSx :
    case STSe :
      break;

    case STAAd :  
    case STAAx :  
    case STAAe :  
      break;

    case STABd :  
    case STABx :  
    case STABe :  
      break;

    //////////////////////////////////////////////////////////////////////
    //
    //  register transfer
    //
    case TAB  : REG_B = REG_A;      SET_SZ8(REG_B); ZERO_OVF(); break;
    case TBA  : REG_A = REG_B;      SET_SZ8(REG_A); ZERO_OVF(); break;
    case TSX  : REG_X = REG_SP + 1; break;
    case TXS  : REG_SP = REG_X - 1; break; 

    case TAP  : REG_STATUS = REG_A | M6800_STATUS_ONES; break; 
    case TPA  : REG_A      = REG_STATUS;                break;

    //////////////////////////////////////////////////////////////////////
    //
    //   misc
    //
    case NOP  : break;

    //////////////////////////////////////////////////////////////////////
    //
    //  interrupts
    //  

    case SWI:
      PUSH_16(cpu->m_regs.m_PC);
      PUSH_16(cpu->m_regs.m_X);
      PUSH_8(cpu->m_regs.m_A);
      PUSH_8(cpu->m_regs.m_B);
      PUSH_8(cpu->m_regs.m_status);
      cpu->m_regs.m_PC = (cpu->m_readMem(SWI_VECTOR) << 8) +
                         (cpu->m_readMem(SWI_VECTOR+1));
      break;

    //////////////////////////////////////////////////////////////////////
    //
    // branches
    //
    case BSR:
      PUSH_16(cpu->m_regs.m_PC);
      cpu->m_regs.m_PC += (int8_t)(cpu->m_ins[1]);
      break;
    case BRA  : 
    case BHI  : 
    case BLS  : 
    case BCC  : 
    case BCS  : 
    case BNE  : 
    case BEQ  :  
    case BVC  : 
    case BVS  : 
    case BPL  : 
    case BMI  : 
    case BGE  : 
    case BLT  : 
    case BGT  : 
    case BLE  :
      break;

    //////////////////////////////////////////////////////////////////////
    //
    //  jump 
    //
    case JSRe : 
    case JSRx : 
    case JMPx : 
    case JMPe : 
      break;

    //////////////////////////////////////////////////////////////////////
    // 
    //  stack operations
    //    
    case PULA : 
    case PULB : 
    case PSHA : 
    case PSHB : 
      break;

    case INX  : 
    case DEX  : 
    case CLV  : 
    case SEV  : 
    case CLC  : 
    case SEC  : 
    case CLI  : 
    case SEI  :
    case SBA  : 
    case CBA  : 

    case DAA  : 
    case ABA  : 
      break;

    case INS  : 
    case DES  : 
    case RTS  : 
    case RTI  : 
    case WAI  : 
      break;

    //////////////////////////////////////////////////////////////////////
    //
    //
    //
    case NEGA : case NEGB : case NEGi : case NEGe : 
    case COMA : case COMB : case COMi : case COMe : 
    case LSRA : case LSRB : case LSRi : case LSRe : 
    case RORA : case RORB : case RORx : case RORe : 
    case ASRA : case ASRB : case ASRx : case ASRe : 
    case ASLA : case ASLB : case ASLx : case ASLe : 
    case ROLA : case ROLB : case ROLx : case ROLe : 
    case DECA : case DECB : case DECx : case DECe : 
    case INCA : case INCB : case INCx : case INCe : 
    case TSTA : case TSTB : case TSTx : case TSTe :
      break; 

    //////////////////////////////////////////////////////////////////////
    //
    //  clear
    //
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

    case CLRx : case CLRe :
      break;

    //////////////////////////////////////////////////////////////////////
    //
    //  
    //
    case SUBAi: case SUBAd: case SUBAx: case SUBAe: 
    case CMPAi: case CMPAd: case CMPAx: case CMPAe: 
    case SBCAi: case SBCAd: case SBCAx: case SBCAe: 
    case ANDAi: case ANDAd: case ANDAx: case ANDAe: 
    case BITAi: case BITAd: case BITAx: case BITAe: 
    case EORAi: case EORAd: case EORAx: case EORAe: 
    case ADCAi: case ADCAd: case ADCAx: case ADCAe: 
    case ORAAi: case ORAAd: case ORAAx: case ORAAe: 
    case ADDAi: case ADDAd: case ADDAx: case ADDAe:  
    case CPXAi: case CPXAd: case CPXAx: case CPXAe:             
      break;

    case SUBBi: case SUBBd: case SUBBx: case SUBBe: 
    case CMPBi: case CMPBd: case CMPBx: case CMPBe: 
    case SBBAi: case SBCBd: case SBCBx: case SBCBe: 
    case ANDBi: case ANDBd: case ANDBx: case ANDBe: 
    case BITBi: case BITBd: case BITBx: case BITBe: 
    case EORBi: case EORBd: case EORBx: case EORBe: 
    case ADCBi: case ADCBd: case ADCBx: case ADCBe: 
    case ORABi: case ORABd: case ORABx: case ORABe:
    case ADDBi: case ADDBd: case ADDBx: case ADDBe:
      break;

    case x00 : case x02 : case x03 : case x04 : 
    case x05 : case x12 : case x13 : case x14 : 
    case x15 : case x18 : case x1a : case x1c : 
    case x1d : case x1e : case x1f : case x21 : 
    case x38 : case x3a : case x3c : case x3d : 
    case x41 : case x42 : case x45 : case x4b : 
    case x4e : case x51 : case x52 : case x55 : 
    case x5b : case x5e : case x83 : case x87 : 
    case x61 : case x62 : case x65 : case x6b : 
    case x71 : case x72 : case x75 : case x7b : 
    case x8f : case x93 : case x9d : case xa3 : 
    case xb3 : case xc3 : case xc7 : case xcc : 
    case xcd : case xcf : case xd3 : case xdc : 
    case xdd : case xe3 : case xec : case xed : 
    case xf3 : case xfc : case xfd : 
    default:
      if (cpu->m_trap)
        (*cpu->m_trap)(cpu->m_regs.m_PC, cpu->m_ins[0]);
  }

  // return cycles
  return cpu->m_cycles;
}
