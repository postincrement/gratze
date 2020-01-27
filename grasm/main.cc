
#include <sstream>
#include <iomanip>

#include "assembler.h"
#include "disassembler.h"
#include "common/cmdargs.h"
#include "common/factory.h"

#include "s2650.h"
#include "z80.h"

using namespace std;

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 't', "type",        's',   "select processor type"},
  { ' ', "list",        ' ',   "list supported processor types"},

  { 'a', "assemble",    ' ',   "assemble file" },
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


static Factory<ProcessorType, std::string> g_typeFactory;

std::string ListProcessorTypes()
{
  std::vector<std::string> types;
  g_typeFactory.GetKeys(types);

  stringstream strm;
  for (auto & r : types)
    strm << r << endl;
  return strm.str();  
}


int main(int argc, char *argv[])
{
  g_typeFactory.AddConcreteClass<S2650Processor>("2650");
  g_typeFactory.AddConcreteClass<Z80Processor>("z80");

  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);
  if ((opt < 0) || (argc < 2)) {
    cerr << "usage: grasm [opts] inputfile\n"
         << "where opts are:\n"
         << args.Usage();
    return -1;
  }

  if (args.HasArg("--list")) {
    cout << ListProcessorTypes();
    return 0;
  }

  std::string type;
  if (!args.GetValue("-t", type)) {
    cerr << "error: must specify processor type." << endl;
    cerr << ListProcessorTypes();
    return -1;
  }

  ProcessorType * processor = g_typeFactory.CreateInstance(type);
  if (processor == nullptr) {
    cerr << "error: unknown processor '" << type << "'" << endl;
    return -1;
  }

  if (args.HasArg("--matrix")) {
    Disassembler * disassembler = processor->CreateDisassembler();
    if (disassembler == nullptr) {
      cerr << "error: processor '" << type << "' does not have a disassembler" << endl;
      return -1;
    }
    disassembler->Matrix();
    return 0;
  }

  if (opt >= argc) {
    cerr << "error: no input filename specified" << endl;
    return -1;
  }

  if (args.HasArg("-d")) {
    Disassembler * disassembler = processor->CreateDisassembler();
    if (disassembler == nullptr) {
      cerr << "error: processor '" << type << "' does not have a disassembler" << endl;
      return -1;
    }
    cerr << "disassembling '" << argv[opt] << "'" << endl;
    if (!disassembler->Open(args, argv[opt])) {
      cerr << "error: " << /*disassembler.GetError() << */ endl;
      return -1;
    }

    if (!disassembler->Run()) {
    }

    return 0;
  }
  else if (args.HasArg("-a")) {
    Assembler * assembler = processor->CreateAssembler();
    if (assembler == nullptr) {
      cerr << "error: processor '" << type << "' does not have an assembler" << endl;
      return -1;
    }
    cerr << "assembling '" << argv[opt] << "'" << endl;
    if (!assembler->Open(args, argv[opt])) {
      cerr << "error: " << assembler->GetError() << endl;
      return -1;
    }

    bool result = assembler->Parse();
    cerr << assembler->GetLineCount() << " lines parsed" << endl;
    if (result) {
      assembler->WriteBinary();
      if (args.HasArg("-l") || args.HasArg("-L"))  
        assembler->WriteListing();
    }
    return 0;
  }

  cerr << "error: select -a, -d, or --matrix" << endl;
  return -1;
}