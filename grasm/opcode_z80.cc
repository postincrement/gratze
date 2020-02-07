
#include "z80.h"

static const char * g_regNames[] = {
  "b", "c", "d",    "e",
  "h", "l", "(hl)", "a", 
  0
};

static const char * g_dregNames[] = {
  "bc", "de", "hl", "sp",
  0
};

static const char * g_adregNames[] = {
  "bc", "de", "hl", "af",
  0
};

static std::string ld_regs(uint8_t opcode)
{
  int from = opcode & 0x7;
  int to   = (opcode >> 3) & 0x7;
  return std::string(g_regNames[to]) + "," + g_regNames[from]; 
}

static std::string ld_n(uint8_t opcode)
{
  int to   = (opcode >> 3) & 0x7;
  return std::string(g_regNames[to]) + ",n"; 
}

static std::string ld_nn(uint8_t opcode)
{
  int to   = (opcode >> 4) & 0x3;
  return std::string(g_dregNames[to]) + ",n"; 
}

XSsembler::OpCodeInfo g_z80_opcodes[] = {
  { "nop",  (int)Z80Mode::eO,     0x00, 0xff, 1  },

  { "ld",   (int)Z80Mode::eDW | 
            (int)Z80Mode::eFunc,  0x01, 0xcf, 3, (void *)&ld_nn},

  { "inc",  (int)Z80Mode::eDW,    0x03, 0xcf, 1, (void *)g_dregNames},

  { "inc",  (int)Z80Mode::eD,     0x04, 0xc7, 1, (void *)g_regNames},
  { "dec",  (int)Z80Mode::eD,     0x05, 0xc7, 1, (void *)g_regNames},
  { "ld",   (int)Z80Mode::eI | 
            (int)Z80Mode::eFunc,  0x06, 0xc7, 2, (void *)&ld_n},

  { "rlca",     (int)Z80Mode::eO,     0x07, 0xff, 1  },
  { "ex af,af", (int)Z80Mode::eO,     0x08, 0xff, 1  },

  { "add hl,",  (int)Z80Mode::eDW,    0x09, 0xcf, 1, (void *)g_dregNames},

  { "dec",  (int)Z80Mode::eDW,    0x0b, 0xcf, 1, (void *)g_dregNames},

  { "rrca", (int)Z80Mode::eO,     0x0f, 0xff, 1  },

  { "djnz", (int)Z80Mode::eR,     0x10, 0xff, 2  },
  { "rla",  (int)Z80Mode::eO,     0x17, 0xff, 1  },
  { "jr",   (int)Z80Mode::eR,     0x18, 0xff, 2  },
  { "rra",  (int)Z80Mode::eO,     0x1f, 0xff, 1  },

  { "jrnz", (int)Z80Mode::eR,     0x20, 0xff, 2  },
  { "daa",  (int)Z80Mode::eO,     0x27, 0xff, 1  },
  { "jrz",  (int)Z80Mode::eR,     0x28, 0xff, 2  },
  { "cpl",  (int)Z80Mode::eO,     0x2f, 0xff, 1  },

  { "jrnc", (int)Z80Mode::eR,     0x30, 0xff, 2  },
  { "scf",  (int)Z80Mode::eO,     0x37, 0xff, 1  },
  { "jrc",  (int)Z80Mode::eR,     0x38, 0xff, 2  },
  { "ccf",  (int)Z80Mode::eO,     0x3f, 0xff, 1  },

  { "ld",   (int)Z80Mode::eRR | 
            (int)Z80Mode::eFunc,  0x40, 0xc0, 1, (void *)&ld_regs},

  { "halt", (int)Z80Mode::eO,     0x76, 0xff, 1  },

  { "add",  (int)Z80Mode::eD,     0x80, 0xf8, 1, (void *)g_regNames},
  { "adc",  (int)Z80Mode::eD,     0x88, 0xf8, 1, (void *)g_regNames},

  { "sub",  (int)Z80Mode::eD,     0x90, 0xf8, 1, (void *)g_regNames},
  { "sbc",  (int)Z80Mode::eD,     0x98, 0xf8, 1, (void *)g_regNames},

  { "and",  (int)Z80Mode::eD,     0xa0, 0xf8, 1, (void *)g_regNames},
  { "xor",  (int)Z80Mode::eD,     0xa8, 0xf8, 1, (void *)g_regNames},

  { "or",   (int)Z80Mode::eD,     0xb0, 0xf8, 1, (void *)g_regNames},
  { "cp",   (int)Z80Mode::eD,     0xb8, 0xf8, 1, (void *)g_regNames},

  { "pop",  (int)Z80Mode::eDW,    0xc1, 0xcf, 1, (void *)g_adregNames},
  { "jp",   (int)Z80Mode::eA,     0xc3, 0xff, 3  },
  { "push", (int)Z80Mode::eDW,    0xc5, 0xcf, 1, (void *)g_adregNames},
  { "rst",  (int)Z80Mode::eI,     0xc7, 0xc7, 1  },
  { "ret",  (int)Z80Mode::eO,     0xc9, 0xff, 3  },
  { "call", (int)Z80Mode::eA,     0xcd, 0xff, 3, },

  { "out",  (int)Z80Mode::eI,     0xd3, 0xff, 2, },
  { "in",   (int)Z80Mode::eI,     0xdb, 0xff, 2, },

  { "di",   (int)Z80Mode::eO,     0xf3, 0xff, 1  },
  { "ei",   (int)Z80Mode::eO,     0xfb, 0xff, 1  },

  { "ldir", (int)Z80Mode::eO,     0xed, 0xff, 2  },

  { 0, 0, 0, 0 }
};