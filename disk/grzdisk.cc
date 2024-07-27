#include <sstream>
#include <iostream>
#include <fstream>
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
  { 'f', "file",        's',   "disk image to use" },
  { 'h', "help",        ' ',   "display this help message" },
  { 'i', "info",        ' ',   "display info on disk format" },
  { 'v', "verbose",     ' ',   "enable verbose reporting" },
  { 'm', "map",         ' ',   "display sector map" },
  { 'd', "data",        ' ',   "display all data" },
  { 'D', "dumpDir",     ' ',   "display dir sectors" },
  { ' ', "list",        ' ',   "list supported disk types"},
  { 'o', "output",      ' ',   "set output file" },
  { ' ', "text",        ' ',   "force read as text file" },
  { ' ', "binary",      ' ',   "force read as binary file" },

  { 0, 0, 0, 0}
};

static 

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

////////////////////////////////////////////////////////////

bool DisplayInfo(CommandLineArgs & args, std::shared_ptr<VirtualDrive> drive)
{
  bool displayMap = args.HasArg("-m");
  bool displayData = args.HasArg("-d");
  bool displayDir = args.HasArg("-D");

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

/////////////////////////////////////////////////////////////////

std::string FormatSize(unsigned bytes)
{
  stringstream strm;
  if (bytes < 10000)
    strm << bytes << "B";
  else
    strm << FIXEDFORMAT(2, bytes / 1024.0) << "k";
  return strm.str();  
}

std::string ExpandMatchExpression(const std::string & expr)
{
  
  if (expr.length() == 0)
    return "????????.???";

  std::string match;
  size_t dot = expr.find('.');
  size_t end = dot;

  if (end == string::npos)
    end = expr.length();
  if (end > 8)
    end = 8;

  size_t pos = 0;
  while (match.length() < 8) {
    if (pos >= end)
      match += " ";
    else if (expr[pos] != '*')
      match += toupper(expr[pos]);
    else {
      while (match.length() < 8)
        match += '?';
    }
    ++pos;
  }

  match += ".";

  if (dot == string::npos) {
    match += "???";
  }
  else {
    size_t pos = dot + 1;
    while (match.length() < 12) {
      if (pos >= expr.length())
        match += "?";
      else if (expr[pos] != '*')
        match += toupper(expr[pos]);
      else {
        while (match.length() < 12)
          match += '?';
      }
      ++pos;
    }
  }

  return match;
}

bool Match(const std::string & filename, const std::string & expr)
{
  if (expr.empty())
    return true;

  if ((filename.length() != 12) || (expr.length() != 12)) {
    cerr << "internal error: wildcard match args not correct length" << endl;
    exit(-1);
  }

  for (size_t i = 0; i < 12; ++i) {
    if ((expr[i] != '?') && (expr[i] != filename[i])) {
      return false;
    }
  }

  return true;
}

void Dir(const std::map<std::string, CPMFileSystem::FileInfo> & m_fileList, const std::string & matchExpr = "")
{
  unsigned totalSize = 0;
  unsigned totalFiles = 0;
  for (auto & r : m_fileList) {
    const CPMFileSystem::FileInfo & file = r.second;
    if (Match(r.first, matchExpr)) {
      cout << setw(2) << (int)file.m_user << ":" << file.m_name 
          << "   " << setw(8) << FormatSize(file.m_size)
          << "   " << setw(3) << file.m_extents.size() 
          << "   " << (file.m_readonly ? "r" : "w") << (file.m_hidden ? "s" : "-")
          << endl;
      totalFiles += 1;
      totalSize  += file.m_size;
    }
  }
  if (totalFiles == 0)
    cout << "no files found" << endl;
  else 
    cout << "\n" << totalFiles << " file" << ((totalFiles > 1) ? "s" : "") << ", " << FormatSize(totalSize) << endl;
}

std::string GetAsText(const std::vector<uint8_t> & data)
{
  std::stringstream strm; 
  for (auto & v : data) {

    // ^Z is EOF  
    if (v == 0x1a) 
      break;

    char c = v & 0x7f;  

    // output data
    strm.write(&c, 1);
  }
  return strm.str();
}

int CopyAsText(const std::string & filename, const std::vector<uint8_t> & data)
{
  cerr << "writing to '" << filename << "'" << endl;
  std::string str = GetAsText(data);
  ofstream output;
  output.open(filename, ios::trunc);
  output << str;
  cerr << "info: wrote " << str.length() << " bytes to '" << filename << "'" << endl;
  output.close();
  return 0;
}

int CopyAsBinary(const std::string & filename, const std::vector<uint8_t> & data)
{
  ofstream output;
  output.open(filename, ios::binary | ios::trunc);
  output.write((const char *)&data[0], data.size());
  output.close();
  cerr << "info: wrote " << data.size() << " bytes to '" << filename << "'" << endl;
  return 0;
}

/////////////////////////////////////////////////////////////////

void Usage(CommandLineArgs & args)
{
  cerr << "usage: grzdisk [opts] -f inputfile [cmds...]\n"
      << "where opts are:\n"
      << args.Usage();
}

int main(int argc, char *argv[])
{
  CommandLineArgs args;
  int opt = args.Parse(g_options, argc, argv);

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

  std::string fn;
  if (args.HasArg("-h") || !args.GetValue("-f", fn)) {
    Usage(args);
    return 0;
  }

  VirtualFileIdentifier fileId;

  if (args.HasArg("-v"))
    fileId.SetVerbose(true);

  std::shared_ptr<VirtualDrive> drive = fileId.Open(fn, true);
  if (drive == nullptr) {
    cerr << "error: " << fileId.GetError() << endl;
    return -1;
  }

  if (args.HasArg("-i")) {
    return DisplayInfo(args, drive);
  }

  CPMFileSystem cpmfs(drive);

  if (!cpmfs.Open())
    return -1;

  if ((argc - opt) < 1) {
    Dir(cpmfs.m_fileList);
    return 0;
  }

  std::string cmd(argv[opt]);
  std::string arg1(((argc - opt) < 2) ? "" : argv[opt+1]);

  if ((cmd == "dir") || (cmd == "ls")) {
    if (arg1.empty())
      Dir(cpmfs.m_fileList);
    else {
      std::string matchExpr = ExpandMatchExpression(arg1);
      Dir(cpmfs.m_fileList, matchExpr);
    }  
  }

  else if ((cmd == "read") || (cmd == "get") || (cmd == "dump") || (cmd == "cat")) {
    if (arg1.empty()) {
      cout << "usage: grdisk [opts] -f file read filespec" << endl;
      return 0;
    }

    std::string matchExpr = ExpandMatchExpression(arg1);
    std::string filename;
    {
      std::vector<std::string> matches;
      for (auto & r : cpmfs.m_fileList) {
        const CPMFileSystem::FileInfo & file = r.second;
        if (Match(r.first, matchExpr)) {
          matches.push_back(r.first);
        }
      }

      if (matches.size() == 0) {
        cerr << "error: no files found matching '" << matchExpr << "'" << endl;
        return -1;
      }
      if (matches.size() != 1) {
        cerr << "error: more than one file found matching '" << matchExpr << "'\n"
              << "chose one of:\n";
        for (auto & r : matches) {
          cerr << "  " << r << endl;
        }
        return -1;
      }
      filename = matches[0];
    }

    std::vector<uint8_t> data;
    if (!cpmfs.Read(filename, data)) {
      cerr << "error: could not read '" << filename << "'" << endl;
      return -1;
    }
    cerr << "info: read " << data.size() << " bytes from '" << filename << "'" << endl;

    if (cmd == "dump") {
      cout << DumpMemory((const uint8_t *)&data[0], data.size());
      return 0;
    }
    else if (cmd == "cat") {
      cout << GetAsText(data);
      return 0;
    }

    else {
      if (args.HasArg("--text")) {
        cerr << "info: forcing text format for '" << filename << "'" << endl;
        return CopyAsText(arg1, data);
      }
      else if (args.HasArg("--binary")) {
        cerr << "info: forcing binary format for '" << filename << "'" << endl;
        return CopyAsBinary(arg1, data);
      }
      else {
        size_t dot = filename.find('.');
        if (dot == string::npos) {
          cerr << "info: no filename extension - assuming binary" << endl;
          return CopyAsBinary(arg1, data);
        }
        std::string ext = filename.substr(dot+1);
        if (ext == "COM") {
          cerr << "info: " << ext << " file is binary" << endl;
          return CopyAsBinary(arg1, data);
        }
        if ((ext == "TXT") || (ext == "DOC")) {
          cerr << "info: " << ext << " file is text" << endl;
          return CopyAsText(arg1, data);
        }

        cerr << "info: unable to identify file type " << ext << " - assuming binary" << endl;
        return CopyAsBinary(arg1, data);
      }
    }
  }

  else {
    cerr << "error: unknown command '" << cmd << "'" << endl;
    return -1;
  }

  return 0;
}


