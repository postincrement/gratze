
#include "assembler.h"

OpCodeInfo g_2650_opcodes[] = {
  { "lodz", 0, 0x00, 1 },
  { "lodi", 1, 0x04, 2 },
  { "lodr", 2, 0x08, 2 },
  { "loda", 3, 0x0c, 3 },

  { "strz", 0, 0xc0, 1 },
  { "stri", 1, 0xc4, 2 },
  { "strr", 2, 0xc8, 2 },
  { "stra", 3, 0xcc, 3 },

  { "addz", 0, 0x80, 1 },
  { "addi", 1, 0x84, 2 },
  { "addr", 2, 0x88, 2 },
  { "adda", 3, 0xcc, 3 },

  { "subz", 0, 0xa0, 1 },
  { "subi", 1, 0xa4, 2 },
  { "subr", 2, 0xa8, 2 },
  { "suba", 3, 0xac, 3 },

  { "dar",  0, 0x94, 1 },

  { "andz", 0, 0x40, 1 },
  { "andi", 1, 0x44, 2 },
  { "andr", 2, 0x48, 2 },
  { "anda", 3, 0x4c, 3 },

  { "iorz", 0, 0x60, 1 },
  { "iori", 1, 0x64, 2 },
  { "iorr", 2, 0x68, 2 },
  { "iora", 3, 0x6c, 3 },

  { "eorz", 0, 0x20, 1 },
  { "eori", 1, 0x24, 2 },
  { "eorr", 2, 0x28, 2 },
  { "eora", 3, 0x2c, 3 },
  
  { "comz", 0, 0xe0, 1 },
  { "comi", 1, 0xe4, 2 },
  { "comr", 2, 0xe8, 2 },
  { "coma", 3, 0xec, 3 },

  { "rrr",  9, 0x50, 1 },
  { "rrl",  9, 0xd0, 1 },

  { "bctr", 4 + 2, 0x18, 2 },
  { "bcta", 4 + 3, 0x1c, 3 },

  { "bcfr", 4 + 2, 0x98, 2 },
  { "bcfa", 4 + 3, 0x9c, 3 },

  { "brnr", 4 + 2, 0x58, 2 },
  { "brna", 4 + 3, 0x5c, 3 },

  { "birr", 4 + 2, 0xd8, 2 },
  { "bira", 4 + 3, 0xdc, 3 },

  { "bdrr", 4 + 2, 0xf8, 2 },
  { "bdra", 4 + 3, 0xfc, 3 },

  { "zbrr", 2, 0x9b, 2 },
  { "bxa",  3, 0x9f, 3 },

  { "bstr", 4 + 2, 0x38, 2 },
  { "bsta", 4 + 3, 0x3c, 3 },

  { "bsfr", 4 + 2, 0xb8, 2 },
  { "bsfa", 4 + 3, 0xbc, 3 },

  { "bsnr", 4 + 2, 0x78, 2 },
  { "bsna", 4 + 3, 0x7c, 3 },

  { "zbsr", 2, 0xbb, 2 },
  { "bsxa", 3, 0xbf, 3 },

  { "retc", 4 + 0, 0x14, 1 },
  { "rete", 4 + 0, 0x34, 1 },

  { "wrtd", -1,    0xf0, 1 },
  { "redd", 8,     0x70, 2 },
  { "wrtc", -1,    0xb0, 1 },
  { "redc", 8,     0x30, 2 },

  { "wrtc", 1,     0xd4, 2 },
  { "rede", 1,     0x54, 2 },

  { "halt", -1,     0x40, 1 },
  { "nop",  -1,     0xc0, 1 },

  { "tmi",  1,     0xf4, 1 },

  { "lpsu", -1,     0x92, 1 },
  { "lpsl", -1,     0x93, 1 },

  { "spsu", -1,     0x12, 1 },
  { "spsl", -1,     0x13, 1 },

  { "cpsu", 8,     0x78, 2 },
  { "cpsl", 8,     0x79, 2 },

  { "ppsu", 8,     0x76, 2 },
  { "ppsl", 8,     0x77, 2 },

  { "tpsu", 8,     0xb4, 2 },
  { "tpsl", 8,     0xb5, 2 },

  { 0 }
};


