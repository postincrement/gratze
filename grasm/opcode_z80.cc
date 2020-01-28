
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


XSsembler::OpCodeInfo g_z80_opcodes[] = {
  { "nop",  (int)Z80Mode::eO,  0x00, 0xff, 1  },

  { "inc",  (int)Z80Mode::eDW, 0x03, 0xcf, 1, (void *)g_dregNames},

  { "inc",  (int)Z80Mode::eD,  0x04, 0xc7, 1, (void *)g_regNames},
  { "dec",  (int)Z80Mode::eD,  0x05, 0xc7, 1, (void *)g_regNames},

  { "dec",  (int)Z80Mode::eDW, 0x0b, 0xcf, 1, (void *)g_dregNames},

  { "ld",   (int)Z80Mode::eRR, 0x40, 0xc0, 1, (void *)g_regNames},
  { "halt", (int)Z80Mode::eO,  0x76, 0xff, 1  },

  { "add",  (int)Z80Mode::eD,  0x80, 0xf8, 1, (void *)g_regNames},
  { "adc",  (int)Z80Mode::eD,  0x88, 0xf8, 1, (void *)g_regNames},

  { "sub",  (int)Z80Mode::eD,  0x90, 0xf8, 1, (void *)g_regNames},
  { "sbc",  (int)Z80Mode::eD,  0x98, 0xf8, 1, (void *)g_regNames},

  { "and",  (int)Z80Mode::eD,  0xa0, 0xf8, 1, (void *)g_regNames},
  { "xor",  (int)Z80Mode::eD,  0xa8, 0xf8, 1, (void *)g_regNames},

  { "or",   (int)Z80Mode::eD,  0xb0, 0xf8, 1, (void *)g_regNames},
  { "cp",   (int)Z80Mode::eD,  0xb8, 0xf8, 1, (void *)g_regNames},

  { "djnz", (int)Z80Mode::eR,  0x10, 0xff, 2  },
  { "jrnz", (int)Z80Mode::eR,  0x20, 0xff, 2  },


  { 0, 0, 0, 0 }
};