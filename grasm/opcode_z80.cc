
#include "z80.h"

XSsembler::OpCodeInfo g_z80_opcodes[] = {
  { "nop", 0,  0x00, 0xff, 1  },
  { 0, 0, 0, 0 }
};