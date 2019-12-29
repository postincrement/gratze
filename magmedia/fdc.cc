#include "fdc.h"

#include <iomanip>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

using namespace std;

#define   INIT_TIME_MS      200

#define   JV3_SECTOR_COUNT  (2901 / 3)

#define   SD_SECTOR_SIZE    256
#define   SD_SECTOR_COUNT   10   

#define   STATUS_BUSY           (1 << 0)
#define   STATUS_INDEX          (1 << 1)
#define   STATUS_DRQ            (1 << 1)
#define   STATUS_TRK0           (1 << 2)
#define   STATUS_LOST_DATA      (1 << 2)
#define   STATUS_CRCERR         (1 << 3)
#define   STATUS_SEEKERR        (1 << 4)
#define   STATUS_RECORDNOTFOUND (1 << 4)
#define   STATUS_HEADON         (1 << 5)
#define   STATUS_WR_PROT        (1 << 6)
#define   STATUS_NOT_READY      (1 << 7)

WD_FDC::WD_FDC()
{
  Reset();
}

void WD_FDC::Reset()
{
  m_state = 0;

  m_drive = -1;
  m_sector = 1;
  m_track = 0;

  m_setInterrupt = false;
  m_intOnNotReadyToReady = false;

  ResetStatus();
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
  if (drive != m_drive) {
    if ((drive >= m_drives.size()) || (!m_drives[drive])) {
      cerr << "FDC: SELECT DRIVE " << drive << " - error : undefined drive " << (int)drive << endl;
    }

    cerr << "FDC: SELECT DRIVE " << drive << endl;
    m_drive = drive;
  }
}

void WD_FDC::ResetStatus()
{
  m_status    = 0;
  m_bufferLen = 0;
  m_bufferPtr = 0;
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
      //m_noPrint = true;
      break; 

    // starting (used to fake out the ROM)
    case 1:
      if (now < m_timer) {
        //m_noPrint = true;
        return m_status;
      }

      m_status = 0;
      m_state = 2;
      break;  

    // ready
    // ready
    case 2:
      break;
  }

  return m_status;
}

uint8_t WD_FDC::ReadData()
{
  // if no data, reset DRQ
  if (m_bufferLen == 0) {
    ResetStatus();
    return 0;
  }

  // if more data, reset DRQ
  uint8_t data = m_buffer[m_bufferPtr++];
  if (m_bufferPtr < m_bufferLen) {
    m_status |= STATUS_DRQ;
  } 
  else {
    ResetStatus();
    m_status &= 0x60;
    switch (m_dam) {
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
    if (m_intOnNotReadyToReady)
      m_setInterrupt = true;
  }

  //cerr << "FDC READ DATA: " << dec << setw(4) << m_bufferPtr << " 0x" << setw(2) << setfill('0') << hex << (int)data << endl;

  return data;  
}

void WD_FDC::WriteCommand(int8_t command)
{
  auto now = std::chrono::system_clock::now();

  std::string cmdName; // = "unknown";

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
          cmdName = "Home";
          ResetStatus();
          m_track = 0;
          m_setInterrupt = true;
          break;

        // seek
        case 0x10:
          {
            std::stringstream strm;
            strm << "Seek " << dec << (int)m_data;
            cmdName = strm.str();
          }
          if (!m_drives[m_drive]) {
            m_status = STATUS_RECORDNOTFOUND;
          }
          else {
            ResetStatus();
            m_track = m_data;
          }
          m_setInterrupt = true;
          break;

        // step
        case 0x20:
        case 0x30:
          cmdName = "Step";
          ResetStatus();
          break;

        // step in
        case 0x40:
        case 0x50:
          cmdName = "Step In";
          ResetStatus();
          break;

        // step out
        case 0x60:
        case 0x70:
          ResetStatus();
          cmdName = "Step Out";
          break;

        // read
        case 0x80:
        case 0x90:
          //cmdName = "Read";
          ResetStatus();
          if (!m_drives[m_drive]) {
            m_status = STATUS_RECORDNOTFOUND;
            m_setInterrupt = true;
          }
          else {
            VirtualDrive::SectorInfo info;
            m_bufferLen = m_drives[m_drive]->ReadSector(m_track, m_sector, info, m_buffer, MAX_SECTOR_SIZE);
            if ((m_bufferLen <= 0)) { // || (info.m_density != m_density)) {
              cerr << "FDC: read track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << " failed" << endl;
              m_status = STATUS_RECORDNOTFOUND;
              m_setInterrupt = true;
              m_bufferPtr = 0;
              m_bufferLen = 0;
            }
            else {
              cerr << "FDC: read track=" << dec << (int)m_track << ",sector=" << dec << (int)m_sector << ", len = " << (int)m_bufferLen << ", density = " << (int)info.m_density << ", DAM 0x" << hex << setw(2) << setfill('0') << (int)info.m_dam << endl;
              m_bufferPtr = 0;
              m_dam = info.m_dam;
              m_status |= (STATUS_DRQ | STATUS_BUSY);
            }
          }
          break;

        // write
        case 0xa0:
        case 0xb0:
          cmdName = "Write";
          ResetStatus();
          break;

        // read address
        case 0xc0:
          cmdName = "Read Address";
          ResetStatus();
          break;

        // read track
        case 0xe0:
          cmdName = "Read Track";
          ResetStatus();
          break;

        // write track
        case 0xf0:
          if ((command & 0xff) == 0xfe) {
            cmdName = "Enable Double Density";
            m_density = true;
          }
          else if ((command & 0xff) == 0xf4) {
            cmdName = "Write Track";
            ResetStatus();
          }
          else {
            cmdName = "Unknown command";
          }
          break;

        // force interrupt
        case 0xd0:
          cmdName = "Force Interrupt";
          if (m_status & STATUS_BUSY) {
            ResetStatus();
          }
          else {
            ResetStatus();
            m_intOnNotReadyToReady = (command & 0x01) != 0;
            if ((command & 0x0e) != 0) {
              cerr << "error: unsupported ForceInterrupt 0x" << setw(2) << std::setfill('0') << hex << (command & 0x0f) << endl;
            }
          }
          break;
      }
      break;
  }

  m_noPrint = true;
  if (!cmdName.empty())
    cerr << "FDC CMD : 0x" << setw(2) << hex << std::setfill('0') << (((unsigned int)command) & 0xff) << " " << cmdName << endl;

  if (m_setInterrupt) {
    m_interruptHandler();
    m_setInterrupt = false;
  }
}

void WD_FDC::Write(uint16_t addr, uint8_t value)
{
  m_noPrint = false;
  std::string title;
  switch (addr & 0x3) {
    case 0:
      if (addr != 0x37ec) {
        cerr << "FDC: write to command reg from address 0x" << setw(4) << setfill('0') << addr << endl;
      }
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
  //if (!m_noPrint)
  //  cerr << "FDC SET " << title << ": 0x" << setw(2) << hex << std::setfill('0') << (int)value << endl;

  if (m_setInterrupt) {
    m_interruptHandler();
    m_setInterrupt = false;
  }
}

uint8_t WD_FDC::Read(uint16_t addr)
{
  uint8_t value = 0;
  switch (addr & 0x3) {
    case 0:
      value = ReadStatus();
      break;
    case 1:
      value = m_track;
      break;
    case 2:
      value = m_sector;
      break;
    case 3:
      value = ReadData(); 
      break;
  }

  if (m_setInterrupt) {
    m_interruptHandler();
    m_setInterrupt = false;
  }

  return value;
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

  cerr << "info: file '" << name << "' is len " << len << " bytes = " << (int)m_trackCount+1 << " tracks" << endl;

  return true;
}

void VirtualDriveFile::ReadJV1(off_t len, std::stringstream & formatError)
{
  m_trackCount    = (len / (SD_SECTOR_COUNT * SD_SECTOR_SIZE));

  if (len != (SD_SECTOR_COUNT * SD_SECTOR_SIZE * m_trackCount)) {
    formatError << "file length " << len << " is not compatible";
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
        cerr << "error: unknown DAM code 0x" << setw(2) << setfill('0') << dam << endl;
      }
      else {
        m_trackCount = std::max(m_trackCount, (int)track);
        m_sectorMap.emplace(sector + (side << 8) + (track << 16), SectorInfo(offs, sectorSize, dam, density));
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
  auto r = m_sectorMap.find((sector + 1) + (side << 8) + (track << 16));
  if (r == m_sectorMap.end()) {
    cerr << "error: request for unknown sector " << dec << sector << " and track " << track << endl;
    return -1;
  }

  info = r->second;  
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
