#include <sstream>
#include <iostream>
#include <iomanip>

#include "../common/cmdargs.h"

#include "virtual_drive.h"

using namespace std;

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 'v', "verbose",     ' ',   "enable verbose reporting" },
  { ' ', "list",        ' ',   "list supported disk types"},

  { 0, 0, 0, 0}
};


int main(int argc, char *argv[])
{
  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);
  if ((opt < 0) || (argc < 2)) {
    cerr << "usage: grzdisk [opts] inputfile\n"
         << "where opts are:\n"
         << args.Usage();
    return -1;
  }

  VirtualDrive::Init();

  if (args.HasArg("--list")) {
    std::vector<std::string> types;
    VirtualDrive::m_virtualDriveFactory.GetKeys(types);

    stringstream strm;
    for (auto & r : types)
      strm << r << endl;
    cout << strm.str();

    return 0;
  }

  std::string fn(argv[opt]);
  VirtualFileIdentifier fileId;

  if (args.HasArg("-v"))
    fileId.SetVerbose(true);
    
  VirtualDrive * drive = fileId.Open(fn, true);
  if (drive == nullptr) {
    cerr << "error: " << fileId.GetError() << endl;
    return -1;
  }

  int capacity = drive->GetSides() * drive->GetTracks() * drive->GetSectors() * drive->GetSectorSize();

  cout << "Format:      " << drive->GetName() << endl
       << "Capacity:    " << (capacity / 1024) << " k" << endl
       << "Sides:       " << drive->GetSides() << endl
       << "Density:     " << (drive->GetDensity() ? "double" : "single") << endl
       << "Tracks:      " << drive->GetTracks() << endl
       << "Sectors:     " << drive->GetSectors() << endl
       << "Sector size: " << drive->GetSectorSize() << endl;

  return 0;
}