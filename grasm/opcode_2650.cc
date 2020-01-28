
#include "s2650.h"

static const char * g_regs[] = { "r0", "r1", "r2", "r3", 0 }; 
static const char * g_cond[] = {  "z", "gt", "lt", "un", 0 }; 

XSsembler::OpCodeInfo g_2650_opcodes[] = {
  { "lodz", (int)S2650Mode::eZ,  0x00, 0xfc, 1, g_regs },
  { "lodi", (int)S2650Mode::eI,  0x04, 0xfc, 2, g_regs },
  { "lodr", (int)S2650Mode::eR,  0x08, 0xfc, 2, g_regs },
  { "loda", (int)S2650Mode::eA,  0x0c, 0xfc, 3, g_regs },

  { "spsu", (int)S2650Mode::eE,  0x12, 0xff, 1 },
  { "spsl", (int)S2650Mode::eE,  0x13, 0xff, 1 },

  { "retc", (int)S2650Mode::eZ,                     0x14, 0xfc, 1, g_cond },
  { "retc", (int)S2650Mode::eZ | XSsembler::eTerm,  0x17, 0xff, 1, (void *)"un" },

  { "bctr", (int)S2650Mode::eR,                     0x18, 0xfc, 2, g_cond },
  { "bctr", (int)S2650Mode::eR | XSsembler::eTerm,  0x1b, 0xff, 2,  (void *)"un" },
  { "bcta", (int)S2650Mode::eB,                     0x1c, 0xfc, 3, g_cond },
  { "bcta", (int)S2650Mode::eB | XSsembler::eTerm,  0x1f, 0xff, 3,  (void *)"un" },

  { "eorz", (int)S2650Mode::eZ,  0x20, 0xfc, 1, g_regs },
  { "eori", (int)S2650Mode::eI,  0x24, 0xfc, 2, g_regs },
  { "eorr", (int)S2650Mode::eR,  0x28, 0xfc, 2, g_regs },
  { "eora", (int)S2650Mode::eA,  0x2c, 0xfc, 3, g_regs },

  { "redc", (int)S2650Mode::eZ,  0x30, 0xfc, 2, g_regs },

  { "rete", (int)S2650Mode::eZ,  0x34, 0xfc, 1, g_regs },

  { "bstr", (int)S2650Mode::eR,  0x38, 0xfc, 2, g_cond },
  { "bsta", (int)S2650Mode::eB,  0x3c, 0xfc, 3, g_cond },
  
  { "halt", (int)S2650Mode::eE,  0x40, 0xff, 1 },

  { "andz", (int)S2650Mode::eZ,  0x41, 0xff, 1, (void *)"r1" },
  { "andz", (int)S2650Mode::eZ,  0x42, 0xff, 1, (void *)"r2" },
  { "andz", (int)S2650Mode::eZ,  0x43, 0xff, 1, (void *)"r3" },
  { "andi", (int)S2650Mode::eI,  0x44, 0xfc, 2, g_regs },
  { "andr", (int)S2650Mode::eR,  0x48, 0xfc, 2, g_regs },
  { "anda", (int)S2650Mode::eA,  0x4c, 0xfc, 3, g_regs },

  { "rrr",  (int)S2650Mode::eZ,  0x50, 0xfc, 1, g_regs },

  { "rede", (int)S2650Mode::eZ,  0x54, 0xfc, 2, g_regs },

  { "brnr", (int)S2650Mode::eR,  0x58, 0xfc, 2, g_regs },
  { "brna", (int)S2650Mode::eB,  0x5c, 0xfc, 3, g_regs },

  { "iorz", (int)S2650Mode::eZ,  0x60, 0xfc, 1, g_regs },
  { "iori", (int)S2650Mode::eI,  0x64, 0xfc, 2, g_regs },
  { "iorr", (int)S2650Mode::eR,  0x68, 0xfc, 2, g_regs },
  { "iora", (int)S2650Mode::eA,  0x6c, 0xfc, 3, g_regs },

  { "redd", (int)S2650Mode::eZ,  0x70, 0xfc, 2, g_regs },

  { "cpsu", (int)S2650Mode::eIn, 0x74, 0xff, 2 },
  { "cpsl", (int)S2650Mode::eIn, 0x75, 0xff, 2 },

  { "ppsu", (int)S2650Mode::eIn, 0x76, 0xff, 2 },
  { "ppsl", (int)S2650Mode::eIn, 0x77, 0xff, 2 },

  { "bsnr", (int)S2650Mode::eR,  0x78, 0xfc, 2, g_regs },
  { "bsna", (int)S2650Mode::eB,  0x7c, 0xfc, 3, g_regs },

  { "addz", (int)S2650Mode::eZ,  0x80, 0xfc, 1, g_regs },
  { "addi", (int)S2650Mode::eI,  0x84, 0xfc, 2, g_regs },
  { "addr", (int)S2650Mode::eR,  0x88, 0xfc, 2, g_regs },
  { "adda", (int)S2650Mode::eA,  0x8c, 0xfc, 3, g_regs },

  { "lpsu", (int)S2650Mode::eE,  0x92, 0xff, 1 },
  { "lpsl", (int)S2650Mode::eE,  0x93, 0xff, 1 },

  { "dar",  (int)S2650Mode::eZ,  0x94, 0xff, 1 },

  { "bcfr", (int)S2650Mode::eR,  0x98, 0xfc, 2, g_cond },

  { "zbrr", (int)S2650Mode::eE,  0x9b, 0xfc, 2, g_cond },

  { "bcfa", (int)S2650Mode::eB,  0x9c, 0xfc, 3, g_cond },
  { "bxa",  (int)S2650Mode::eE,  0x9f, 0xfc, 3, g_cond },

  { "subz", (int)S2650Mode::eZ,  0xa0, 0xfc, 1, g_regs },
  { "subi", (int)S2650Mode::eI,  0xa4, 0xfc, 2, g_regs },
  { "subr", (int)S2650Mode::eR,  0xa8, 0xfc, 2, g_regs },
  { "suba", (int)S2650Mode::eA,  0xac, 0xfc, 3, g_regs },

  { "wrtc", (int)S2650Mode::eZ,  0xb0, 0xfc, 1, g_regs },

  { "tpsu", (int)S2650Mode::eIn, 0xb4, 0xfc, 2, g_regs },
  { "tpsl", (int)S2650Mode::eIn, 0xb5, 0xfc, 2, g_regs },

  { "bsfr", (int)S2650Mode::eR,  0xb8, 0xfc, 2, g_cond },
  { "zbsr", (int)S2650Mode::eE,  0xbb, 0xfc, 2, g_cond },
 
  { "bsfa", (int)S2650Mode::eB,  0xbc, 0xfc, 3, g_cond },
  { "bsxa", (int)S2650Mode::eE,  0xbf, 0xfc, 3, g_cond },

  { "nop",  (int)S2650Mode::eE,  0xc0, 0xff, 1 },

  { "strz", (int)S2650Mode::eZ,  0xc1, 0xff, 1, (void *)"r1" },
  { "strz", (int)S2650Mode::eZ,  0xc2, 0xff, 1, (void *)"r2" },
  { "strz", (int)S2650Mode::eZ,  0xc3, 0xff, 1, (void *)"r3" },
  { "stri", (int)S2650Mode::eI,  0xc4, 0xfc, 2, g_regs },
  { "strr", (int)S2650Mode::eR,  0xc8, 0xfc, 2, g_regs },
  { "stra", (int)S2650Mode::eA,  0xcc, 0xfc, 3, g_regs },

  { "rrl",  (int)S2650Mode::eZ,  0xd0, 0xfc, 1, g_regs },

  { "wrte", (int)S2650Mode::eI,  0xd4, 0xfc, 2, g_regs },

  { "birr", (int)S2650Mode::eR,  0xd8, 0xfc, 2, g_regs },
  { "bira", (int)S2650Mode::eB,  0xdc, 0xfc, 3, g_regs },

  { "comz", (int)S2650Mode::eZ,  0xe0, 0xfc, 1, g_regs },
  { "comi", (int)S2650Mode::eI,  0xe4, 0xfc, 2, g_regs },
  { "comr", (int)S2650Mode::eR,  0xe8, 0xfc, 2, g_regs },
  { "coma", (int)S2650Mode::eA,  0xec, 0xfc, 3, g_regs },

  { "wrtd", (int)S2650Mode::eZ,  0xf0, 0xfc, 1, g_regs },

  { "tmi",  (int)S2650Mode::eI,  0xf4, 0xff, 1 },

  { "bdrr", (int)S2650Mode::eR,  0xf8, 0xfc, 2, g_cond },
  { "bdra", (int)S2650Mode::eB,  0xfc, 0xfc, 3, g_cond },

  { 0 }
};


