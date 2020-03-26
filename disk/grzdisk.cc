#include <sstream>
#include <iostream>
#include <iomanip>

#include "../common/cmdargs.h"
#include "../common/misc.h"

#include "virtual_drive.h"

using namespace std;

#define DIR_TYPE      'D'

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 'v', "verbose",     ' ',   "enable verbose reporting" },
  { 'i', "info",        ' ',   "display info" },
  { 'd', "info",        ' ',   "display data" },
  { 'D', "dumpDir",     ' ',   "display dir sectors" },
  { ' ', "list",        ' ',   "list supported disk types"},

  { 0, 0, 0, 0}
};

bool IsEmpty(const uint8_t * data, int len)
{
  // look for empty chunk             
  bool empty = true;
  for (int i = 0; empty && (i < len); ++i) {
    if (data[i] != 0xe5)
      empty = false; 
  }

  return empty;
}

char IdentifyChunk(const uint8_t * data, int len)
{ 
  if (IsEmpty(data, len))
    return '.';

  // look for a CPM directory chunk
  bool isDir = true;
  for (int i = 0; i < 4; ++i) {
    const uint8_t * ptr = data + i*32;

    // unused entries are OK
    if (IsEmpty(ptr, 32))
      continue;

    // deleted entries are OK
    if (
        ((ptr[0] == 0xe5) || (ptr[0] < 32)) &&

        (isprint(ptr[1])) &&
        (isprint(ptr[2])) &&
        (isprint(ptr[3])) &&
        (isprint(ptr[4])) &&
        (isprint(ptr[5])) &&
        (isprint(ptr[6])) &&
        (isprint(ptr[7])) &&
        (isprint(ptr[8])) &&

        (isprint(ptr[9] & 0x7f)) &&
        (isprint(ptr[10] & 0x7f)) &&
        (isprint(ptr[11] & 0x7f))
      )
      ;
    else
      isDir = false;
  }  

  if (isDir)
    return DIR_TYPE;

  return '*';
}

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

  cout << "Format:      " << drive->GetFormat() << endl
       << "Capacity:    " << (capacity / 1024) << " k" << endl
       << "Sides:       " << drive->GetSides() << endl
       << "Density:     " << (drive->GetDensity() ? "double" : "single") << endl
       << "Tracks:      " << drive->GetTracks() << endl
       << "Sectors:     " << drive->GetSectors() << endl
       << "Sector size: " << drive->GetSectorSize() << endl;

  bool displayData = args.HasArg("-d");
  bool dumpDir = args.HasArg("-D");

  if (args.HasArg("-i")) {

    std::vector< std::vector< std::vector< std::vector<char> > > > diskData;
    std::vector<uint8_t> data;
    data.resize(drive->GetSectorSize());
    int chunkCount = drive->GetSectorSize() / 128;

    diskData.resize(drive->GetTracks());
    for (int track = 0; track < drive->GetTracks(); ++track) {
      auto & trackData = diskData[track];
      trackData.resize(drive->GetSides());
      for (int side = 0; side < drive->GetSides(); ++side) {
        auto & sideData = trackData[side];

        sideData.resize(drive->GetSectors());
        for (int sector = 0; sector < drive->GetSectors(); ++sector) {

          auto & sectorData = sideData[sector];
          sectorData.resize(chunkCount);

          VirtualDrive::SectorInfo sectorInfo;
          int len = drive->ReadSector(track, side, sector, sectorInfo, &data[0], data.size());
          if (len < 0) {
            for (auto & r : sectorData)
              r = 'X';
            if (displayData) {
              cout << "Track " << track << ", side " << side << ", sector " << sector+1 << endl;
              cout << "Could not read" << endl;
            }
          }
          else {
            bool isDir = false;
            for (int chunk = 0; chunk < chunkCount; ++chunk) {
              sectorData[chunk] = IdentifyChunk(&data[chunk*128], 128);
              isDir = isDir || (sectorData[chunk] == DIR_TYPE);
            }
            if (displayData || (dumpDir && isDir)) {
              cout << "Track " << track << ", side " << side << ", sector " << sector+1 << endl;
              cout << "Type ";
              for (int chunk = 0; chunk < chunkCount; ++chunk) {
                cout << sectorData[chunk];
              }
              cout << endl;  
              cout << DumpMemory(&data[0], len);
              cout << endl;
            }
          }  
        }
      }
    }

    for (int track = 0; track < drive->GetTracks(); ++track) {
      cout << setw(2) << track << "   ";
      for (int side = 0; side < drive->GetSides(); ++side) {
        cout << " ";
        for (int sector = 0; sector < drive->GetSectors(); ++sector) {
          for (auto & chunk : diskData[track][side][sector]) {
            cout << chunk;
          }
        }
      }
      cout << endl;
    }    
  }     

  return 0;
}