#ifndef M6800_H_
#define M6800_H_

#include <stdint.h>

#define M6800_STATUS_CARRY      (1 << 0)
#define M6800_STATUS_OVERFLOW   (1 << 1)
#define M6800_STATUS_ZERO       (1 << 2)
#define M6800_STATUS_SIGN       (1 << 3)
#define M6800_STATUS_IE         (1 << 4)
#define M6800_STATUS_AC         (1 << 5)

#define M6800_STATUS_MASK       (0x3f)
#define M6800_STATUS_ONES       (0xc0)

typedef struct
{
  uint8_t m_A;
  uint8_t m_B;
  uint16_t m_X;
  uint16_t m_PC;
  uint16_t m_SP;
  uint8_t m_status;
} M6800_regs;

struct M6800_;

#define MODE_NONE   1
#define MODE_INHER  1
#define MODE_REL    2
#define MODE_EXT    3
#define MODE_IMM    4

typedef struct M6800_
{
  M6800_regs m_regs;
  uint8_t (* m_readMem)(uint16_t addr);
  void (* m_writeMem)(uint16_t addr, uint8_t data);
  void (*m_trap)(uint16_t addr, uint8_t opcode);

  int (*m_exec)(struct M6800_ * cpu);
  uint16_t m_insAddr;
  uint8_t m_ins[3];
  uint8_t m_insLen;
  uint8_t m_cycles;
}  M6800;

extern void M6800_Init(M6800 * cpu);
extern int M6800_Run(M6800 * cpu, int instructionCount);
extern uint8_t * M6800_GetInstruction(M6800 * cpu, int * len);
extern void M6800_Disassemble(uint8_t * code, int len, char * text, int textLen);

#endif // M6800_H_
