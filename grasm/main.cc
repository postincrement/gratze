
#include <sstream>
#include <iomanip>

#include "assembler.h"
#include "disassembler.h"
#include "common/cmdargs.h"
#include "common/factory.h"

#include "s2650.h"

using namespace std;

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 'l', "",            ' ',   "generate listing file as basename.lst" },
  { 'L', "listing",     's',   "generate listing as filename" },
  { 'O', "output",      's',   "generate output as filename instead of basename.out" },
  { 's', "symbols",     ' ',   "include symbols in listing" },
  { 'p', "pagelength",  'u',   "set page length for listing (0 = no pages)" },

  { 'd', "disassemble", ' ',   "disassemble file"},
  { ' ', "org",         'x',   "set origin address for disassemble"},
  { ' ', "db",          'x',   "addresses to force as db"},

  { ' ', "matrix",      ' ',   "print an opcode matrix" },

  { 0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);
  if ((opt < 0) || (argc < 2)) {
    cerr << "usage: grasm [opts] inputfile\n"
         << "where opts are:\n"
         << args.Usage();
    return -1;
  }

  if (args.HasArg("--matrix")) {
    S2650Disassembler disassembler;
    disassembler.Matrix();
    return 0;
  }

  if (opt >= argc) {
    cerr << "error: no input filename specified" << endl;
    return -1;
  }

  if (args.HasArg("-d")) {
    cerr << "disassembling '" << argv[opt] << "'" << endl;
    S2650Disassembler disassembler;

    if (!disassembler.Open(args, argv[opt])) {
      cerr << "error: " << /*disassembler.GetError() << */ endl;
      return -1;
    }

    if (!disassembler.Run()) {
    }
  }
  else {
    cerr << "assembling '" << argv[opt] << "'" << endl;
    S2650Assembler assembler;

    if (!assembler.Open(args, argv[opt])) {
      cerr << "error: " << assembler.GetError() << endl;
      return -1;
    }

    bool result = assembler.Parse();
    cerr << assembler.GetLineCount() << " lines parsed" << endl;
    if (result) {
      assembler.WriteBinary();
      if (args.HasArg("-l") || args.HasArg("-L"))  
        assembler.WriteListing();
    }
  }
}