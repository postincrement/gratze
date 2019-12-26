#include "fdc.h"

#include <iomanip>
#include <iostream>

using namespace std;

#define   INIT_TIME_MS      200

#define   STATUS_BUSY      (1 << 0)
#define   STATUS_INDEX     (1 << 1)
#define   STATUS_TRK0      (1 << 2)
#define   STATUS_CRCERR    (1 << 3)
#define   STATUS_SEEKERR   (1 << 4)
#define   STATUS_HEADON    (1 << 5)
#define   STATUS_WR_PROT   (1 << 6)
#define   STATUS_NOT_READY (1 << 7)

WD_FDC::WD_FDC()
{
  m_state = 0;
  m_status = 0;
  m_drive = -1;
}

void WD_FDC::SetInterruptHandler(std::function<void ()> handler)
{
  m_interruptHandler = handler;
}

bool WD_FDC::MountDrive(int driveNum, VirtualDrive * drive, bool readOnly)
{
  if (driveNum >= m_drives.size()) {
    m_drives.resize(driveNum+1);
  }

  m_drives[driveNum].reset(drive);

  return m_drives[driveNum]->Mount(readOnly);
}

bool WD_FDC::SelectDrive(int drive)
{
  if ((drive >= m_drives.size()) || (!m_drives[drive])) {
    cerr << "FDC SELECT DRIVE " << drive << " - error : undefined drive " << (int)drive << endl;
    return false;
  }

  cerr << "FDC SELECT DRIVE " << drive << endl;
  m_drive = drive;
}


uint8_t WD_FDC::ReadStatus()
{
  auto now = std::chrono::system_clock::now();

  switch (m_state) {
    // startup
    case 0:
      m_status = STATUS_BUSY;
      m_state = 1;
      m_timer = now + std::chrono::milliseconds(INIT_TIME_MS);
      m_noPrint = true;
      break; 

    // starting (used to fake out the ROM)
    case 1:
      if (now < m_timer) {
        m_noPrint = true;
        return m_status;
      }

      m_status = 0;
      m_state = 2;
      break;  

    // ready
    case 2:
      break;
  }

  return m_status;
}

void WD_FDC::WriteCommand(int8_t command)
{
  auto now = std::chrono::system_clock::now();

  std::string cmdName = "unknown";

  switch (m_state) {

    // startup
    case 0:
      break;

    // starting (used to fake out the ROM)
    case 1:
    // ready
    case 2:
      switch (command & 0xf0) {
        // restore
        case 0x00:
          cmdName = "Restore";
          m_track = 0;
          m_interruptHandler();
          break;

        // seek
        case 0x10:
          cmdName = "Seek";
          break;

        // step
        case 0x20:
        case 0x30:
          cmdName = "Step";
          break;

        // step in
        case 0x40:
        case 0x50:
          cmdName = "Step In";
          break;

        // step out
        case 0x60:
        case 0x70:
          cmdName = "Step Out";
          break;

        // read
        case 0x80:
        case 0x90:
          cmdName = "Read";
          break;

        // write
        case 0xa0:
        case 0xb0:
          cmdName = "Write";
          break;

        // read address
        case 0xc0:
          cmdName = "Read Address";
          break;

        // read track
        case 0xe0:
          cmdName = "Read Track";
          break;

        // write track
        case 0xf0:
          cmdName = "Write Track";
          break;

        // force interrupt
        case 0xd0:
          cmdName = "Force Interrupt";
          break;
      }
      break;
  }

  m_noPrint = true;
  cerr << "FDC WRITE CMD : 0x" << setw(2) << hex << std::setfill('0') << (((unsigned int)command) & 0xff) << " " << cmdName << endl;
}

void WD_FDC::Write(uint16_t addr, uint8_t value)
{
  m_noPrint = false;
  std::string title;
  switch (addr & 0x3) {
    case 0:
      WriteCommand(value);
      title = "CMD";
      break;
    case 1:
      title = "TRK";
      m_track = value;
      break;
    case 2:
      title = "SECT";
      m_sector = value;
      break;
    case 3:
      title = "DATA";
      m_data = value;
      break;
  }
  if (!m_noPrint)
    cerr << "FDC WRITE " << title << ": 0x" << setw(2) << hex << std::setfill('0') << (int)value << endl;
}

uint8_t WD_FDC::Read(uint16_t addr)
{
  m_noPrint = false;
  uint8_t value = 0;
  std::string title;
  switch (addr & 0x3) {
    case 0:
      value = ReadStatus();
      title = "STATUS";
      break;
    case 1:
      title = "TRK";
      value = m_track;
      break;
    case 2:
      title = "SEC";
      value = m_sector;
      break;
    case 3:
      title = "DATA";
      value = 0x00;;
      break;
  }
  if (!m_noPrint)
    cerr << "FDC READ " << title <<": " << setw(2) << hex << std::setfill('0') << (unsigned)value << endl;
}

/////////////////////////////////////////////////////////////

VirtualDrive::VirtualDrive()
{}

VirtualDrive::~VirtualDrive()
{}

std::string VirtualDrive::GetName()
{
  return m_name;
}

/////////////////////////////////////////////////////////////

VirtualDriveFile::VirtualDriveFile()
{}

VirtualDriveFile::~VirtualDriveFile()
{}

bool VirtualDriveFile::Open(const std::string & name, int sectorSize)
{
  m_name = name;
  return false;
}

bool VirtualDriveFile::Mount(bool readonly)
{
  return false;
}

bool VirtualDriveFile::ReadSector(int track, int sector, uint8_t * data, int len)
{
  return false;
}

bool VirtualDriveFile::WriteSector(int track, int sector, uint8_t * data, int len)
{
  return false;
}
