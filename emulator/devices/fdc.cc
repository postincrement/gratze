
#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "src/emulator.h"
#include "common/misc.h"
#include "devices/fdc.h"

using namespace std;

#define   DISK_SPEED_5_INCH_RPM     300.0

#define   INIT_TIME_MS      200

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
#define   COMMAND_HEAD_LOAD_I       (1 << 3)      // type I
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

  { 0xf4, 0xc4, "readAddr",   3, 0 },
  { 0xfe, 0xe4, "readTrack",  3, 0 },
  { 0xff, 0xf4, "writeTrack", 3, &WD_FDC::WriteTrackCommand },

  { 0xf0, 0xd0, "forceInt",   4, &WD_FDC::ForceIntCommand },

  { 0xff, 0xfe, "enable1771", 0, &WD_FDC::PercomCommand },
  { 0xff, 0xff, "enable1791", 0, &WD_FDC::PercomCommand }
};

///////////////////////////////////////////////////////////

WD_FDC::WD_FDC(unsigned int mode, bool debug)
  : m_mode(mode)
  , m_debug(debug)
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
  m_sector      = 0;
  m_side        = 0;
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

void WD_FDC::SetInterruptHandler(std::function<void (bool)> handler)
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

bool WD_FDC::SelectSide(int side)
{
  cerr << "FDC: select side " << side << endl;
  m_side = side;
  return true;
}

bool WD_FDC::SelectDrive(int driveNum)
{
  int oldDrive = driveNum;

  bool ret = false;
  if (driveNum >= MAX_DRIVE) {
    cerr << "FDC: Cannot select drive " << dec << driveNum << endl;
    driveNum = -1;
  }

  if (driveNum < 0) {
    if (m_debug)
      cerr << "FDC: SELECT NO DRIVE" << endl;
    m_drive = -1;
    ret = true;
  }
  else if (driveNum != m_drive) {
    if (m_debug)
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

bool WD_FDC::GetDRQ() const
{
  return (m_status & STATUS_DRQ) != 0;
}

bool WD_FDC::GetInterrupt() const
{
  return m_interrupt;
}

/////////////////////////////////////////////////////////////////////////////////////

WD_FDC::CommandInfo * WD_FDC::GetCommand(uint8_t cmd, WD_FDC::CommandInfo * info, size_t count)
{
  for (int i = 0; i < count; ++i) {
    if ((cmd & info->m_andMask) == (info->m_cmd))
      return info;
    info++;
 }

 return nullptr;
}

WD_FDC::CommandInfo * WD_FDC::GetCommand(uint8_t cmd)
{
  return GetCommand(cmd, g_commands, sizeof(g_commands)/sizeof(g_commands[0]));
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

  if (m_debug)
    cerr << "FDC: command " << HEXFORMAT0x2(command) << " " << info->m_name << " is type " << (int)info->m_type << endl;
  m_pulseIndex = false;

  switch (info->m_type) {

    case 0:  // Undocumented commands
      m_setInterrupt   = false;
      m_currentCommand = -1;
      std::invoke(info->m_function, this, command);
      break;

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
      m_setInterrupt   = false;
      m_status         = STATUS_BUSY;
      m_currentCommand = command;
      std::invoke(info->m_function, this, command);
      break;

    case 4:  // TYPE IV - 
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
  if (m_pulseIndex)
    m_status ^= STATUS_INDEX;
  //cerr << "FDC: read status " << HEXFORMAT0x2(m_status) << endl;
  return m_status;
}

uint8_t WD_FDC::ReadDataReg()
{
  if (!m_reading)
    return 0;

  // get data
  m_data = m_buffer[m_bufferPtr];
  cerr << "FDC: reading byte " << dec << (int)m_bufferPtr << " of " << (int)m_bufferLen << endl;
  if (++m_bufferPtr < m_bufferLen) {
    m_status |= STATUS_DRQ;      
  }
  else {
    int maxSector = m_drives[m_drive]->GetSectors();
    if (
        ((m_currentCommand & COMMAND_MULT_RECS) == 0)
        || (maxSector == m_sector)
       ) {
      m_currentCommand = -1;
      m_status = m_readDAM;  // resets STATUS_BUSY
      m_reading = false;
      UpdateInterrupt(true);
      if (m_debug)
        cerr << "FDC: read ended" << endl;
    }
    else {
      m_sector++;
      ReadCommand(m_currentCommand);
      m_status |= STATUS_DRQ;
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
  if (m_interruptHandler) {
    m_interruptHandler(interruptOn);
  }
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
  return 0;
}

int WD_FDC::StepOutCommand(uint8_t cmd)
{
  m_directionIn = false;
  SeekTrack(cmd, m_track-1, cmd & COMMAND_UPDATE);
  return 0;
}

int WD_FDC::StepCommand(uint8_t cmd)
{
  int newTrack = m_track + (m_directionIn ? 1 : -1);
  return SeekTrack(cmd, newTrack, cmd & COMMAND_UPDATE);
}

int WD_FDC::SeekTrack(uint8_t cmd, uint8_t track, bool update)
{
  if (m_debug)
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
  //m_pulseIndex = true;
  //m_status |= ((m_realTrack == 0) ? STATUS_TRK0 : 0);
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
    m_bufferPtr = 0;
    int bufferLen = m_drives[m_drive]->ReadSector(m_realTrack, m_side, m_sector, info, m_buffer, MAX_SECTOR_SIZE);
    if ((bufferLen <= 0)) { // || (info.m_density != m_density)) {
      if (m_debug)
        cerr << "FDC: read sector, track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << " failed" << endl;
      m_status = STATUS_RECORDNOTFOUND;  // resets STATUS_BUSY
      m_setInterrupt = true;
    }
    else {
      //if (m_debug)
        cerr << "FDC: read sector, track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << ",len=" << (int)bufferLen << ",density=" << (int)info.m_density << ",DAM=" << HEXFORMAT0x2(info.m_dam) << endl;
      m_bufferLen = bufferLen;
      m_reading   = true;
      m_status |= STATUS_DRQ;
      uint8_t dam = info.m_dam;
      //if (dam == 0xfa)
      //  dam = 0xf8;
      // TODO: DAM translation here
      switch (dam) {
        case 0xf8:
          m_readDAM = 0x60;
          break;
        case 0xf9:
          m_readDAM = 0x40;
          break;
        case 0xfa:
          m_readDAM = 0x20;
          break;
        case 0xfb:
          m_readDAM = 0x00;
          break;
      }
      if (m_debug)
        cerr << "FDC: DAM mask = " << HEXFORMAT0x4(info.m_dam) << " => " << HEXFORMAT0x2(m_readDAM) << endl;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////
//
//  TYPE III commands
//

int WD_FDC::WriteTrackCommand(uint8_t cmd)
{
  LoadHead(cmd & COMMAND_HEAD_LOAD_I);

  if (!IsCurrentDriveAvailable()) {
    m_status = STATUS_SEEKERR;
    m_setInterrupt = true;
  }
  else {
    if (m_debug)
      cerr << "FDC: setting write protected" << endl;
    m_status = STATUS_WR_PROT; // STATUS_LOST_DATA;  
    m_setInterrupt = true;
  }
  return 0;
}

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
    if (m_currentCommand < 0) {
      if (m_debug)
        cerr << "FDC: force int on busy with no command" << endl;
    }
    else {
    if (m_debug)
        cerr << "FDC: force int on busy with command " << HEXFORMAT0x2(m_currentCommand) << endl;
    }
    if (cmd & (COMMAND_FORCE_INT_NR2R | COMMAND_FORCE_INT_INDEX))
      UpdateInterrupt(true);
  }
  else {
    SetTypeIStatus();
    if (m_debug)
      cerr << "FDC: force int not busy with no command " << HEXFORMAT0x2(m_status) << endl;
  }
  return 0;
}

////////////////////////////////////////////////////////////////
//
//  Undocumented commands
//

int WD_FDC::PercomCommand(uint8_t cmd)
{
  if (m_debug)
    cerr << "FDC: Percom select " << ((cmd == 0xfe) ? "1771" : "1791") << endl;
  return 0;
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
  if (!title.empty()) {
    if (m_debug)
      cerr << "FDC SET " << title << ": " << HEXFORMAT0x2(value) << endl;
  }
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

int WD_FDC::ReadAddrCommand_1793(uint8_t cmd)
{
  Emulator::GetInstance()->DisplayTraceInfo();
  LoadHead(cmd & COMMAND_HEAD_LOAD_II);

  if (!IsCurrentDriveAvailable()) {
    m_status = STATUS_SEEKERR;
    m_setInterrupt = true;
  }
  else {
    //if (m_debug)
      cerr << "FDC: read addr, track=" << dec << (int)m_track << endl;
    m_bufferPtr = 0;
    m_bufferLen = 4;
    m_reading   = true;

    Emulator::GetInstance()->DisplayTraceInfo();

    m_buffer[0] = m_track;
    m_buffer[1] = m_side;
    m_buffer[2] = 1;
    m_buffer[3] = 2;

    m_reading   = true;
    m_status |= STATUS_DRQ;
  }
  return 0;
}

static WD_FDC::CommandInfo g_2793commands[] = {
  { 0xfb, 0xc0,  "readaddr",  3, &WD_FDC::ReadAddrCommand_1793 }
};

WD_FDC::CommandInfo * WD_FD1793::GetCommand(uint8_t cmd)
{
  WD_FDC::CommandInfo * info = WD_FDC::GetCommand(cmd, g_2793commands, sizeof(g_2793commands)/sizeof(g_2793commands[0]));
  if (info != nullptr)
    return info;
  return WD_FDC::GetCommand(cmd);  
}

