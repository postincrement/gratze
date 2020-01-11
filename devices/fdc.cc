
#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "src/misc.h"
#include "devices/fdc.h"

using namespace std;

#define   DISK_SPEED_5_INCH_RPM     300.0

#define   INIT_TIME_MS      200

#define   JV3_SECTOR_COUNT  (2901 / 3)

#define   SD_SECTOR_SIZE    256
#define   SD_SECTOR_COUNT   10   

// status for multiple types
#define   STATUS_BUSY               (1 << 0)    // type I and II
#define   STATUS_WR_PROT            (1 << 6)    // type I and type II write
#define   STATUS_NOT_READY          (1 << 7)    // type II and III

// status for type I
#define   STATUS_INDEX              (1 << 1)    // type I
#define   STATUS_TRK0               (1 << 2)    // type I
#define   STATUS_SEEKERR            (1 << 4)    // type I
#define   STATUS_HEADON             (1 << 5)    // type I

// status for type II and III
#define   STATUS_DRQ                (1 << 1)    // type II
#define   STATUS_LOST_DATA          (1 << 2)    // type II
#define   STATUS_CRCERR             (1 << 3)    // type II
#define   STATUS_RECORDNOTFOUND     (1 << 4)    // type II
#define   STATUS_REC_TYPE           (1 << 5)    // type II write
#define   STATUS_DAM_MASK           (STATUS_REC_TYPE | (STATUS_REC_TYPE << 1))
#define   STATUS_WR_FAULT           (1 << 5)    // type II write
#define   STATUS_WR_PROT            (1 << 6)    // type II

// comand bits for type I
#define   COMMAND_VERIFY            (1 << 2)      // type I
#define   COMMAND_HEAD_LOAD_I         (1 << 3)      // type I
#define   COMMAND_UPDATE            (1 << 4)      // type I  

// comand bits for type II
#define   COMMAND_DAM               (1 << 0)      // type II
#define   COMMAND_HEAD_LOAD_II      (1 << 2)      // type II
#define   COMMAND_BLOCK_LEN         (1 << 3)      // type II
#define   COMMAND_MULT_RECS         (1 << 4)      // type II

#define   COMMAND_FORCE_INT_NR2R    (1 << 0)
#define   COMMAND_FORCE_INT_R2NR    (1 << 1)
#define   COMMAND_FORCE_INT_INDEX   (1 << 2)
#define   COMMAND_FORCE_INT_IMMED   (1 << 3)


#define  MAX_DRIVE      4


static WD_FDC::CommandInfo g_commands[] = {
  { 0xf0, 0x00, "home",       1, &WD_FDC::HomeCommand },
  { 0xf0, 0x10, "seek",       1, &WD_FDC::SeekCommand },
  { 0xf0, 0x20, "step",       1, &WD_FDC::StepCommand },
  { 0xe0, 0x30, "stepIn",     1, &WD_FDC::StepInCommand },
  { 0xe0, 0x60, "stepOut",    1, &WD_FDC::StepOutCommand },

  { 0xe3, 0x80, "read",       2, &WD_FDC::ReadCommand },
  { 0xe0, 0xa0, "write",      2, 0 },

  { 0xff, 0xc0, "readAddr",   3, 0 },
  { 0xfe, 0xe4, "readTrack",  3, 0 },
  { 0xff, 0xf4, "writeTrack", 3, 0 },

  { 0xf0, 0xd0, "forceInt",   4, &WD_FDC::ForceIntCommand }
};

///////////////////////////////////////////////////////////

WD_FDC::WD_FDC()
{
  m_diskRevTime_ms = (1000.0 / DISK_SPEED_5_INCH_RPM);
  Reset();
}

void WD_FDC::Reset()
{
  m_state       = 0;
  m_drive       = -1;
  m_headLoaded  = false;
  m_directionIn = true;
  m_realTrack   = 0;

  m_status      = STATUS_BUSY;  // IMPORTANT: without this, the L2 ROM won't detect the FDC
  m_sector      = 1;
  m_track       = m_realTrack;
  m_data        = 0;

  m_bufferPtr   = 0;
  m_bufferLen   = 0;

  m_setInterrupt = false;
  m_interrupt    = false;

  m_reading      = false;
  m_writing      = false;

  m_currentCommand = -1;

  if (m_driveChangedHandler)
    m_driveChangedHandler(m_drive, m_headLoaded);
}

void WD_FDC::SetInterruptHandler(std::function<void ()> handler)
{
  m_interruptHandler = handler;
}

void WD_FDC::SetDriveChangedHandler(std::function<void (int, bool)> handler)
{
  m_driveChangedHandler = handler;
}

/////////////////////////////////////////////////////////////////////////////////////

bool WD_FDC::MountDrive(int driveNum, VirtualDrive * drive, bool readOnly)
{
  if ((driveNum < 0) || (driveNum >= MAX_DRIVE))
    return false;

  if (driveNum >= m_drives.size()) {
    m_drives.resize(driveNum+1);
  }

  m_drives[driveNum].reset(drive);
  return m_drives[driveNum]->Mount(readOnly);
}

bool WD_FDC::SelectDrive(int driveNum)
{
  int oldDrive = driveNum;

  bool ret = false;
  if (driveNum >= MAX_DRIVE) {
    cerr << "FDC: Canonot selected drive " << dec << driveNum << endl;
    driveNum = -1;
  }

  if (driveNum < 0) {
    cerr << "FDC: SELECT NO DRIVE" << endl;
    m_drive = -1;
    ret = true;
  }
  else if (driveNum != m_drive) {
    cerr << "FDC: SELECT DRIVE " << dec << driveNum << endl;
    m_drive = driveNum;
  }

  if (m_driveChangedHandler && (m_drive != oldDrive))
    m_driveChangedHandler(m_drive, m_headLoaded);

  return true;
}

bool WD_FDC::IsCurrentDriveAvailable() const
{
  return (m_drive >= 0) && (m_drive < m_drives.size()) && m_drives[m_drive];
}

/////////////////////////////////////////////////////////////////////////////////////

#define HASH_STS(side, track, sector)     (sector + (side << 8) + (track << 16))

WD_FDC::CommandInfo * WD_FDC::GetCommand(uint8_t cmd)
{
  WD_FDC::CommandInfo * info = g_commands;
  for (int i = 0; i < (sizeof(g_commands)/sizeof(g_commands[0])); ++i) {
    if ((cmd & info->m_andMask) == (info->m_cmd))
      return info;
    info++;
 }

 return nullptr;   
}


void WD_FDC::Run()
{
  auto now = std::chrono::system_clock::now();

  switch (m_state) {
    case 0:  // idle state
      break;

    case 1:  // type 1
      break;  
  }
}

void WD_FDC::WriteCmdReg(int8_t command)
{
  auto now = std::chrono::system_clock::now();

  UpdateInterrupt(false);

  CommandInfo * info = GetCommand(command);
  if (info == nullptr) {
    cerr << "FDC: unknown command " << HEXFORMAT0x2(command) << endl;
    return;
  }

  if (!info->m_function) {
    cerr << "FDC: " << info->m_name << " command not implemented" << endl;
    return;
  }

  cerr << "FDC: command " << HEXFORMAT0x2(command) << " " << info->m_name << " is type " << (int)info->m_type << endl;

  switch (info->m_type) {

    case 1:  // TYPE I
      m_setInterrupt   = false;
      // all type I commands finish immediately - no BUSY required
      //m_status         = STATUS_BUSY;
      m_currentCommand = -1;
      std::invoke(info->m_function, this, command);
      break;

    case 2:  // TYPE II
      m_setInterrupt   = false;
      m_status         = STATUS_BUSY;
      m_currentCommand = command;
      std::invoke(info->m_function, this, command);
      break;

    case 3:  // TYPE III
      cerr << "FDC: " << info->m_name << " command not supported" << endl;
      break;

    case 4:  // TYPE IV
      std::invoke(info->m_function, this, command);
      break;

    default:
      cerr << "FDC: " << info->m_name << " command has unknown type " << dec << info->m_type << endl;
      break;
  }

  if (m_setInterrupt) {
    UpdateInterrupt(true);
    m_setInterrupt = false;
  }
}

uint8_t WD_FDC::ReadStatusReg()
{
  UpdateInterrupt(false);
  return m_status;
}

uint8_t WD_FDC::ReadDataReg()
{
  if (!m_reading)
    return 0;

  // if more data, reset DRQ
  for (;;) {
    m_data = m_buffer[m_bufferPtr++];
    if (m_bufferPtr < m_bufferLen) {
      //cerr << "FDC: reading byte " << dec << (int)m_bufferPtr << " of " << (int)m_bufferLen << endl;
      m_status |= STATUS_DRQ;
      break;
    } 
    else if (m_currentCommand & COMMAND_MULT_RECS) {
      m_sector++;
      cerr << "FDC: read multiple moving to sector " << dec << (int)m_sector << endl;
      ReadCommand(m_currentCommand);
    }
    else {
      cerr << "FDC: read ended" << endl;
      m_currentCommand = -1;
      m_status &= m_statusMask;  // resets STATUS_BUSY
      m_reading = false;
      UpdateInterrupt(true);
      break;
    }
  }

  return m_data;
}

void WD_FDC::RestartHeadLoadTimer()
{
  m_headLoadtimer = std::chrono::system_clock::now() + std::chrono::milliseconds((int)(2 * m_diskRevTime_ms));
}

void WD_FDC::UpdateInterrupt(bool interruptOn)
{
  if (interruptOn == m_interrupt)
    return;

  m_interrupt = interruptOn;
  if (interruptOn && m_interruptHandler)
    m_interruptHandler();
}

void WD_FDC::LoadHead(bool loadHead)
{
  bool oldHeadLoaded = m_headLoaded;
  m_headLoaded = loadHead;
  if (m_driveChangedHandler && (oldHeadLoaded != loadHead))
    m_driveChangedHandler(m_drive, m_headLoaded);
  RestartHeadLoadTimer();
}

////////////////////////////////////////////////////////////////
//
//  TYPE I commands
//

int WD_FDC::HomeCommand(uint8_t cmd)
{
  return SeekTrack(cmd, 0, false);
}

int WD_FDC::SeekCommand(uint8_t cmd)
{
  return SeekTrack(cmd, m_data, false);
}

int WD_FDC::StepInCommand(uint8_t cmd)
{
  m_directionIn = true; 
  SeekTrack(cmd, m_track-1, cmd & COMMAND_UPDATE);
}

int WD_FDC::StepOutCommand(uint8_t cmd)
{
  m_directionIn = false; 
  SeekTrack(cmd, m_track-1, cmd & COMMAND_UPDATE);
}

int WD_FDC::StepCommand(uint8_t cmd)
{
  int newTrack = m_track + (m_directionIn ? 1 : -1);
  return SeekTrack(cmd, newTrack, cmd & COMMAND_UPDATE);
}

int WD_FDC::SeekTrack(uint8_t cmd, uint8_t track, bool update)
{
  cerr << "FDC: seek to track " << dec << (int)track << endl;

  LoadHead(cmd & COMMAND_HEAD_LOAD_I);

  // update track and do update bit logic
  m_realTrack = track;
  if (update) {
    m_track = m_realTrack;
  }

  // verify logic
  if (!(cmd & COMMAND_VERIFY)) {
    m_status = 0;
  }
  else {
    // "no disk"            STATUS_NOT_READY
    // "Bad CRC"            CRC_ERROR
    // "CRC, wrong track"   SEEK_ERROR
    // "valid"              no error
    // 
    if (IsCurrentDriveAvailable()) {
      m_status = STATUS_SEEKERR;
    }
    else if (m_track != m_realTrack) {
      m_status = STATUS_SEEKERR;
    }
    else {
      m_status = 0;
    }
  }

  SetTypeIStatus();

  m_setInterrupt = true;
  return 0;
}

void WD_FDC::SetTypeIStatus()
{
  // set track 0 bit
  m_status |= ((m_realTrack == 0) ? STATUS_TRK0 : 0);
}


////////////////////////////////////////////////////////////////
//
//  TYPE II commands
//

int WD_FDC::ReadCommand(uint8_t cmd)
{
  LoadHead(cmd & COMMAND_HEAD_LOAD_II);

  if (!IsCurrentDriveAvailable()) {
    m_status = STATUS_SEEKERR;
    m_setInterrupt = true;
  }
  else {
    VirtualDrive::SectorInfo info;
    int bufferLen = m_drives[m_drive]->ReadSector(m_realTrack, m_sector, info, m_buffer, MAX_SECTOR_SIZE);
    if ((bufferLen <= 0)) { // || (info.m_density != m_density)) {
      cerr << "FDC: read track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << " failed" << endl;
      m_status = STATUS_RECORDNOTFOUND;  // resets STATUS_BUSY
      m_setInterrupt = true;
    }
    else {
      cerr << "FDC: read track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << ",len=" << (int)bufferLen << ",density=" << (int)info.m_density << ",DAM=" << HEXFORMAT0x2(info.m_dam) << endl;
      m_bufferPtr = 0;
      m_bufferLen = bufferLen;
      m_reading   = true;
      /*
      {
        int i;
        for (i = 0; i < bufferLen; ++i) {
          if ((i % 16) == 0)
            cout << HEXFORMAT0x4(i) << "  ";
          cout << ' ' << HEXFORMAT2(m_buffer[i]);
          if ((i % 16) == 15)
            cout << endl;
        }
        if ((i % 16) != 15)
          cout << endl;
      }
      */
      m_status |= STATUS_DRQ;
      switch (info.m_dam) {
        case 0xf8:
          m_status |= 0x60;
          break; 
        case 0xf9:
          m_status |= 0x40;
          break; 
        case 0xfa:
          m_status |= 0x20;
          break; 
        case 0xfb:
          m_status |= 0x00;
          break; 
      }
      m_statusMask = STATUS_DAM_MASK;   // reset STATUS_BUSY
    }
  }
}


////////////////////////////////////////////////////////////////
//
//  TYPE III commands
//

////////////////////////////////////////////////////////////////
//
//  TYPE IV commands
//

int WD_FDC::ForceIntCommand(uint8_t cmd)
{
  if (m_status & STATUS_BUSY) {
    m_bufferLen = 0;
    m_bufferPtr = 0;
    m_reading = false;
    m_status &= !STATUS_BUSY;
    if (m_currentCommand < 0)
      cerr << "FDC: force int on busy with no command" << endl;
    else { 
      cerr << "FDC: force int on busy with command " << HEXFORMAT0x2(m_currentCommand) << endl;
    }
    if (cmd & COMMAND_FORCE_INT_NR2R)
      UpdateInterrupt(true);
  }
  else {
    cerr << "FDC: force int not busy with no command" << endl;
    SetTypeIStatus();
  }
}


////////////////////////////////////////////////////////////////
//
//  
//

void WD_FDC::Write(uint16_t addr, uint8_t value)
{
  //m_noPrint = false;
  std::string title;
  switch (addr & 0x3) {
    case 0:
      WriteCmdReg(value);
      //title = "CMD";
      break;
    case 1:
      //title = "TRK";
      m_track = value;
      break;
    case 2:
      //title = "SECT";
      m_sector = value;
      break;
    case 3:
      //title = "DATA";
      m_data = value;
      break;
  }
  if (!title.empty())
    cerr << "FDC SET " << title << ": " << HEXFORMAT0x2(value) << endl;
}

uint8_t WD_FDC::Read(uint16_t addr)
{
  uint8_t value = 0;
  switch (addr & 0x3) {
    case 0:
      value = ReadStatusReg();
      break;
    case 1:
      value = m_track;
      break;
    case 2:
      value = m_sector;
      break;
    case 3:
      value = ReadDataReg(); 
      break;
  }

  return value;
}

/////////////////////////////////////////////////////////////

WD_FD1771::WD_FD1771()
{
}

/////////////////////////////////////////////////////////////

VirtualDrive::VirtualDrive()
{}

VirtualDrive::~VirtualDrive()
{}

std::string VirtualDrive::GetName() const
{
  return m_name;
}

bool VirtualDrive::IsReadOnly() const
{
  return m_readOnly;
}

/////////////////////////////////////////////////////////////

static const char * g_formatNames[(int)VirtualDriveFile::Format::eCount] = {
  "Unknown",
  "JV1",
  "JV3",
  "DMK"
};

VirtualDriveFile::VirtualDriveFile()
  : m_fd(-1)
{}

VirtualDriveFile::~VirtualDriveFile()
{
  if (m_fd >= 0)
    ::close(m_fd);
}

bool VirtualDriveFile::Open(const std::string & name, bool readOnly)
{
  m_name       = name;
  m_readOnly   = readOnly;
  m_format     = Format::eUnknown;

  // see if file exists
  if (::access(name.c_str(), 0) != 0) {
    cerr << "error: cannot find file '" << name << "'" << endl;
    return false;
  }

  // see if underlying file is read-only
  if (!m_readOnly) {
    m_readOnly = ::access(name.c_str(), W_OK) != 0;
    cerr << "info: file '" << name << "' is read-only" << endl;
  }

  //if (m_readOnly)
    m_fd = ::open(name.c_str(), O_RDONLY | O_BINARY);
  //else  
  //  m_fd = ::open(name.c_str(), O_RDWR);

  if (m_fd < 0) {
    cerr << "error: cannot open file '" << name << "' - " << strerror(errno) << endl;
    return false;
  }

  // get size of file
  off_t len = lseek(m_fd, 0, SEEK_END);
  lseek(m_fd, 0, SEEK_SET);

  // identify the file format
  size_t pos = name.rfind('.');
  if (pos != std::string::npos) {
    std::string extension(name.substr(pos+1));
    for (auto & r : extension) r = tolower(r);
    if (extension == "jv1")
      m_format     = Format::eJV1;
    else if (extension == "jv3")  
      m_format     = Format::eJV3;
    else if (extension == "dmk")  
      m_format     = Format::eDMK;
  }
  if (m_format != Format::eUnknown) {
    cerr << "info: file '" << name << "' set to format '" << g_formatNames[(int)m_format] << "' using file extension" << endl;
  }
  else {
    if ((len % (SD_SECTOR_SIZE * SD_SECTOR_COUNT)) == 0)
      m_format     = Format::eJV1;
    else  
      m_format     = Format::eJV3;
    if (m_format != Format::eUnknown) {
      cerr << "info: file '" << name << "' set to format '" << g_formatNames[(int)m_format] << "' using inspection" << endl;
    }
  }

  lseek(m_fd, 0, SEEK_SET);

  std::stringstream formatError;
  switch (m_format) {
    case Format::eJV1:
      ReadJV1(len, formatError);
      break;
    case Format::eJV3:
      ReadJV3(len, formatError);
      break;
    case Format::eDMK:
      ReadDMK(len, formatError);
      break;
    case Format::eUnknown:
      cerr << "error: cannot identify format of file '" << name << "'" << endl;
      return false;
  }

  if (formatError.str().length() > 0) {
    cerr << "error: file '" << name << "' is not valid for " << g_formatNames[(int)m_format] << " - " << formatError.str() << endl;
    return false;
  }

  cerr << "info: file '" << name << "' is len " << len << " bytes = " << (int)m_trackCount << " tracks" << endl;

  return true;
}

void VirtualDriveFile::ReadJV1(off_t len, std::stringstream & formatError)
{
  m_trackCount    = (len / (SD_SECTOR_COUNT * SD_SECTOR_SIZE));

  if (len != (SD_SECTOR_COUNT * SD_SECTOR_SIZE * m_trackCount)) {
    formatError << "file length " << len << " is not compatible with tracks of " << SD_SECTOR_COUNT << " x " << SD_SECTOR_SIZE << " bytes" << endl;
    return;
  }

  off_t offs = 0;
  for (int track = 0; track < m_trackCount; ++track) {
    for (int sector = 1; sector <= SD_SECTOR_COUNT; ++sector) {
      m_sectorMap.emplace(sector + (track << 16), SectorInfo(offs, SD_SECTOR_SIZE, (track == 17) ? 0xfa : 0xfb, 0));
      offs += SD_SECTOR_SIZE;
    }
  }
}

void VirtualDriveFile::ReadJV3(off_t len, std::stringstream & formatError)
{
  uint8_t header[JV3_SECTOR_COUNT*3];
  int ret = ::read(m_fd, header, sizeof(header));
  if (ret != sizeof(header)) {
    formatError << "unable to read header - "<< ret << " != " << sizeof(header);
    return;
  }

  m_trackCount = 0;
  uint8_t * ptr = header;
  off_t offs = JV3_SECTOR_COUNT*3 + 1;

  for (int i = 0; i < JV3_SECTOR_COUNT; ++i) {
    uint8_t track  = ptr[0];
    uint8_t sector = ptr[1];
    uint8_t flags  = ptr[2];

    int sectorSize;

    if ((track == 0xff) && (sector == 0xff)) {
      switch (flags & 0x3) {
        case 0:
          sectorSize = 512;
          break;
        case 1:
          sectorSize = 1024;
          break;
        case 2:
          sectorSize = 128;
          break;
        case 3:
          sectorSize = 256;
          break;
      }
    }
    else {
      int density = (flags & 0x80) ? 1 : 0;
      uint8_t dam = 0x00;
      int side    = (flags & 0x10) ? 1 : 0;
      switch (flags & 0x3) {
        case 0:
          sectorSize = 256;
          break;
        case 1:
          sectorSize = 128;
          break;
        case 2:
          sectorSize = 1024;
          break;
        case 3:
          sectorSize = 512;
          break;
      }
      switch ((flags & 0x60) + density) {
        case 0x00:  // single
        case 0x01:  // double
          dam = 0xfb;
          break;
        case 0x20:  // single
          dam = 0xfa;
          break;
        case 0x21:  // double
          dam = 0xf8;
          break;
        case 0x40:  // single
          dam = 0xf9;
          break;
        case 0x60:  // double
          dam = 0xf8;
          break;
      }
      if (dam == 0x00) {
        cerr << "error: unknown DAM code " << HEXFORMAT0x2(dam) << endl;
      }
      else {
        m_trackCount = std::max(m_trackCount, (int)track);
        m_sectorMap.emplace(HASH_STS(0, track, sector), SectorInfo(offs, sectorSize, dam, density));
      }
    }

    offs += sectorSize;
    ptr += 3;
  }
}

void VirtualDriveFile::ReadDMK(off_t len, std::stringstream & formatError)
{
  formatError << "not supported";
}

bool VirtualDriveFile::Mount(bool readonly)
{
  return true;
}

int VirtualDriveFile::ReadSector(int track, int sector, SectorInfo & info, uint8_t * data, int len)
{
  int side = 0;
  auto r = m_sectorMap.find(HASH_STS(side, track, sector+1));
  if (r == m_sectorMap.end()) {
    cerr << "error: request for unknown sector " << dec << sector << " and track " << track << endl;
    return -1;
  }

  info = r->second;  

  cout << "FDC: seek side " << side << ",track " << (int)track << ",sector " << sector << " = offset " << info.m_offset << " (" << HEXFORMAT0x4(info.m_offset) << ")" << endl;

  if (lseek(m_fd, info.m_offset, SEEK_SET) < 0) {
    cerr << "error: cannot seek for sector " << dec << sector << " and track " << track << endl;
    return -1;
  }

  if (len > info.m_size) {
    len = info.m_size;
  }

  return ::read(m_fd, data, len);
}

int VirtualDriveFile::WriteSector(int track, int sector, uint8_t * data, int len)
{
  return false;
}
