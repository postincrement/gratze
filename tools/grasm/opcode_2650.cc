
#include "s2650.h"

OpCodeInfo g_2650_opcodes[] = {
  { "lodz", (int)S2650Mode::eZ,  0x00, 1 },
  { "lodi", (int)S2650Mode::eI,  0x04, 2 },
  { "lodr", (int)S2650Mode::eR,  0x08, 2 },
  { "loda", (int)S2650Mode::eA,  0x0c, 3 },

  { "strz", (int)S2650Mode::eZ,  0xc0, 1 },
  { "stri", (int)S2650Mode::eI,  0xc4, 2 },
  { "strr", (int)S2650Mode::eR,  0xc8, 2 },
  { "stra", (int)S2650Mode::eA,  0xcc, 3 },

  { "addz", (int)S2650Mode::eZ,  0x80, 1 },
  { "addi", (int)S2650Mode::eI,  0x84, 2 },
  { "addr", (int)S2650Mode::eR,  0x88, 2 },
  { "adda", (int)S2650Mode::eA,  0x8c, 3 },

  { "subz", (int)S2650Mode::eZ,  0xa0, 1 },
  { "subi", (int)S2650Mode::eI,  0xa4, 2 },
  { "subr", (int)S2650Mode::eR,  0xa8, 2 },
  { "suba", (int)S2650Mode::eA,  0xac, 3 },

  { "dar",  (int)S2650Mode::eZ,  0x94, 1 },

  { "andz", (int)S2650Mode::eZ,  0x40, 1 },
  { "andi", (int)S2650Mode::eI,  0x44, 2 },
  { "andr", (int)S2650Mode::eR,  0x48, 2 },
  { "anda", (int)S2650Mode::eA,  0x4c, 3 },

  { "iorz", (int)S2650Mode::eZ,  0x60, 1 },
  { "iori", (int)S2650Mode::eI,  0x64, 2 },
  { "iorr", (int)S2650Mode::eR,  0x68, 2 },
  { "iora", (int)S2650Mode::eA,  0x6c, 3 },

  { "eorz", (int)S2650Mode::eZ,  0x20, 1 },
  { "eori", (int)S2650Mode::eI,  0x24, 2 },
  { "eorr", (int)S2650Mode::eR,  0x28, 2 },
  { "eora", (int)S2650Mode::eA,  0x2c, 3 },

  { "comz", (int)S2650Mode::eZ,  0xe0, 1 },
  { "comi", (int)S2650Mode::eI,  0xe4, 2 },
  { "comr", (int)S2650Mode::eR,  0xe8, 2 },
  { "coma", (int)S2650Mode::eA,  0xec, 3 },

  { "rrr",  (int)S2650Mode::eZ,  0x50, 1 },
  { "rrl",  (int)S2650Mode::eZ,  0xd0, 1 },

  { "bctr", (int)S2650Mode::eR,  0x18, 2 },
  { "bcta", (int)S2650Mode::eB,  0x1c, 3 },

  { "bcfr", (int)S2650Mode::eR,  0x98, 2 },
  { "bcfa", (int)S2650Mode::eB,  0x9c, 3 },

  { "brnr", (int)S2650Mode::eR,  0x58, 2 },
  { "brna", (int)S2650Mode::eB,  0x5c, 3 },

  { "birr", (int)S2650Mode::eR,  0xd8, 2 },
  { "bira", (int)S2650Mode::eB,  0xdc, 3 },

  { "bdrr", (int)S2650Mode::eR,  0xf8, 2 },
  { "bdra", (int)S2650Mode::eB,  0xfc, 3 },

  { "zbrr", (int)S2650Mode::eE,  0x9b, 2 },
  { "bxa",  (int)S2650Mode::eE,  0x9f, 3 },

  { "bstr", (int)S2650Mode::eR,  0x38, 2 },
  { "bsta", (int)S2650Mode::eB,  0x3c, 3 },

  { "bsfr", (int)S2650Mode::eR,  0xb8, 2 },
  { "bsfa", (int)S2650Mode::eB,  0xbc, 3 },

  { "bsnr", (int)S2650Mode::eR,  0x78, 2 },
  { "bsna", (int)S2650Mode::eB,  0x7c, 3 },

  { "zbsr", (int)S2650Mode::eE,  0xbb, 2 },
  { "bsxa", (int)S2650Mode::eE,  0xbf, 3 },

  { "retc", (int)S2650Mode::eZ,  0x14, 1 },
  { "rete", (int)S2650Mode::eZ,  0x34, 1 },

  { "wrtd", (int)S2650Mode::eZ,  0xf0, 1 },
  { "redd", (int)S2650Mode::eZ,  0x70, 2 },
  { "wrtc", (int)S2650Mode::eZ,  0xb0, 1 },
  { "redc", (int)S2650Mode::eZ,  0x30, 2 },

  { "wrte", (int)S2650Mode::eI,  0xd4, 2 },
  { "rede", (int)S2650Mode::eZ,  0x54, 2 },

  { "halt", (int)S2650Mode::eE,  0x40, 1 },
  { "nop",  (int)S2650Mode::eE,  0xc0, 1 },

  { "tmi",  (int)S2650Mode::eI,  0xf4, 1 },

  { "lpsu", (int)S2650Mode::eE,  0x92, 1 },
  { "lpsl", (int)S2650Mode::eE,  0x93, 1 },

  { "spsu", (int)S2650Mode::eE,  0x12, 1 },
  { "spsl", (int)S2650Mode::eE,  0x13, 1 },

  { "cpsu", (int)S2650Mode::eIn, 0x74, 2 },
  { "cpsl", (int)S2650Mode::eIn, 0x75, 2 },

  { "ppsu", (int)S2650Mode::eIn, 0x76, 2 },
  { "ppsl", (int)S2650Mode::eIn, 0x77, 2 },

  { "tpsu", (int)S2650Mode::eIn, 0xb4, 2 },
  { "tpsl", (int)S2650Mode::eIn, 0xb5, 2 },

  { 0 }
};


