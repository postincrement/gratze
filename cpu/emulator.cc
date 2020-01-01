#include <memory.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "cpu/emulator.h"
#include "magmedia/fdc.h"
#include "video/virtual_screen.h"
#include "mainwindow.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator(EmulatorInfo * info)
  : m_info(info)
{
  if ((info->m_rom.m_size_k > 0) && info->m_rom.m_data) {
    m_rom           = info->m_rom.m_data;
    m_romSize_bytes = info->m_rom.m_size_k * 1024;
  } 
}

const EmulatorInfo & Emulator::GetInfo() const
{
  return *m_info;
}

bool Emulator::Open(const Options & options)
{
  // get the drives
  for (auto &r : options.m_driveFns) {
    std::string fn(r.second);
    VirtualDriveFile *drive = new VirtualDriveFile();
    if (!drive->Open(fn, true))
      return false;
    if (!MountDrive(r.first, drive, true))
      return false;
    cerr << "info: mounted '" << fn << " as drive " << r.first << endl;
  }

  // size the video RAM
  if (m_info->m_video.m_memorySize_k > 0) {
    m_videoRAM.resize(m_info->m_video.m_memorySize_k * 1024);
  }

  // set target CPU speed
  m_targetCPUClock_Hz = m_info->m_cpuCLockSpeed_MHz * 1000000.0;
  m_actualCPUClock_Hz = m_targetCPUClock_Hz;

  return true;
}

bool Emulator::SetRAMSize_k(int len)
{
  m_ram.resize(len * 1024);
  m_ramSize_bytes = m_ram.size();
  m_ramMask = (m_ramSize_bytes - 1);

  cout << "info: RAM size " << len << " k, " << m_ramSize_bytes << " bytes, " << hex << m_ramMask << endl;
} 

int Emulator::GetRAMSize_k() const
{
  return m_ram.size() / 1024;
}


double Emulator::GetActualCPUSpeed_Hz() const
{
  return m_actualCPUClock_Hz;
}

bool Emulator::Poll()
{
  if (m_video)
    m_video->Update(false);

  SDL_Event event;
  if (SDL_PollEvent(&event)) {
    switch (event.type) {

      case SDL_QUIT:
        return false;
        break;

      case SDL_KEYDOWN:
        if (event.key.repeat == 0) {
          OnKeyDown(event.key.keysym);
        }
        break;

      case SDL_KEYUP:
        if (event.key.repeat == 0)
          OnKeyUp(event.key.keysym);
        break;

      default:
        break;
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::OpenVideo(MainWindow & mainWindow, const Options & options)
{
  switch (m_info->m_video.m_type) {
    case EmulatorInfo::VideoDriver::eExplicit:
      break;
    case EmulatorInfo::VideoDriver::eDG640:
      InitializeDG640();
      break;
    case EmulatorInfo::VideoDriver::eNone:
      return false;
      break;
  }

  // create the virtual screen
  m_video.reset(new VirtualScreen(mainWindow, *this, options));
  return true;
}

void Emulator::CreateScreen(MainWindow & mainWindow, const Options & options)
{
  cerr << "Opening window at " << dec << m_info->m_video.m_screenWidth << "x" << m_info->m_video.m_screenHeight << endl;

  // create main window with out best guess at the size
  mainWindow.Open(2, m_info->m_video.m_screenWidth, m_info->m_video.m_screenHeight);

  cerr << "Opening virtual screen" << endl;

  // get the emulator to create the font, and any other video options
  OpenVideo(mainWindow, options);

  cerr << "Getting font size" << endl;

  // calculate the correct size given the final font and the required rows and columns
  int finalWidth  = m_info->m_video.m_screenCols * m_info->m_video.m_fontWidth;
  int finalHeight = m_info->m_video.m_screenRows * m_info->m_video.m_fontHeight;

  double hScale = 1.0;
  double vScale = 1.0;

  bool changed = false;

  if (finalWidth != m_info->m_video.m_screenWidth) {
    hScale = finalWidth * 1.0 / m_info->m_video.m_screenWidth;
    changed = true;
  }
  if (finalHeight != m_info->m_video.m_screenHeight) {
    vScale = finalHeight * 1.0 / m_info->m_video.m_screenHeight;
    changed = true;
  }

  if (changed) {
    cout << "info: screen resize from " << dec << m_info->m_video.m_screenWidth << "x" << m_info->m_video.m_screenHeight << " to " <<  finalWidth << "x" << finalHeight << endl; 
    //mainWindow.SetFinalSizePixels(finalWidth, finalHeight)
    mainWindow.Open(2, finalWidth, finalHeight);

    // get the emulator to create the font, and any other video options
    OpenVideo(mainWindow, options);
/*
    SDL_RenderSetScale(SDL_Renderer* renderer,
                       float         scaleX,
                       float         scaleY)
*/
  }
}

void Emulator::WriteVideoChar(unsigned int offset, uint8_t ch)
{
  if (m_video)
    m_video->WriteChar(offset, ch);
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::OnKeyDown(SDL_Keysym &keysym)
{
}

void Emulator::OnKeyUp(SDL_Keysym &keysym)
{
}

/////////////////////////////////////////////////////////////////////////////////////

uint8_t Emulator::ReadNull(uint16_t)
{
  return 0x00;
}

void Emulator::WriteNull(uint16_t, uint8_t)
{
}

uint8_t Emulator::ReadLog(uint16_t addr)
{
  cerr << "READ 0x" << std::setw(4) << std::setfill('0') << hex << addr << endl;
  return 0x00;
}

void Emulator::WriteLog(uint16_t addr, uint8_t val)
{
  cerr << "WRITE 0x" << std::setw(4) << std::setfill('0') << hex << addr << " 0x" << std::setw(2) << hex << (int)val << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::ReadROMFromFile(const std::string &filename, unsigned char *ptr, int len)
{
  ifstream file(filename.c_str(), ifstream::in | ifstream::binary);
  if (!file.is_open())
  {
    cerr << "error: cannot read ROM file '" << filename << "'" << endl;
    return false;
  }

  if (len <= 0)
  {
    file.seekg(0, ios::end);
    len = file.tellg();
    file.seekg(0, ios::beg);
  }

  file.read((char *)ptr, len);

  return !file.fail();
}

void Emulator::DumpStack(const std::vector<uint16_t> & stack)
{
  DumpStackInternal(stack);
}

void Emulator::DumpStack(int count)
{
  std::vector<uint16_t> stack;
  stack.resize(count);
  GetStack(stack);
  DumpStack(stack);
}

bool Emulator::MountDrive(int driveNum, VirtualDrive * drive, bool readOnly)
{
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////
