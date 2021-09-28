//
// Modified from the Phunsy emulator by Frank Pocnet
// http://www.tubedata.org/phunsy/index.html
//
// By Craig Southeren Jan 2020
//
//  Original change comments below
//
/*
** 100902 PHUNSY 2650 micro computer emulator
** 100925 for the phunsy 1 cpu cycle time is 3�s
** 101026
** 101113 added mdcr read data clock activity while cassette in reverse
** 101114 changed exit by ~ key to DEL key
** 101114 added extended i/o
** 101114 added print to logfile
** 101120 SDL added
** 101121 bug in Q bank memory movement corrected
** 101121 memory filled with HALT instructions
** 101123 indirect addressing bug fixed
** 101124 added exit on monitor or mdcr rom write
** 101127 phunsy roms included in exe
** 101127 arrow keys added
** 101127 added 4 cassette feature
** 101201 small error in phgraphpix[128] corrected
** 140104 texts simulator changed to emulator
*/

#include <stdio.h>
#include "cpu_2650.h"

#define noMDCRDEBUG

/***************************************
** 2650 cpu definitions & variables
***************************************/

// addressing format
#define F_N  0  // no addressing
#define F_Z  1  // register addressing
#define F_I  2  // immediate
#define F_R  3  // relative
#define F_A  4  // absolute non branch
#define F_B  5  // absolute branch
#define F_C  6  // absolute program status
#define F_E  7  // miscellaneaus one byte instructions
#define F_EI 8  // miscellaneaus immediate
#define F_ER 9  // zero branch relative
#define F_EB 10 // absolute branch index R3

// psl bits
#define CY  (1<<0)
#define COM (1<<1)
#define OVF (1<<2)
#define WC  (1<<3)
#define RS  (1<<4)
#define IDC (1<<5)
#define CC0 (1<<6)
#define CC1 (1<<7)

// psu bits
#define SPMASK 0x07
// the rest is not interresting as there is no hardware. All 8 bits can be set or cleared by software (lpsu instruction) even the sense bit. The II bit is cleared by RETE.
#define UF1   (1<<3) // only in 2650B
#define UF2   (1<<4) // only in 2650B
#define II    (1<<5)
#define FLAG  (1<<6) // Phunsy used this as audio casste data output
#define SENSE (1<<7) // Phunsy used this as audio casste data input

struct instructions
{
  char mnem[8];
  int bytes;
  int format;
  int cycles;
} instrdata[256] =
{
//0x0x
  "INVALID", 1, 0   , 0,
  "LODZ R1", 1, F_Z , 2,
  "LODZ R2", 1, F_Z , 2,
  "LODZ R3", 1, F_Z , 2,
  "LODI R0", 2, F_I , 2,
  "LODI R1", 2, F_I , 2,
  "LODI R2", 2, F_I , 2,
  "LODI R3", 2, F_I , 2,
  "LODR R0", 2, F_R , 3,
  "LODR R1", 2, F_R , 3,
  "LODR R2", 2, F_R , 3,
  "LODR R3", 2, F_R , 3,
  "LODA R0", 3, F_A , 4,
  "LODA R1", 3, F_A , 4,
  "LODA R2", 3, F_A , 4,
  "LODA R3", 3, F_A , 4,
// 0x1x
  "LDPL   ", 3, F_C , 4, // 2650B only
  "STPL   ", 3, F_C , 4, // 2650B only
  "SPSU   ", 1, F_E , 2,
  "SPSL   ", 1, F_E , 2,
  "RETC  Z", 1, F_Z , 3,
  "RETC  P", 1, F_Z , 3,
  "RETC  N", 1, F_Z , 3,
  "RETC UN", 1, F_Z , 3,
  "BCTR  Z", 2, F_R , 3,
  "BCTR  P", 2, F_R , 3,
  "BCTR  N", 2, F_R , 3,
  "BCTR UN", 2, F_R , 3,
  "BCTA  Z", 3, F_B , 3,
  "BCTA  P", 3, F_B , 3,
  "BCTA  N", 3, F_B , 3,
  "BCTA UN", 3, F_B , 3,
// 0x2x
  "EORZ R0", 1, F_Z , 2,
  "EORZ R1", 1, F_Z , 2,
  "EORZ R2", 1, F_Z , 2,
  "EORZ R3", 1, F_Z , 2,
  "EORI R0", 2, F_I , 2,
  "EORI R1", 2, F_I , 2,
  "EORI R2", 2, F_I , 2,
  "EORI R3", 2, F_I , 2,
  "EORR R0", 2, F_R , 3,
  "EORR R1", 2, F_R , 3,
  "EORR R2", 2, F_R , 3,
  "EORR R3", 2, F_R , 3,
  "EORA R0", 3, F_A , 4,
  "EORA R1", 3, F_A , 4,
  "EORA R2", 3, F_A , 4,
  "EORA R3", 3, F_A , 4,
// 0x3x
  "REDC R0", 1, F_Z , 2,
  "REDC R1", 1, F_Z , 2,
  "REDC R2", 1, F_Z , 2,
  "REDC R3", 1, F_Z , 2,
  "RETE  Z", 1, F_Z , 3,
  "RETE  P", 1, F_Z , 3,
  "RETE  N", 1, F_Z , 3,
  "RETE UN", 1, F_Z , 3,
  "BSTR  Z", 2, F_R , 3,
  "BSTR  P", 2, F_R , 3,
  "BSTR  N", 2, F_R , 3,
  "BSTR UN", 2, F_R , 3,
  "BSTA  Z", 3, F_B , 3,
  "BSTA  P", 3, F_B , 3,
  "BSTA  N", 3, F_B , 3,
  "BSTA UN", 3, F_B , 3,
// 0x4x
  "HALT   ", 1, F_E , 1,
  "ANDZ R1", 1, F_Z , 2,
  "ANDZ R2", 1, F_Z , 2,
  "ANDZ R3", 1, F_Z , 2,
  "ANDI R0", 2, F_I , 2,
  "ANDI R1", 2, F_I , 2,
  "ANDI R2", 2, F_I , 2,
  "ANDI R3", 2, F_I , 2,
  "ANDR R0", 2, F_R , 3,
  "ANDR R1", 2, F_R , 3,
  "ANDR R2", 2, F_R , 3,
  "ANDR R3", 2, F_R , 3,
  "ANDA R0", 3, F_A , 4,
  "ANDA R1", 3, F_A , 4,
  "ANDA R2", 3, F_A , 4,
  "ANDA R3", 3, F_A , 4,
// 0x5x
  "RRR  R0", 1, F_Z , 2,
  "RRR  R1", 1, F_Z , 2,
  "RRR  R2", 1, F_Z , 2,
  "RRR  R3", 1, F_Z , 2,
  "REDE R0", 2, F_I , 3,
  "REDE R1", 2, F_I , 3,
  "REDE R2", 2, F_I , 3,
  "REDE R3", 2, F_I , 3,
  "BRNR R0", 2, F_R , 3,
  "BRNR R1", 2, F_R , 3,
  "BRNR R2", 2, F_R , 3,
  "BRNR R3", 2, F_R , 3,
  "BRNA R0", 3, F_B , 3,
  "BRNA R1", 3, F_B , 3,
  "BRNA R2", 3, F_B , 3,
  "BRNA R3", 3, F_B , 3,
// 0x6
  "IORZ R0", 1, F_Z , 2,
  "IORZ R1", 1, F_Z , 2,
  "IORZ R2", 1, F_Z , 2,
  "IORZ R3", 1, F_Z , 2,
  "IORI R0", 2, F_I , 2,
  "IORI R1", 2, F_I , 2,
  "IORI R2", 2, F_I , 2,
  "IORI R3", 2, F_I , 2,
  "IORR R0", 2, F_R , 3,
  "IORR R1", 2, F_R , 3,
  "IORR R2", 2, F_R , 3,
  "IORR R3", 2, F_R , 3,
  "IORA R0", 3, F_A , 4,
  "IORA R1", 3, F_A , 4,
  "IORA R2", 3, F_A , 4,
  "IORA R3", 3, F_A , 4,
// 0x7x
  "REDD R0", 1, F_Z , 2,
  "REDD R1", 1, F_Z , 2,
  "REDD R2", 1, F_Z , 2,
  "REDD R3", 1, F_Z , 2,
  "CPSU   ", 2, F_EI, 3,
  "CPSL   ", 2, F_EI, 3,
  "PPSU   ", 2, F_EI, 3,
  "PPSL   ", 2, F_EI, 3,
  "BSNR  Z", 2, F_R , 3,
  "BSNR  P", 2, F_R , 3,
  "BSNR  N", 2, F_R , 3,
  "BSNR UN", 2, F_R , 3,
  "BSNA  Z", 3, F_B , 3,
  "BSNA  P", 3, F_B , 3,
  "BSNA  N", 3, F_B , 3,
  "BSNA UN", 3, F_B , 3,
// 0x8x
  "ADDZ R0", 1, F_Z , 2,
  "ADDZ R1", 1, F_Z , 2,
  "ADDZ R2", 1, F_Z , 2,
  "ADDZ R3", 1, F_Z , 2,
  "ADDI R0", 2, F_I , 2,
  "ADDI R1", 2, F_I , 2,
  "ADDI R2", 2, F_I , 2,
  "ADDI R3", 2, F_I , 2,
  "ADDR R0", 2, F_R , 3,
  "ADDR R1", 2, F_R , 3,
  "ADDR R2", 2, F_R , 3,
  "ADDR R3", 2, F_R , 3,
  "ADDA R0", 3, F_A , 4,
  "ADDA R1", 3, F_A , 4,
  "ADDA R2", 3, F_A , 4,
  "ADDA R3", 3, F_A , 4,
// 0x9x
  "INVALID", 1, 0   , 0,
  "INVALID", 1, 0   , 0,
  "LPSU   ", 1, F_E , 2,
  "LPSL   ", 1, F_E , 2,
  "DAR  R0", 1, F_Z , 3, // cc is set to a meaningless value
  "DAR  R1", 1, F_Z , 3,
  "DAR  R2", 1, F_Z , 3,
  "DAR  R3", 1, F_Z , 3,
  "BCFR  Z", 2, F_R , 3,
  "BCFR  P", 2, F_R , 3,
  "BCFR  N", 2, F_R , 3,
  "ZBRR   ", 2, F_ER, 3,
  "BCFA  Z", 3, F_B , 3,
  "BCFA  P", 3, F_B , 3,
  "BCFA  N", 3, F_B , 3,
  "BXA  R3", 3, F_EB, 3, // index register is R3 only for BXA
// 0xax
  "SUBZ R0", 1, F_Z , 2,
  "SUBZ R1", 1, F_Z , 2,
  "SUBZ R2", 1, F_Z , 2,
  "SUBZ R3", 1, F_Z , 2,
  "SUBI R0", 2, F_I , 2,
  "SUBI R1", 2, F_I , 2,
  "SUBI R2", 2, F_I , 2,
  "SUBI R3", 2, F_I , 2,
  "SUBR R0", 2, F_R , 3,
  "SUBR R1", 2, F_R , 3,
  "SUBR R2", 2, F_R , 3,
  "SUBR R3", 2, F_R , 3,
  "SUBA R0", 3, F_A , 4,
  "SUBA R1", 3, F_A , 4,
  "SUBA R2", 3, F_A , 4,
  "SUBA R3", 3, F_A , 4,
// 0xbx
  "WRTC R0", 1, F_Z , 2,
  "WRTC R1", 1, F_Z , 2,
  "WRTC R2", 1, F_Z , 2,
  "WRTC R3", 1, F_Z , 2,
  "TPSU   ", 2, F_EI, 2,
  "TPSL   ", 2, F_EI, 2,
  "INVALID", 1, 0   , 0,
  "INVALID", 1, 0   , 0,
  "BSFR  Z", 2, F_R , 3,
  "BSFR  P", 2, F_R , 3,
  "BSFR  N", 2, F_R , 3,
  "ZBSR   ", 2, F_ER, 3,
  "BSFA  Z", 3, F_B , 3,
  "BSFA  P", 3, F_B , 3,
  "BSFA  N", 3, F_B , 3,
  "BSXA R3", 3, F_EB, 3, // index register is R3 only for BXA
// 0xcx
  "NOP    ", 1, F_Z , 1,
  "STRZ R1", 1, F_Z , 2,
  "STRZ R2", 1, F_Z , 2,
  "STRZ R3", 1, F_Z , 2,
  "INVALID", 1, 0   , 0,
  "INVALID", 1, 0   , 0,
  "INVALID", 1, 0   , 0,
  "INVALID", 1, 0   , 0,
  "STRR R0", 2, F_R , 3,
  "STRR R1", 2, F_R , 3,
  "STRR R2", 2, F_R , 3,
  "STRR R3", 2, F_R , 3,
  "STRA R0", 3, F_A , 4,
  "STRA R1", 3, F_A , 4,
  "STRA R2", 3, F_A , 4,
  "STRA R3", 3, F_A , 4,
// 0xdx
  "RRL  R0", 1, F_Z , 2,
  "RRL  R1", 1, F_Z , 2,
  "RRL  R2", 1, F_Z , 2,
  "RRL  R3", 1, F_Z , 2,
  "WRTE R0", 2, F_I , 3,
  "WRTE R1", 2, F_I , 3,
  "WRTE R2", 2, F_I , 3,
  "WRTE R3", 2, F_I , 3,
  "BIRR R0", 2, F_R , 3,
  "BIRR R1", 2, F_R , 3,
  "BIRR R2", 2, F_R , 3,
  "BIRR R3", 2, F_R , 3,
  "BIRA R0", 3, F_B , 3,
  "BIRA R1", 3, F_B , 3,
  "BIRA R2", 3, F_B , 3,
  "BIRA R3", 3, F_B , 3,
// 0xex
  "COMZ R0", 1, F_Z , 2,
  "COMZ R1", 1, F_Z , 2,
  "COMZ R2", 1, F_Z , 2,
  "COMZ R3", 1, F_Z , 2,
  "COMI R0", 2, F_I , 2,
  "COMI R1", 2, F_I , 2,
  "COMI R2", 2, F_I , 2,
  "COMI R3", 2, F_I , 2,
  "COMR R0", 2, F_R , 3,
  "COMR R1", 2, F_R , 3,
  "COMR R2", 2, F_R , 3,
  "COMR R3", 2, F_R , 3,
  "COMA R0", 3, F_A , 4,
  "COMA R1", 3, F_A , 4,
  "COMA R2", 3, F_A , 4,
  "COMA R3", 3, F_A , 4,
// oxfx
  "WRTD R0", 1, F_Z , 2,
  "WRTD R1", 1, F_Z , 2,
  "WRTD R2", 1, F_Z , 2,
  "WRTD R3", 1, F_Z , 2,
  "TMI  R0", 2, F_I , 3,
  "TMI  R1", 2, F_I , 3,
  "TMI  R2", 2, F_I , 3,
  "TMI  R3", 2, F_I , 3,
  "BDRR R0", 2, F_R , 3,
  "BDRR R1", 2, F_R , 3,
  "BDRR R2", 2, F_R , 3,
  "BDRR R3", 2, F_R , 3,
  "BDRA R0", 3, F_B , 3,
  "BDRA R1", 3, F_B , 3,
  "BDRA R2", 3, F_B , 3,
  "BDRA R3", 3, F_B , 3
};


/***************************************
** 2650 Emulator
***************************************/

CPU2650::~CPU2650()
{}

// perform add and set ICD, CY and OVF accordingly
// this seems to work correctly
unsigned char CPU2650::add(unsigned char byte1, unsigned char byte2)
{
  unsigned short result, result2;

  result=0;
  if(((registers.psl)&(WC|CY))==(WC|CY))
    result=1;
  result2=result;
  result+=byte1+byte2;
  result2+=(byte1&15)+(byte2&15);

  registers.psl&=~IDC;
  if((result2&0x0010)!=0)
    registers.psl|=IDC;

  registers.psl&=~CY;
    if((result&0x0100)!=0)
  registers.psl|=CY;

  registers.psl&=~OVF;
  if((byte1&0x80)==(byte2&0x80))
    if((byte1&0x80)!=(result&0x0080))
      registers.psl|=OVF;

  return((unsigned char)result);
}

// perform subtract and set ICD, CY and OVF accordingly
// note: borrow is stored inverted in program status. IDC too?
unsigned char CPU2650::sub(unsigned char byte1, unsigned char byte2)
{
  unsigned short result, result2;

  result=0;
  if(((registers.psl)&(WC|CY))==WC)
    result=1;
  result2=result;
  result=byte1-byte2-result;
  result2=(byte1&15)-(byte2&15)-result2;

  registers.psl&=~IDC;
  if((result2&0x0010)==0)
    registers.psl|=IDC;

  registers.psl&=~CY;
    if((result&0x0100)==0)
  registers.psl|=CY;

  registers.psl&=~OVF;
  if((byte1&0x80)==(byte2&0x80))
    if((byte1&0x80)!=(result&0x0080))
      registers.psl|=OVF;

  return((unsigned char)result);
}

// perform compare
// returns positive byte if byte1 > byte2
// returns negative byte if byte1 < byte2
// returns zero byte if byte1 = byte2
// COM in psl determines compare mode:
// 0 means signed compare, 1 means unsigned compare
unsigned char CPU2650::com(unsigned char byte1, unsigned char byte2)
{
  unsigned short result;

  result=0;
  if((registers.psl&COM)!=0)                // unsigned compare
  {
    if(((byte1&0x80)!=0)&&((byte2&0x80)==0))
      result=0x0001;
    if(((byte1&0x80)==0)&&((byte2&0x80)!=0))
      result=0xffff;
  }
  else                                      // signed compare
  {
    if(((byte1&0x80)!=0)&&((byte2&0x80)==0))
      result=0xffff;
    if(((byte1&0x80)==0)&&((byte2&0x80)!=0))
      result=0x0001;
  }
  if(result==0)
    result=byte1-byte2;

  return((unsigned char)result);
}

// perform right rotate and set ICD, CY and OVF accordingly
unsigned char CPU2650::rrr(unsigned char byte)
{
  unsigned short result;

  result=byte;
  if((registers.psl&WC)!=0)
  {
    if((registers.psl&CY)!=0)
      result|=0x0100;
    registers.psl&=~CY;
    if((result&0x0001)!=0)
      registers.psl|=CY;
    registers.psl&=~IDC;
    if((result&0x0040)!=0)
      registers.psl|=IDC;
  }
  else
  {
    if((result&0x0001)!=0)
      result|=0x0100;
  }
  result=result>>1;
  return((unsigned char)result);
}

// perform left rotate and set ICD, CY and OVF accordingly
unsigned char CPU2650::rrl(unsigned char byte)
{
  unsigned short result;

  result=byte<<1;
  if((registers.psl&WC)!=0)
  {
    if((registers.psl&CY)!=0)
      result|=0x0001;
    registers.psl&=~CY;
    if((result&0x0100)!=0)
      registers.psl|=CY;
    registers.psl&=~IDC;
    if((result&0x0020)!=0)
      registers.psl|=IDC;
  }
  else
  {
    if((result&0x0100)!=0)
      result|=0x0001;
  }
  return((unsigned char)result);
}

// set cc according to byte
void CPU2650::setcc(unsigned char byte)
{
    registers.psl&=~(CC1|CC0);
    if((byte&0x80)!=0)
      registers.psl|=CC1;
    else
      if((byte&0x7f)!=0)
        registers.psl|=CC0;
}

// push address on stack
void CPU2650::push(unsigned short address)
{
  registers.as[registers.psu&0x07]=address; // store address
  registers.psu=((registers.psu+1)&0x07)|(registers.psu&0xf8); // increment stack pointer and leave other bits
}

// pull address from stack
unsigned short CPU2650::pull(void)
{
  registers.psu=((registers.psu-1)&0x07)|(registers.psu&0xf8); // increment stack pointer and leave other bits
  return(registers.as[registers.psu&0x07]);
}

void CPU2650::interrupt(void)
{
  push(registers.na);
  registers.na=0x001D; // this is the same as 'ZBSR 001d'
}

// cpu function return codes
#define NORMAL      (0<<16)  // just registers have changed
#define INV_OPCODE  (1<<16)  // invalid opcode
#define HALT        (2<<16)  // HALT instruction encounted
#define MEM_WRITE   (3<<16)  // a location in memory has been altered
#define MEM_READ    (4<<16)  // a location in memory has been read
#define PC_WRITE    (5<<16)  // A write to port-c
#define PC_READ     (6<<16)  // A read from port-c
#define PD_WRITE    (7<<16)  // A write to port-d
#define PD_READ     (8<<16)  // A read from port-d
#define EXT_WRITE   (9<<16)  // A write to port-d
#define EXT_READ   (10<<16)  // A read from port-d

// this executes one instruction
int CPU2650::cpu(void)
{
  int ret; // return code
  unsigned char instrbyte1;
  unsigned char instrbyte2;
  unsigned char instrbyte3;
  unsigned short address;
  unsigned short effaddr;  // effective address
  unsigned short tmpaddr;
  int numbytes;
  int regnum; // 0..3 and 4..6
  int result;
  int ind;    // 2 extra cycles for indirect addressing

  registers.ap=registers.na;
  ind=0;
  ret=(NORMAL|registers.ap);
  address = registers.ap;
  instrbyte1 = ReadMemory(address);
  numbytes=instrdata[instrbyte1].bytes;
  cpucycles=instrdata[instrbyte1].cycles;
  address=((address+1)&0x1fff)|(address&0x6000);
  if(numbytes>1)
  {
    instrbyte2 = ReadMemory(address);
    address=((address+1)&0x1fff)|(address&0x6000);
    if(numbytes>2)
    {
      instrbyte3 = ReadMemory(address);
      address=((address+1)&0x1fff)|(address&0x6000);
    }
  }

  regnum=instrbyte1&0x03; // in many cases
  if(regnum!=0)
    if((registers.psl&RS)!=0)
      regnum+=3;

  switch(instrdata[instrbyte1].format)
  {
    case F_R  : // relative
      if((instrbyte2&0x40)==0)
        tmpaddr=((address+(instrbyte2&0x3f))&0x1fff)|(address&0x6000);
      else
        tmpaddr=(address-((~instrbyte2&0x3f)+1))|(address&0x6000);
      if((instrbyte2&0x80)==0)
        effaddr=tmpaddr;
      else // indirect
      {
        effaddr=((ReadMemory(tmpaddr)<<8) | ReadMemory(((tmpaddr+1)&0x1fff)|(tmpaddr&0x6000))) & 0x7fff;
        ind=2;
      }
      break;
    case F_A  : // absolute, non branch instructions
      tmpaddr= (((instrbyte2<<8) | instrbyte3) & 0x1fff) | (address&0x6000);
      if((instrbyte2&0x80)==0)
        effaddr=tmpaddr;
      else // indirect
      {
        effaddr=((ReadMemory(tmpaddr)<<8) | ReadMemory(((tmpaddr+1)&0x1fff)|(tmpaddr&0x6000))) & 0x7fff;
        ind=2;
      }
      if((instrbyte2&0x60)!=0)  // indexed
      {
        if((instrbyte2&0x60)==0x20)
          registers.r[regnum]++;
        if((instrbyte2&0x60)==0x40)
          registers.r[regnum]--;
        effaddr+=registers.r[regnum];
        effaddr&=0x7fff;
        regnum=0;
      }
      break;
    case F_B  : // absolute branch
    case F_C  : // absolute program status
    case F_EB : // absolute indexed R3
      tmpaddr= ((instrbyte2<<8) | instrbyte3) & 0x7fff;
      if((instrbyte2&0x80)==0)
        effaddr=tmpaddr;
      else // indirect
      {
        effaddr=((ReadMemory(tmpaddr)<<8) | ReadMemory(((tmpaddr+1)&0x1fff)|(tmpaddr&0x6000))) & 0x7fff;
        ind=2;
      }
      if(instrdata[instrbyte1].format==F_EB)
        effaddr=effaddr+registers.r[regnum]&0x7fff;
      break;
    case F_ER : // zero branch
      if((instrbyte2&0x40)==0)
        tmpaddr=instrbyte2&0x3f;
      else
        tmpaddr=(0x2000-((~instrbyte2&0x3f)+1));
      if((instrbyte2&0x80)==0)
        effaddr=tmpaddr;
      else // indirect
      {
        effaddr=((ReadMemory(tmpaddr)<<8) | ReadMemory(((tmpaddr+1)&0x1fff)|(tmpaddr&0x6000))) & 0x7fff;
        ind=2;
      }
      break;
  }


  switch(instrbyte1)
  {
// LOD instructuins
    case 0x01 : // lodz r1
    case 0x02 : // lodz r2
    case 0x03 : // lodz r3
      result=registers.r[regnum];
      registers.r[0]=result;
      setcc(result);
      break;
    case 0x04 : // lodi r0
    case 0x05 : // lodi r1
    case 0x06 : // lodi r2
    case 0x07 : // lodi r3
      result=instrbyte2;
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0x08 : // lodr r0
    case 0x09 : // lodr r1
    case 0x0a : // lodr r2
    case 0x0b : // lodr r3
    case 0x0c : // loda r0
    case 0x0d : // loda r1
    case 0x0e : // loda r2
    case 0x0f : // loda r3
      result=ReadMemory(effaddr);
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// EOR instructions
    case 0x20 : // eorz r0
    case 0x21 : // eorz r1
    case 0x22 : // eorz r2
    case 0x23 : // eorz r3
      result=registers.r[regnum]&~registers.r[0]|~registers.r[regnum]&registers.r[0];
      registers.r[0]=result;
      setcc(result);
      break;
    case 0x24 : // eori r0
    case 0x25 : // eori r1
    case 0x26 : // eori r2
    case 0x27 : // eori r3
      result=registers.r[regnum]&~instrbyte2|~registers.r[regnum]&instrbyte2;
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0x28 : // eorr r0
    case 0x29 : // eorr r1
    case 0x2a : // eorr r2
    case 0x2b : // eorr r3
    case 0x2c : // eora r0
    case 0x2d : // eora r1
    case 0x2e : // eora r2
    case 0x2f : // eora r3
      result=registers.r[regnum]&~ReadMemory(effaddr)|~registers.r[regnum]&ReadMemory(effaddr);
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// AND instructions
    case 0x41 : // andz r1
    case 0x42 : // andz r2
    case 0x43 : // andz r3
      result=registers.r[regnum]&registers.r[0];
      registers.r[0]=result;
      setcc(result);
      break;
    case 0x44 : // andi r0
    case 0x45 : // andi r1
    case 0x46 : // andi r2
    case 0x47 : // andi r3
      result=registers.r[regnum]&instrbyte2;
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0x48 : // andr r0
    case 0x49 : // andr r1
    case 0x4a : // andr r2
    case 0x4b : // andr r3
    case 0x4c : // anda r0
    case 0x4d : // anda r1
    case 0x4e : // anda r2
    case 0x4f : // anda r3
      result=registers.r[regnum]&ReadMemory(effaddr);
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// IOR instructions
    case 0x60 : // iorz ro
    case 0x61 : // iorz r1
    case 0x62 : // iorz r2
    case 0x63 : // iorz r3
      result=registers.r[regnum]|registers.r[0];
      registers.r[0]=result;
      setcc(result);
      break;
    case 0x64 : // iori r0
    case 0x65 : // iori r1
    case 0x66 : // iori r2
    case 0x67 : // iori r3
      result=registers.r[regnum]|instrbyte2;
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0x68 : // iorr r0
    case 0x69 : // iorr r1
    case 0x6a : // iorr r2
    case 0x6b : // iorr r3
    case 0x6c : // iora r0
    case 0x6d : // iora r1
    case 0x6e : // iora r2
    case 0x6f : // iora r3
      result=registers.r[regnum]|ReadMemory(effaddr);
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// ADD instructions
    case 0x80 : // addz r0
    case 0x81 : // addz r1
    case 0x82 : // addz r2
    case 0x83 : // addz r3
      result=add(registers.r[0],registers.r[regnum]);
      registers.r[0]=result;
      setcc(result);
      break;
    case 0x84 : // addi r0
    case 0x85 : // addi r1
    case 0x86 : // addi r2
    case 0x87 : // addi r3
      result=add(registers.r[regnum],instrbyte2);
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0x88 : // addr r0
    case 0x89 : // addr r1
    case 0x8a : // addr r2
    case 0x8b : // addr r3
    case 0x8c : // adda r0
    case 0x8d : // adda r1
    case 0x8e : // adda r2
    case 0x8f : // adda r3
      result=add(registers.r[regnum],ReadMemory(effaddr));
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// SUB instructions
    case 0xa0 : // subz r0
    case 0xa1 : // subz r1
    case 0xa2 : // subz r2
    case 0xa3 : // subz r3
      result=sub(registers.r[0],registers.r[regnum]);
      registers.r[0]=result;
      setcc(result);
      break;
    case 0xa4 : // subi r0
    case 0xa5 : // subi r1
    case 0xa6 : // subi r2
    case 0xa7 : // subi r3
      result=sub(registers.r[regnum],instrbyte2);
      registers.r[regnum]=result;
      setcc(result);
      break;
    case 0xa8 : // subr r0
    case 0xa9 : // subr r1
    case 0xaa : // subr r2
    case 0xab : // subr r3
    case 0xac : // suba r0
    case 0xad : // suba r1
    case 0xae : // suba r2
    case 0xaf : // suba r3
      result=sub(registers.r[regnum],ReadMemory(effaddr));
      registers.r[regnum]=result;
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// STR instructions
    case 0xc1 : // strz r1
    case 0xc2 : // strz r2
    case 0xc3 : // strz r3
      registers.r[regnum]=registers.r[0];
      break;
    case 0xc8 : // strr r0
    case 0xc9 : // strr r1
    case 0xca : // strr r2
    case 0xcb : // strr r3
    case 0xcc : // stra r0
    case 0xcd : // stra r1
    case 0xce : // stra r2
    case 0xcf : // stra r3
      WriteMemory(effaddr, registers.r[regnum]);
      cpucycles+=ind;
      ret=MEM_WRITE|effaddr;
      break;
// COM instructions
    case 0xe0 : // comz r0
    case 0xe1 : // comz r1
    case 0xe2 : // comz r2
    case 0xe3 : // comz r3
      result=com(registers.r[0],registers.r[regnum]);
      setcc(result);
      break;
    case 0xe4 : // comi r0
    case 0xe5 : // comi r1
    case 0xe6 : // comi r2
    case 0xe7 : // comi r3
      result=com(registers.r[regnum],instrbyte2);
      setcc(result);
      break;
    case 0xe8 : // comr r0
    case 0xe9 : // comr r1
    case 0xea : // comr r2
    case 0xeb : // comr r3
    case 0xec : // coma r0
    case 0xed : // coma r1
    case 0xee : // coma r2
    case 0xef : // coma r3
      result=com(registers.r[regnum],ReadMemory(effaddr));
      setcc(result);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
// 0x18+
    case 0x18 : // bctr  z
    case 0x19 : // bctr  p
    case 0x1a : // bctr  n
    case 0x1c : // bcta  z
    case 0x1d : // bcta  p
    case 0x1e : // bcta  n
      if((instrbyte1&0x03) == ((registers.psl>>6)&0x03))
      {
        address=effaddr;
        cpucycles+=ind;
      }
      break;
    case 0x1b : // bctr un
    case 0x1f : // bcta un
      address=effaddr;
      cpucycles+=ind;
      break;
// 0x38+
    case 0x38 : // bstr  z
    case 0x39 : // bstr  p
    case 0x3a : // bstr  n
    case 0x3c : // bsta  z
    case 0x3d : // bsta  p
    case 0x3e : // bsta  n
      if((instrbyte1&0x03) == ((registers.psl>>6)&0x03))
      {
        push(address);
        address=effaddr;
        cpucycles+=ind;
      }
      break;
    case 0x3b : // bstr un
    case 0x3f : // bsta un
      push(address);
      address=effaddr;
      cpucycles+=ind;
      break;
// 0x58+
    case 0x58 : // brnr r0
    case 0x59 : // brnr r1
    case 0x5a : // brnr r2
    case 0x5b : // brnr r3
    case 0x5c : // brna r0
    case 0x5d : // brna r1
    case 0x5e : // brna r2
    case 0x5f : // brna r3
      if(registers.r[regnum]!=0)
      {
        address=effaddr;
        cpucycles+=ind;
      }
      break;
// 0x78+
    case 0x78 : // bsnr r0
    case 0x79 : // bsnr r1
    case 0x7a : // bsnr r2
    case 0x7b : // bsnr r3
    case 0x7c : // bsna r0
    case 0x7d : // bsna r1
    case 0x7e : // bsna r2
    case 0x7f : // bsna r3
      if(registers.r[regnum]!=0)
      {
        push(address);
        address=effaddr;
        cpucycles+=ind;
      }
      break;
// 0x98+
    case 0x98 : // bcfr  z
    case 0x99 : // bcfr  p
    case 0x9a : // bcfr  n
    case 0x9c : // bcfa  z
    case 0x9d : // bcfa  p
    case 0x9e : // bcfa  n
      if((instrbyte1&0x03) != ((registers.psl>>6)&0x03))
      {
        address=effaddr;
        cpucycles+=ind;
      }
      break;
    case 0x9b : // zbrr
    case 0x9f : // bxa
      address=effaddr;
      cpucycles+=ind;
      break;
// 0xb8+
    case 0xb8 : // bsfr  z
    case 0xb9 : // bsfr  p
    case 0xba : // bsfr  n
    case 0xbc : // bsfa  z
    case 0xbd : // bsfa  p
    case 0xbe : // bsfa  n
      if((instrbyte1&0x03) != ((registers.psl>>6)&0x03))
      {
        push(address);
        address=effaddr;
        cpucycles+=ind;
      }
      break;
    case 0xbb : // zbsr
    case 0xbf : // bsxa
      push(address);
      address=effaddr;
      cpucycles+=ind;
      break;
// 0xd8+
    case 0xd8 : // birr r0
    case 0xd9 : // birr r1
    case 0xda : // birr r2
    case 0xdb : // birr r3
    case 0xdc : // bira r0
    case 0xdd : // bira r1
    case 0xde : // bira r2
    case 0xdf : // bira r3
      registers.r[regnum]++;
      if(registers.r[regnum]!=0)
      {
        address=effaddr;
        cpucycles+=ind;
      }
      break;
// 0xf8+
    case 0xf8 : // bdrr r0
    case 0xf9 : // bdrr r1
    case 0xfa : // bdrr r2
    case 0xfb : // bdrr r3
    case 0xfc : // bdra r0
    case 0xfd : // bdra r1
    case 0xfe : // bdra r2
    case 0xff : // bdra r3
      registers.r[regnum]--;
      if(registers.r[regnum]!=0)
      {
        address=effaddr;
        cpucycles+=ind;
      }
      break;

// 0x10+
    case 0x10 : // ldpl // 2650B only
      registers.psl=ReadMemory(effaddr);
      cpucycles+=ind;
      ret=MEM_READ|effaddr;
      break;
    case 0x11 : // stpl // 2650B only
      WriteMemory(effaddr, registers.psl);
      cpucycles+=ind;
      ret=MEM_WRITE|effaddr;
      break;
    case 0x12 : // spsu
      registers.r[0]=registers.psu;
      setcc(registers.r[0]);
      break;
    case 0x13 : // spsl
      registers.r[0]=registers.psl;
      setcc(registers.r[0]);
      break;

// 0x14+
    case 0x14 : // retc  z
    case 0x15 : // retc  p
    case 0x16 : // retc  n
      if((instrbyte1&0x03) == ((registers.psl>>6)&0x03))
        address=pull();
      break;
    case 0x17 : // retc un
      address=pull();
      break;

// 0x30+ // the C-port was used for several hardware functions e.g. the mdcr interface.
// bit 1 and 2 determine the interface
    case 0x30 : // redc r0
    case 0x31 : // redc r1
    case 0x32 : // redc r2
    case 0x33 : // redc r3
      result=ReadPortC();
      registers.r[regnum]=result;
      setcc(result);
      ret=PC_READ;
      break;

// 0x34+
    case 0x34 : // rete  z
    case 0x35 : // rete  p
    case 0x36 : // rete  n
      if((instrbyte1&0x03) == ((registers.psl>>6)&0x03))
      {
        registers.psu&=~II; // clear interrupt inhibit in the psu
        address=pull();
      }
      break;
    case 0x37 : // rete un
      registers.psu&=~II; // clear interrupt inhibit in the psu
      address=pull();
      break;

// 0x50+
    case 0x50 : // rrr r0
    case 0x51 : // rrr r1
    case 0x52 : // rrr r2
    case 0x53 : // rrr r3
      result=rrr(registers.r[regnum]);
      registers.r[regnum]=result;
      setcc(result);
      break;

// 0x54+
    case 0x54 : // rede r0
    case 0x55 : // rede r1
    case 0x56 : // rede r2
    case 0x57 : // rede r3
      ret=instrbyte2;
      registers.r[regnum]=ReadExtPort(ret);
      setcc(registers.r[regnum]);
      ret|=EXT_READ;
      break;

// 0x70+
    case 0x70 : // redd r0
    case 0x71 : // redd r1
    case 0x72 : // redd r2
    case 0x73 : // redd r3
      result=ReadPortD();
      registers.r[regnum]=result;
      setcc(result);
      ret=PD_READ;
      break;

// 0x74+
    case 0x74 : // cpsu
      registers.psu&=registers.psu&~instrbyte2;
      break;
    case 0x75 : // cpsl
      registers.psl&=registers.psl&~instrbyte2;
      break;
    case 0x76 : // ppsu
      registers.psu=registers.psu|instrbyte2;
      break;
    case 0x77 : // ppsl
      registers.psl=registers.psl|instrbyte2;
      break;

// 0x90+
    case 0x92 : // lpsu
      registers.psu=registers.r[0];
      break;
    case 0x93 : // lpsl
      registers.psl=registers.r[0];
      break;

// 0x90+
  case 0x94 : // dar r0
  case 0x95 : // dar r1
  case 0x96 : // dar r2
  case 0x97 : // dar r3
    result=registers.r[regnum];
    if((registers.psl&CY)==0)
      result=(result&0x0f)|((result+0xa0)&0xf0);
    if((registers.psl&IDC)==0)
      result=(result&0xf0)|((result+0x0a)&0x0f);
    registers.r[regnum]=result;
      break;

// 0xb0+
// this causes bank switching.
    case 0xb0 : // wrtc r0
    case 0xb1 : // wrtc r1
    case 0xb2 : // wrtc r2
    case 0xb3 : // wrtc r3
      WritePortC(registers.r[regnum]);
      ret=PC_WRITE;
      break;

    case 0xb4 : // tpsu
      result=registers.psu&instrbyte2;
      registers.psl&=~(CC0|CC1); // clear cc bits
      if(result!=instrbyte2)
        registers.psl|=0x80; // indicate not all selected bits are '1'
      break;
    case 0xb5 : // tpsl
      result=registers.psl&instrbyte2;
      registers.psl&=~(CC0|CC1); // clear cc bits
      if(result!=instrbyte2)
        registers.psl|=0x80; // indicate not all selected bits are '1'
      break;

// 0xd0+
    case 0xd0 : // rrl r0
    case 0xd1 : // rrl r1
    case 0xd2 : // rrl r2
    case 0xd3 : // rrl r3
      result=rrl(registers.r[regnum]);
      registers.r[regnum]=result;
      setcc(result);
      break;

// 0xb4+
    case 0xd4 : // wrte r0
    case 0xd5 : // wrte r1
    case 0xd6 : // wrte r2
    case 0xd7 : // wrte r3
      ret=instrbyte2;
      WritePortExt(ret, registers.r[regnum]);
      ret|=EXT_WRITE;
      break;

// 0xf0+
    case 0xf0 : // wrtd r0
    case 0xf1 : // wrtd r1
    case 0xf2 : // wrtd r2
    case 0xf3 : // wrtd r3
      WritePortD(registers.r[regnum]);
      ret=PD_WRITE;
      break;

// 0xf4+
    case 0xf4 : // tmi r0
    case 0xf5 : // tmi r1
    case 0xf6 : // tmi r2
    case 0xf7 : // tmi r3
      result=registers.r[regnum]&instrbyte2;
      registers.psl&=~(CC0|CC1); // clear cc bits
      if(result!=instrbyte2)
        registers.psl|=0x80; // indicate not all selected bits are '1'
      break;

// misc instructions
    case 0x40 : // halt
      ret = HALT|registers.ap;
      break;
    case 0xc0 : // nop
      break;    // do nothing

    default : // invalid opcode
      ret=(INV_OPCODE|registers.ap); // invalid instruction
      break;
  }

  registers.ea=effaddr;
  registers.na=address;
  return(ret);
}


void CPU2650::disassemble(void)
{
  unsigned char instrbyte1;
  unsigned char instrbyte2;
  unsigned char instrbyte3;
  unsigned short address;
  int numbytes;

  address = registers.ap;
  instrbyte1 =ReadMemory(address);
  numbytes=instrdata[instrbyte1].bytes;
  if(numbytes>1)
  {
    address=((address+1)&0x1fff)|(address&0x6000);
    instrbyte2 = ReadMemory(address);
    if(numbytes>2)
    {
      address=((address+1)&0x1fff)|(address&0x6000);
      instrbyte3 = ReadMemory(address);
    }
  }

  fprintf(stdout, "%04X %02X ", registers.ap, instrbyte1);
  if(numbytes>1)
    fprintf(stdout, "%02X ", instrbyte2);
  else
    fprintf(stdout, "   ");
  if(numbytes>2)
    fprintf(stdout, "%02X ", instrbyte3);
  else
    fprintf(stdout, "   ");
  fprintf(stdout, "%s ", instrdata[instrbyte1].mnem);
  switch(instrdata[instrbyte1].format)
  {
    case F_R :
    case F_A :
    case F_B :
    case F_C :
    case F_EB :
    case F_ER :
     fprintf(stdout, "%04X ", registers.ea);
     break;
    case F_I :
     fprintf(stdout, "%02X   ", instrbyte2);
     break;
    default :
     fprintf(stdout, "     ");
     break;
  }
  switch(instrdata[instrbyte1].format)
  {
    case F_R :
    case F_A :
    case F_B :
    case F_C :
    case F_EB :
    case F_ER :
      if((instrbyte2&0x80)!=0)
        fprintf(stdout, "* ");
      else
        fprintf(stdout, "  ");
      break;
    default :
      fprintf(stdout, "  ");
      break;
  }
  switch(instrdata[instrbyte1].format)
  {
    case F_A :
    case F_C :
      switch(instrbyte2&0x60)
      {
        case 0x20 :
          fprintf(stdout, "#+ ");
          break;
        case 0x40 :
          fprintf(stdout, "#- ");
          break;
        case 0x60 :
          fprintf(stdout, "#  ");
          break;
        default :
          fprintf(stdout, "   ");
          break;
      }
      break;
    default :
      fprintf(stdout, "   ");
      break;
  }
  fprintf(stdout, "%02X ", registers.r[0]);
  fprintf(stdout, "%02X ", registers.r[1]);
  fprintf(stdout, "%02X ", registers.r[2]);
  fprintf(stdout, "%02X ", registers.r[3]);
  fprintf(stdout, "%02X ", registers.r[4]);
  fprintf(stdout, "%02X ", registers.r[5]);
  fprintf(stdout, "%02X ", registers.r[6]);
  fprintf(stdout, "%02X ", registers.psu);
  fprintf(stdout, "%02X ", registers.psl);
  fprintf(stdout, "\n");
}

#if 0
    ret=cpu();
    if(options&PRINT!=0)
      disassemble();
    memcpy(&history[historyindex++], &registers, sizeof(struct regset2650));
    if(historyindex>=HISTORY_SIZE)
      historyindex=0;
#endif