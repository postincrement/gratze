#include <sstream>
#include <iostream>
#include <iomanip>
#include <set>

#include "../common/cmdargs.h"
#include "../common/misc.h"

#include "virtual_drive.h"
#include "cpmfs.h"

using namespace std;

#define NO_CHUNK     " "
#define EMPTY_CHUNK  "."
#define DIR_CHUNK    "D"
#define NORM_CHUNK   "*"

#define CHUNK_SIZE    128

static CommandLineArgs::Option g_options[] = {
  { 'h', "help",        ' ',   "display this help message" },
  { 'v', "verbose",     ' ',   "enable verbose reporting" },
  { 'm', "map",         ' ',   "display sector map" },
  { 'd', "data",        ' ',   "display all data" },
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

std::string IdentifyChunk(const uint8_t * data, int len)
{ 
  if (IsEmpty(data, len))
    return EMPTY_CHUNK;

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
    return DIR_CHUNK;

  return NORM_CHUNK;
}



bool DisplayInfo(std::shared_ptr<VirtualDrive> drive, bool displayMap, bool displayData, bool displayDir)
{
  VirtualDrive::SideList & sideList = drive->GetSides();

  if (sideList.size() == 0) {
    cerr << "error: disk has no sides" << endl;
    return false;
  }

  VirtualDrive::SideInfo & side0 = sideList.begin()->second;
  VirtualDrive::TrackList & side0Tracks = side0.GetTracks();

  if (side0Tracks.size() == 0) {
    cerr << "error: side 0 has no tracks" << endl;
    return false;
  }

  VirtualDrive::TrackInfo & side0Track0         = side0Tracks.begin()->second;
  VirtualDrive::SectorList & side0Track0Sectors = side0Track0.GetSectors();

  if (side0Track0Sectors.size() == 0) {
    cerr << "error: side 0, track 0 has no sectors" << endl;
    return false;
  }

  // calculate capacity and get various min and max numbers
  std::set<int> sectorSizes;
  std::set<int> sectorNums;
  std::set<int> trackNums;
  std::set<int> sideNums;

  int capacity = 0;
  for (auto & s : sideList) {
    sideNums.insert(s.first);
    for (auto & t : s.second.GetTracks()) {
      trackNums.insert(t.first);
      for (auto & e : t.second.GetSectors()) {
        sectorNums.insert(e.m_id);
        sectorSizes.insert(e.m_size);
        capacity += e.m_size;
      }
    }
  }
  
  cout << "Format:      " << drive->GetFormat() << endl
       << "Capacity:    " << (capacity / 1024) << " kb" << endl
       << "Sides:       " << sideList.size() << endl
       << "Density:     " << (drive->GetDensity() ? "double" : "single") << endl
       << "Tracks:      " << side0Tracks.size() << endl
       << "Sectors:     " << side0Track0Sectors.size() << endl
       << "Sector size: ";
  {
    string first;     
    for (auto sectorSize : sectorSizes) {
      cout << first << sectorSize << endl;
      first = ", ";
    }
    cout << endl;
  }

  int maxSectorSize = *sectorSizes.rbegin();
  int chunkCount = (maxSectorSize + 127) / 128;

  ColumnFormatter::Columns output(1 + 1 + (sectorNums.size() * sectorNums.size()) + (sideList.size() - 1));

  // track number : 2 or 3 rows depending on chunk count
  //output[0].push_back("   ");    
  //if (chunkCount < 2) {
  //  output[0].push_back("   ");
  //}

  int col = 0;
  for (auto sideNum : sideNums) {
    output[col+0].push_back("   ");
    output[col+0].push_back("   ");
    output[col+1].push_back("|");
    output[col+1].push_back("+");
    col += 2;
    for (auto sectorNum : sectorNums) {
      stringstream strm;
      if (chunkCount > 1) {
        strm << setw(chunkCount) << setfill(' ') << sectorNum << '|';
        output[col].push_back(strm.str());
        output[col].push_back(std::string(chunkCount, '=') + "+");
      }
      else {
        if ((sectorNum % 10) != 0)
          output[col].push_back(" ");
        else {
          strm << (sectorNum / 10);
          output[col].push_back(strm.str());
        }
        strm.str("");
        strm << (sectorNum % 10);
        output[col].push_back(strm.str());
        output[col].push_back("+");
      }
      col++;
    }
  }

  std::map<int, std::map<int, std::map<int, std::vector<uint8_t>>>> diskData;
  std::map<int, std::map<int, std::map<int, std::vector<std::string>>>> chunkType;

  // sector information
  for (auto trackNum : trackNums) {
    int col = 0;
    for (auto sideNum : sideNums) {
      stringstream strm;
      if (col == 0) {
        strm << trackNum;
      }
      output[col+0].push_back(strm.str());
      output[col+1].push_back("|");
      col += 2;
      auto & sideData = diskData[sideNum];
      for (auto sectorNum : sectorNums) {
        std::stringstream strm;
        const VirtualDrive::SectorInfo * sector = drive->GetInfo(sideNum, trackNum, sectorNum);
        if (sector == nullptr) {
          if (chunkCount > 1) {
            for (int chunk = 0; chunk < chunkCount; ++chunk) {
              strm << NO_CHUNK;
            }
            strm << "|";
          }
        }
        else {
          auto & sectorData = sideData[trackNum][sectorNum];
          sectorData.resize(sector->m_size);
          VirtualDrive::SectorInfo sectorInfo;
          int len = drive->ReadSector(sideNum, trackNum, sectorNum, sectorInfo, &sectorData[0], sectorData.size());
          for (int chunk = 0; chunk < chunkCount; ++chunk) {
            std::string id = IdentifyChunk(&sectorData[chunk*CHUNK_SIZE], CHUNK_SIZE);
            chunkType[sideNum][trackNum][sectorNum].push_back(id);
            strm << id;
          }
        }
        output[col++].push_back(strm.str());
      }
    }
  }

  if (displayMap)
    cout << ColumnFormatter::Print(output, 0);

  if (displayData || displayDir) {

    for (auto sideNum : sideNums) {
      for (auto trackNum : trackNums) {
        for (auto sectorNum : sectorNums) {
          int len = diskData[sideNum][trackNum][sectorNum].size();
          if (len > 0) {
            stringstream hdr;
            hdr <<  "Side " << sideNum << ", track " << trackNum << ", sector " << sectorNum;
            bool isDir = false;
            for (int chunk = 0; !isDir && (chunk < chunkCount); ++chunk) {
              isDir = chunkType[sideNum][trackNum][sectorNum][chunk] == DIR_CHUNK;
            }
            if (displayData || (displayDir && isDir)) {
              cout << hdr.str()/* << ", offset " << sectorInfo.m_offset */ << endl;
              cout << DumpMemory(&diskData[sideNum][trackNum][sectorNum][0], len);
              cout << endl;
            }
          }  
        }
      }
    }
  }     

  return true;
}

int main(int argc, char *argv[])
{
  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);
  if ((argc - opt) < 1) {
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

  if (args.HasArg("-h")) {
    cout << args.Usage();
    return 0;
  }
 
  std::string fn(argv[opt]);
  VirtualFileIdentifier fileId;

  if (args.HasArg("-v"))
    fileId.SetVerbose(true);

  std::shared_ptr<VirtualDrive> drive = fileId.Open(fn, true);
  if (drive == nullptr) {
    cerr << "error: " << fileId.GetError() << endl;
    return -1;
  }

  bool displayMap = args.HasArg("-m");
  bool displayData = args.HasArg("-d");
  bool displayDir = args.HasArg("-D");

  if (!DisplayInfo(drive, displayMap, displayData, displayDir)) {
    return -1;
  }

  CPMFileSystem cpmfs(drive);

  cpmfs.Open();

  return 0;
}