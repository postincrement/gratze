#include <sstream>
#include <iostream>
#include <iomanip>
#include <set>

#include "../common/cmdargs.h"
#include "../common/misc.h"
#include "../common/binfile.h"

using namespace std;

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 'v', "verbose",     ' ',   "enable verbose reporting" },
  { ' ', "list",        ' ',   "list supported binary types"},

  { 0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);
  if ((opt < 0) || (argc < 1)) {
    cerr << "usage: grzbin [opts] inputfile\n"
         << "where opts are:\n"
         << args.Usage();
    return -1;
  }

  if (args.HasArg("--list")) {
    std::vector<std::string> types;
    BINFile::GetExtensions(types);

    if (types.size() == 0) {
      cerr << "error: no types registered" << endl;
      return -1;
    }

    stringstream strm;
    for (auto & r : types)
      strm << r << endl;
    cout << strm.str();

    return 0;
  }

  if (args.HasArg("-h")) {
    cout << args.Usage();
    return 0;
  }

  BINFileIdentifier id;
  BINFile * file = id.Open(argv[opt], true);
  if (file == nullptr) {
    cerr << "error: cannot open '" << argv[opt] << "'" << endl;
    return -1;
  }

  cerr << "info: reading '" << argv[opt] << "'" << endl;
  BINFile::Data data;
  if (!file->Load(data)) {
    cerr << "error: could not load file" << endl;
    return -1;
  }

  cout << "Block count:  "    << data.m_blocks.size() << "\n"
       << "Exec address: ";
  uint16_t execAddr;     
  if (file->GetExecAddr(execAddr))
    cout << HEXFORMAT0x4(execAddr);
  else
    cout << "none";
  cout << "\n"
       << "Total data size: " << data.m_dataSize << "\n"
       ;

  for (auto & r : data.m_blocks) {
    cout << DumpMemory(r.m_addr, &r.m_data[0], r.m_data.size());
    cout << endl;
  }     

  delete file;
  return 0;  
}