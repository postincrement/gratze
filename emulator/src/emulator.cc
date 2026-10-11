#include <memory.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <functional>
#include <math.h>
#include <algorithm>
#include <chrono>
#include <thread>

#include "common/misc.h"
#include "common/binfile.h"
#include "common/config.h"
#include "src/emulator.h"
#include "devices/fdc.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"
#include "terminal/terminal.h"
#include "common/grz.h"

extern "C" {
#include "nfd.h"
};

using namespace std;

#define TRACE_SYM   SDLK_F2
#define COLOUR_SYM  SDLK_F9
#define DUMP_SYM    SDLK_F10
#define LOAD_SYM    SDLK_F11
#define REBOOT_SYM  SDLK_F12


Emulator * Emulator::m_instance = nullptr;

static double SteadyNowSeconds()
{
  return std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Sleep the bulk of a lead, then spin out the last millisecond. Host sleep
// wakes late by about a millisecond, and the spin absorbs that overshoot.
static void WaitUntilVirtual(std::chrono::steady_clock::time_point virtualNow)
{
  using clock = std::chrono::steady_clock;
  auto host = clock::now();
  if (virtualNow <= host)
    return;

  if (virtualNow - host > std::chrono::milliseconds(2)) {
    auto wake = virtualNow - std::chrono::milliseconds(1);
    if (wake > host)
      std::this_thread::sleep_until(wake);
  }

  while (clock::now() < virtualNow)
    ;
}

/////////////////////////////////////////////////////////////////////////////////////

Emulator::Emulator(const EmulatorInfo * info)
  : m_info(info)
{
  m_instance = this;
}

Emulator::~Emulator()
{}

void Emulator::Instantiate()
{
}

const EmulatorInfo & Emulator::GetInfo() const
{
  return *m_info;
}

bool Emulator::Open(const Options & options)
{
  return true;
}

void Emulator::Reset(int addr)
{
  m_pollers.m_list.clear();

  using namespace std::placeholders;
  AddRealTimePollDef(1.0,   std::bind(&Emulator::CalcCPUSpeed,  this, _1, _2));

  if (m_options.m_useSDL) {
    AddRealTimePollDef(0.01,   std::bind(&Emulator::CheckSDLKeyboard, this));
    AddRealTimePollDef(100000, std::bind(&Emulator::UpdateScreen,  this));
    AddRealTimePollDef(0.1,    std::bind(&Emulator::UpdateScreen,  this));
  }

  if (m_terminal)
    m_terminal->AddPollers(*this);
}

bool Emulator::SetRAMSize_k(int len)
{
  //m_ram.resize(len * 1024);
  m_ramSize_bytes = len * 1024; //m_ram.size();
  m_ramMask = (m_ramSize_bytes - 1);

  if (m_options.m_verbose)
    cout << "info: RAM size " << len << " k, " << m_ramSize_bytes << " bytes, " << HEXFORMAT0x4(m_ramMask) << endl;

  return true;
}

int Emulator::GetRAMSize_k() const
{
  return m_ramSize_bytes / 1024;
}

double Emulator::GetActualCPUSpeed_Hz() const
{
  return m_actualCPUClock_Hz;
}

void Emulator::UpdateScreen()
{
  if (m_mainWindow != nullptr) {
    RefreshPanelDrives();
    m_mainWindow->SetDrives(m_panelDrives);
    m_mainWindow->SetCpuHz(m_actualCPUClock_Hz);
  }
  if (m_screen)
    m_screen->Update(false);
  if (m_mainWindow != nullptr)
    m_mainWindow->Update();
}

void Emulator::SendKeyText(const SDL_Event & event)
{
  if (m_keyboard == nullptr)
    return;

  SDL_Keycode keycode = event.key.keysym.sym;
  SDL_Keymod mod = SDL_GetModState();
  bool ctrl = (mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0;

  int ascii = -1;

  // Printable glyphs arrive through SDL_TEXTINPUT so the host layout is kept.
  // These keys do not, and a parallel keyboard such as the DG680 PIO needs them.
  if ((keycode == SDLK_RETURN) || (keycode == SDLK_KP_ENTER))
    ascii = 0x0d;
  else if (keycode == SDLK_ESCAPE)
    ascii = 0x1b;
  else if (keycode == SDLK_BACKSPACE)
    ascii = 0x08;
  else if (keycode == SDLK_TAB)
    ascii = 0x09;
  else if (keycode == SDLK_DELETE)
    ascii = 0x7f;
  else if (ctrl && (keycode >= 'a') && (keycode <= 'z'))
    ascii = keycode & 0x1f;

  if (ascii < 0)
    return;

  char ch = (char)ascii;
  m_keyboard->OnKeyText(std::string(&ch, 1));
}

void Emulator::CheckSDLKeyboard()
{
  SDL_Event event;
  if (!SDL_PollEvent(&event))
    return;

  switch (event.type) {

    case SDL_QUIT:
      exit(0);
      break;

    case SDL_TEXTINPUT:
      if (m_keyboard == nullptr)
        break;
      if (m_keyboard->IsScanned())
        m_keyboard->OnKeyText(event.text.text);
      else {
        std::string ascii;
        for (const char * p = event.text.text; *p != 0; ++p) {
          unsigned char ch = (unsigned char)*p;
          if ((ch >= 0x20) && (ch < 0x7f))
            ascii.push_back((char)ch);
        }
        if (!ascii.empty())
          m_keyboard->OnKeyText(ascii);
      }
      break;

    case SDL_WINDOWEVENT:
      if ((event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) && (m_keyboard != nullptr))
        m_keyboard->Reset();
      break;

    case SDL_MOUSEMOTION:
      if (m_mainWindow != nullptr)
        m_mainWindow->SetMouse(event.motion.x, event.motion.y);
      break;

    case SDL_MOUSEBUTTONDOWN:
      if ((event.button.button == SDL_BUTTON_LEFT) && (m_mainWindow != nullptr) && m_mainWindow->HitReset(event.button.x, event.button.y))
        RequestReset();
      break;

    case SDL_KEYDOWN:
      if (OnHostKey(event.key.keysym))
        break;

      if ((m_keyboard == nullptr) || !m_keyboard->IsScanned())
        SendKeyText(event);

      if (event.key.repeat == 0) {
        if (event.key.keysym.sym == TRACE_SYM)
          SetTrace(true);

        if (event.key.keysym.sym == TRACE_SYM + 1)
          SetTrace(false);

        if (event.key.keysym.sym == COLOUR_SYM) {
          ChangeVideoColour();
        }
        else if (event.key.keysym.sym == DUMP_SYM) {
          MemoryDump();
        }
        else if (event.key.keysym.sym == LOAD_SYM) {
          LoadFile();
        }
        else if (event.key.keysym.sym == REBOOT_SYM) {
          Reset();
        }
        else if (m_keyboard != nullptr) {
          if (m_options.m_keyboardDebug) {
            cerr << "debug: key down " << HEXFORMAT0x8(event.key.keysym.sym) << endl;
          }
          m_keyboard->OnKeyDown(event.key.keysym);
        }
        else {
          OnKeyDown(event.key.keysym);
        }
        break;

      case SDL_KEYUP:
        if (event.key.repeat == 0) {
          if (m_keyboard != nullptr) {
            if (m_options.m_keyboardDebug) {
              cerr << "debug: key up " << HEXFORMAT0x8(event.key.keysym.sym) << endl;
            }
            m_keyboard->OnKeyUp(event.key.keysym);
          }
          else {
            OnKeyUp(event.key.keysym);
          }
        }
        break;

      default:
        break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::SetKeyboard(VirtualKeyboard * kb)
{
  m_keyboard.reset(kb);
}

/////////////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
extern "C" bool MacUsableDisplaySize(int * width, int * height);
#endif

static bool UsableDisplaySize(int & width, int & height)
{
#ifdef __APPLE__
  if (MacUsableDisplaySize(&width, &height))
    return true;
#endif

  SDL_Rect bounds;
  if ((SDL_GetDisplayUsableBounds(0, &bounds) == 0) && (bounds.w > 0) && (bounds.h > 0)) {
    width = bounds.w;
    height = bounds.h;
    return true;
  }

  SDL_DisplayMode mode;
  if ((SDL_GetDesktopDisplayMode(0, &mode) == 0) && (mode.w > 0) && (mode.h > 0)) {
    width = mode.w;
    height = mode.h;
    return true;
  }

  return false;
}

// Largest integer scale whose window still fits in about four fifths of the
// usable desktop, leaving the menu bar, dock, and some space around it.
static unsigned ScaleToFit(int contentW, int contentH, int desktopW, int desktopH)
{
  if (contentW < 1)
    contentW = 1;
  if (contentH < 1)
    contentH = 1;

  int availW = desktopW * 4 / 5 - 20;
  int availH = desktopH * 4 / 5 - 20 - StatusPanel::kHeight;
  if (availW < contentW)
    availW = contentW;
  if (availH < contentH)
    availH = contentH;

  unsigned scaleW = (unsigned)(availW / contentW);
  unsigned scaleH = (unsigned)(availH / contentH);
  unsigned scale = std::min(scaleW, scaleH);
  if (scale < 1)
    scale = 1;
  return scale;
}

void Emulator::RequestReset()
{
  m_resetRequested = true;
}

void Emulator::RefreshPanelDrives()
{
  m_panelDrives.clear();
  for (const auto & mounted : m_mountedDriveNames) {
    StatusDrive drive;
    // Drive indices are A=0 … P=15 for CP/M and FDC mounts.
    drive.m_label.assign(1, (char)('A' + (mounted.first % 26)));
    drive.m_name = mounted.second;
    drive.m_mounted = true;
    m_panelDrives.push_back(drive);
  }
}

void Emulator::CollectFdcDrives(const WD_FDC * fdc)
{
  m_panelDrives.clear();
  int slots = 4;
  if ((fdc != nullptr) && (fdc->DriveSlots() > slots))
    slots = fdc->DriveSlots();

  for (int i = 0; i < slots; ++i) {
    StatusDrive drive;
    drive.m_label.assign(1, (char)('A' + i));
    auto mounted = m_mountedDriveNames.find(i);
    if (mounted != m_mountedDriveNames.end())
      drive.m_name = mounted->second;
    drive.m_mounted = (fdc != nullptr) && fdc->DrivePresent(i);
    drive.m_selected = (fdc != nullptr) && (fdc->SelectedDrive() == i);
    drive.m_busy = drive.m_selected && fdc->HeadLoaded();
    m_panelDrives.push_back(drive);
  }
}

void Emulator::ArmPollers()
{
  double now = SteadyNowSeconds();
  for (auto & r : m_pollers.m_list) {
    PollDef & def = r.second;
    def.m_lastTime  = now;
    def.m_lastClock = m_cycleCounter;
    if (def.m_pollIsTime)
      def.m_nextTime = now + def.m_timeInterval;
    else
      def.m_nextClock = m_cycleCounter + def.m_clockInterval;
  }
}

void Emulator::CreateScreen(MainWindow & mainWindow)
{
  if (m_options.m_useSDL)
    m_mainWindow = &mainWindow;
  if (m_info->m_title != nullptr)
    mainWindow.SetTitle(m_info->m_title);
  int vdup = 1;                 // duplicate lines for fields

  int pixelCols;
  int pixelRows;

  int width;
  int height;

  // get size of screen
  SDL_DisplayMode mode;
  if (m_options.m_useSDL) {
    SDL_GetDesktopDisplayMode(0, &mode);
    cout << "info: desktop size is " << mode.w << "x" << mode.h << endl;
  }

  // check for memory mapped screens
  const Config::MemoryMappedScreen * mmapScreenInfo = GetMemoryMappedInfo();
  if (mmapScreenInfo != nullptr) {

    if (!m_options.m_useSDL) {
      cerr << "error: must use SDL for memory mapped screens" << endl;
      exit(-1);
    }

    cout << "info: emulated screen is memory mapped" << endl;

    const Config::Block * block     = GetConfigBlock(Config::Type::eMonitor);
    const Config::Monitor * monitor = (block == nullptr) ? nullptr : &block->m_info.m_monitor;

    // display video output pixels
    pixelCols = mmapScreenInfo->m_screenWidth;
    pixelRows = mmapScreenInfo->m_screenHeight;

    if (m_options.m_verbose)
      cout << "info: virtual screen base pixel size is " << pixelCols << "x" << pixelRows << endl;

    if (m_options.m_videoScale == 0) {
      int deskW = 0;
      int deskH = 0;
      if (UsableDisplaySize(deskW, deskH))
        m_options.m_videoScale = ScaleToFit(pixelCols, pixelRows, deskW, deskH);
      else
        m_options.m_videoScale = 2;
      cout << "info: screen scale is " << m_options.m_videoScale << endl;
    }

    width  = pixelCols * m_options.m_videoScale;
    height = pixelRows * m_options.m_videoScale;

    if (m_options.m_verbose)
      cout << "info: scaled virtual screen pixel size is " << width << "x" << height << endl;

    // create main window
    mainWindow.Open(width, height);

    m_memMapScreen.reset(MemoryMappedScreen::Create(mainWindow, m_options, *mmapScreenInfo));
    m_screen = m_memMapScreen;
    m_screen->SetScale(m_options.m_videoScale, m_options.m_videoScale);

    bool fontOpened;
    if (!m_options.m_font.empty())
      fontOpened = m_memMapScreen->SetFont(new TTFFont(mmapScreenInfo->m_font, 128, m_options.m_font, m_options.m_fontSize));
    else
      fontOpened = m_memMapScreen->SetFont(new PixelFont(mmapScreenInfo->m_font));
    if (!fontOpened)
      exit(-1);
  }

  // check for terminals
  else {
    const Config::Block * block = GetConfigBlock(Config::Type::eTerminal);
    if (block == nullptr) {
      cerr << "error: no screen or terminal defined" << endl;
      exit(-1);
    }

    if (m_options.m_verbose)
      cout << "info: using terminal" << endl;

    const Config::Terminal & termInfo = block->m_info.m_terminal;

    // create main window with a guess at the size
    if (m_options.m_useSDL) {
      if (m_options.m_verbose)
        cout << "info: using SDL" << endl;
      if (m_options.m_videoScale == 0) {
        int deskW = 0;
        int deskH = 0;
        int nativeW = termInfo.m_cols * 8;
        int nativeH = termInfo.m_rows * 16;
        if (UsableDisplaySize(deskW, deskH))
          m_options.m_videoScale = ScaleToFit(nativeW, nativeH, deskW, deskH);
        else
          m_options.m_videoScale = 1;
        cout << "info: screen scale is " << m_options.m_videoScale << endl;
      }
      mainWindow.Open(800 * m_options.m_videoScale, 600 * m_options.m_videoScale);
      m_terminal.reset(new SDLTerminal(mainWindow, m_options, termInfo.m_cols, termInfo.m_rows, m_options.m_videoScale, m_options.m_videoScale));
    }
    else {
      if (m_options.m_verbose)
        cout << "info: using console" << endl;
      m_terminal.reset(new ConsoleTerminal(m_options, termInfo.m_cols, termInfo.m_rows));
    }

    m_screen   = m_terminal->m_screen;
    m_keyboard = m_terminal->m_keyboard;

    if (!m_terminal->Open()) {
      cerr << "error: could not open terminal" << endl;
      exit(-1);
    }

    if (m_options.m_useSDL) {
      std::string fontName = m_options.m_font;
      if (fontName.empty())
        fontName = DEFAULT_TTF_FONT;

      int fontSize = m_options.m_fontSize;
      if (fontSize <= 0)
        fontSize = 15;

      if (!m_screen->SetFont(new TTFFont(fontName, fontSize)))
        exit(-1);
    }
  }

  if (!m_screen) {
    cerr << "error: could not instantiate screen" << endl;
    return; // false;
  }

  if (m_options.m_videoScale == 0)
    m_options.m_videoScale = 1;

  if (m_options.m_verbose)  
    cout << "info: setting screen scale " << m_options.m_videoScale << endl;

  m_screen->SetScale(m_options.m_videoScale, m_options.m_videoScale);
  m_screen->Open();
}

void Emulator::WriteToVideo(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  if (addr > info.m_endAddr)
    cerr << "warning: bad video write" << endl;
  else
    m_memMapScreen->WriteMemoryAtAddress(addr - info.m_startAddr, data);
}

uint8_t Emulator::ReadFromVideo(const ReadMemoryBlockInfo & info, uint16_t addr) const
{
  if (addr > info.m_endAddr) {
    cerr << "warning: bad video read" << endl;
    return 0x00;
  }
  else
    return m_memMapScreen->ReadMemoryAtAddress(addr - info.m_startAddr);
}

void Emulator::ChangeVideoColour()
{
  SDL_Color newFg;
  SDL_Color newBg;

  m_memMapScreen->GetFontColour(newFg, newBg);

  newFg.r ^= 0xff;
  newFg.b ^= 0xff;

  m_memMapScreen->SetFontColour(newFg, newBg);
}

/////////////////////////////////////////////////////////////////////////////////////

bool Emulator::OnHostKey(const SDL_Keysym &)
{
  return false;
}

void Emulator::OnKeyDown(const SDL_Keysym &keysym)
{
  cerr << "warning: emulator got key down sym code " << HEXFORMAT0x2(keysym.sym) << endl;
}

void Emulator::OnKeyUp(const SDL_Keysym &keysym)
{
  cerr << "warning: emulator got key up sym code " << HEXFORMAT0x2(keysym.sym) << endl;
}

void Emulator::OnKeyText(const std::string & str)
{
  cerr << "warning: emulator got key string '" << str << "'" << endl;
}

/////////////////////////////////////////////////////////////////////////////////////

const Config::Block * Emulator::GetConfigBlock(Config::Type type) const
{
  return GetConfigBlock(type, m_info->m_blocks, m_info->m_blockCount);
}

const Config::Block * Emulator::GetConfigBlock(Config::Type type, 
                                              const Config::Block * blocks,
                                              size_t blockCount) const
{
  for (int p = 0; p < blockCount; ++p) {
    const Config::Block & block = blocks[p];
    if (block.m_type == Config::Type::eEnd)
      break;
    if (block.m_type == type)
      return &block;
    if (block.m_type == Config::Type::eParent) {
      const Config::Parent & parent = block.m_info.m_parent;
      const Config::Block * ptr = GetConfigBlock(type, parent.m_blocks, parent.m_blockCount);
      if (ptr != nullptr)
        return ptr;
    }
  }
  return nullptr;
}

const Config::CPU * Emulator::GetCPUInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eCPU);
  return (info == nullptr) ? nullptr : &info->m_info.m_cpu;
}

const Config::MemoryMappedScreen * Emulator::GetMemoryMappedInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eMemoryMappedScreen);
  return (info == nullptr) ? nullptr : &info->m_info.m_memoryMappedScreen;
}

const Config::RAM * Emulator::GetMainRAMInfo() const
{
  const Config::Block * info = GetConfigBlock(Config::Type::eMainRAM);
  return (info == nullptr) ? nullptr : &info->m_info.m_ram;
}

/////////////////////////////////////////////////////////////////////////////////////

void Emulator::CompileConfigBlocks()
{
  m_readMemoryBlocks.clear();
  m_writeMemoryBlocks.clear();
  m_readIOPortBlocks.clear();
  m_writeIOPortBlocks.clear();

  if (m_options.m_verbose) {
    cout << "debug: compiling config blocks" << endl;
  }

  CompileConfigBlocks(m_info->m_blocks, m_info->m_blockCount);

  if (m_options.m_verbose) {
    for (auto & r : m_writeMemoryBlocks) {
      cout << "WRITE " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl; // << " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
    }
    for (auto & r : m_readMemoryBlocks) {
      cout << "READ  " << HEXFORMAT0x4(r.m_startAddr) << " - " << HEXFORMAT0x4(r.m_endAddr) << " - " << (int)r.m_type << endl; //  <<  " " << (void *)r.m_function << " " << (void *)r.m_realFunction << endl;
    }

    for (auto & r : m_writeIOPortBlocks) {
      cout << "WRITE IO " << HEXFORMAT0x2(r.m_startPort) << " - " << HEXFORMAT0x2(r.m_endPort) << " - " << (int)r.m_type << endl; //  << " " << (void *)r.m_function << endl;
    }
    for (auto & r : m_readIOPortBlocks) {
      cout << "READ IO " << HEXFORMAT0x2(r.m_startPort) << " - " << HEXFORMAT0x2(r.m_endPort) << " - " << (int)r.m_type << endl; //  <<  " " << (void *)r.m_function << endl;
    }
  }
}

void Emulator::CompileConfigBlocks(const Config::Block * blocks, size_t blockCount)
{
  for (int p = 0; p < blockCount; ++p) {
    const Config::Block & block = blocks[p];

    if (block.m_type == Config::Type::eEnd)
      break;

    if (block.m_type == Config::Type::eParent) {
      const Config::Parent & parent = block.m_info.m_parent;
      CompileConfigBlocks(parent.m_blocks, parent.m_blockCount);
    }

    // add RAM
    else if ((block.m_type == Config::Type::eRAM) || (block.m_type == Config::Type::eMainRAM)) {
      const Config::RAM & info = block.m_info.m_ram;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: RAM block has end address" << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      if (m_options.m_verbose) {
        cout << "debug: RAM block" << endl;
      }
      uint8_t * memory;
      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block.m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        memory = (uint8_t *)malloc(writeInfo.m_endAddr - writeInfo.m_startAddr + 1);
        writeInfo.m_memory = memory;
        if (m_options.m_writeMemory) {
          writeInfo.m_function  = &Emulator::DebugWriteMemory;
        }
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block.m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_memory    = memory;
        if (m_options.m_readMemory) {
          readInfo.m_function  = &Emulator::DebugReadMemory;
        }
        m_readMemoryBlocks.push_back(readInfo);
      }
    }

    // add ROM
    else if (block.m_type == Config::Type::eROM) {
      if (m_options.m_verbose) {
        cout << "debug: ROM block" << endl;
      }
      const Config::ROM & info = block.m_info.m_rom;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: ROM block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type      = block.m_type;
      readInfo.m_startAddr = info.m_startAddr;
      readInfo.m_endAddr   = info.m_endAddr;
      readInfo.m_memory    = info.m_data;
      if (m_options.m_readMemory) {
        readInfo.m_function  = &Emulator::DebugReadMemory;
      }
      m_readMemoryBlocks.push_back(readInfo);
    }

    // add read memIO
    else if (block.m_type == Config::Type::eMemIORead) {
      const Config::MemIO & info = block.m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO read block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      ReadMemoryBlockInfo readInfo;
      readInfo.m_type         = block.m_type;
      readInfo.m_startAddr    = info.m_startAddr;
      readInfo.m_endAddr      = info.m_endAddr;
      readInfo.m_realFunction = &Emulator::ReadIOMemoryInternal;
      if (m_options.m_readMemory) {
        readInfo.m_function  = &Emulator::DebugReadIOMemory;
      }
      else {
        readInfo.m_function  = &Emulator::ReadIOMemoryInternal;
      }
      readInfo.m_id        = info.m_id;
      m_readMemoryBlocks.push_back(readInfo);
    }

    // add write memIO
    else if (block.m_type == Config::Type::eMemIOWrite) {
      const Config::MemIO & info = block.m_info.m_memIO;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: MemIO write block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }
      WriteMemoryBlockInfo writeInfo;
      writeInfo.m_type         = block.m_type;
      writeInfo.m_startAddr    = info.m_startAddr;
      writeInfo.m_endAddr      = info.m_endAddr;
      writeInfo.m_realFunction = &Emulator::WriteIOMemoryInternal;
      if (m_options.m_writeMemory) {
        writeInfo.m_function  = &Emulator::DebugWriteIOMemory;
      }
      else {
        writeInfo.m_function  = &Emulator::WriteIOMemoryInternal;
      }
      writeInfo.m_id        = info.m_id;
      m_writeMemoryBlocks.push_back(writeInfo);
    }

    // add video
    else if (block.m_type == Config::Type::eMemoryMappedScreen) {
      const Config::MemoryMappedScreen & info = block.m_info.m_memoryMappedScreen;
      if (info.m_variable)
        continue;
      if (info.m_startAddr > info.m_endAddr) {
        cerr << "error: video block has end address " << HEXFORMAT0x4(info.m_endAddr) << " < start address " << HEXFORMAT0x4(info.m_startAddr) << endl;
        exit(-1);
      }

      {
        WriteMemoryBlockInfo writeInfo;
        writeInfo.m_type      = block.m_type;
        writeInfo.m_startAddr = info.m_startAddr;
        writeInfo.m_endAddr   = info.m_endAddr;
        writeInfo.m_realFunction = &Emulator::WriteToVideo;
        if (m_options.m_writeVideo) {
          writeInfo.m_function  = &Emulator::DebugWriteMemory;
        }
        else {
          writeInfo.m_function     = &Emulator::WriteToVideo;
        }
        m_writeMemoryBlocks.push_back(writeInfo);
      }
      {
        ReadMemoryBlockInfo readInfo;
        readInfo.m_type      = block.m_type;
        readInfo.m_startAddr = info.m_startAddr;
        readInfo.m_endAddr   = info.m_endAddr;
        readInfo.m_realFunction = &Emulator::ReadFromVideo;
        if (m_options.m_readVideo) {
          readInfo.m_function = &Emulator::DebugReadMemory;
        }
        else {
          readInfo.m_function = &Emulator::ReadFromVideo;
        }
        m_readMemoryBlocks.push_back(readInfo);
      }
    }

    // add read block
    if ((block.m_type == Config::Type::eIOPortRead) || (block.m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block.m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: read IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      ReadIOPortBlockInfo readInfo;
      readInfo.m_type      = block.m_type;
      readInfo.m_startPort = info.m_startPort;
      readInfo.m_endPort   = info.m_endPort;
      readInfo.m_function  = &Emulator::ReadIOPort;
      readInfo.m_id        = info.m_id;
      m_readIOPortBlocks.push_back(readInfo);
    }

    // add write block
    if ((block.m_type == Config::Type::eIOPortWrite) || (block.m_type == Config::Type::eIOPortRW)) {
      const Config::IOPort & info = block.m_info.m_ioPort;
      if (info.m_startPort > info.m_endPort) {
        cerr << "error: write IO port has end port" << HEXFORMAT0x2(info.m_startPort) << " < start address " << HEXFORMAT0x2(info.m_startPort) << endl;
        exit(-1);
      }
      WriteIOPortBlockInfo writeInfo;
      writeInfo.m_type      = block.m_type;
      writeInfo.m_startPort = info.m_startPort;
      writeInfo.m_endPort   = info.m_endPort;
      writeInfo.m_function   = &Emulator::WriteIOPort;
      writeInfo.m_id        = info.m_id;
      m_writeIOPortBlocks.push_back(writeInfo);
    }
  }
}

uint8_t Emulator::ReadMemory(uint16_t addr) const
{
  for (auto & r : m_readMemoryBlocks) {
    if ((addr >= r.m_startAddr) && (addr <= r.m_endAddr)) {
      if (r.m_function != nullptr)
        return std::invoke(r.m_function, *this, r, addr);
      uint8_t data = r.m_memory[addr - r.m_startAddr];
      // cout << "info: read " << HEXFORMAT0x2(data) << " from " << HEXFORMAT0x4(addr - r.m_startAddr) << endl;
      return data;
    }
  }
  return ReadLog(addr);
}

void Emulator::WriteMemory(uint16_t addr, uint8_t data)
{
  for (auto & r : m_writeMemoryBlocks) {
    if ((addr >= r.m_startAddr) && (addr <= r.m_endAddr)) {
      if (r.m_function != nullptr) {
        std::invoke(r.m_function, *this, r, addr, data);
        return;
      }
      r.m_memory[addr - r.m_startAddr] = data;
      return;
    }
  }
  WriteLog(addr, data);
}

uint8_t Emulator::DebugReadMemory(const ReadMemoryBlockInfo & info, uint16_t addr) const
{
  if (info.m_realFunction != nullptr) {
    cout << "debug: reading via fn from " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
    return std::invoke(info.m_realFunction, *this, info, addr);
  }
  if (info.m_memory == nullptr) {
    cout << "debug: reading from " << HEXFORMAT0x4(addr) << " failed with no memory" << endl;
    return 0x00;
  }
  uint8_t v = info.m_memory[addr - info.m_startAddr];
  cout << "debug: read " << HEXFORMAT0x2(v) << " from " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
  return v;
}

void Emulator::DebugWriteMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  stringstream strm;
  strm << HEXFORMAT0x2(data) << " ";
  if (isgraph((char)data) && (data != 0x20)) 
    strm << (char)data;
  else  
    strm << ".";
  if (info.m_realFunction != nullptr) {
    cout << "debug: writing " << strm.str() << " via fn to " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
    std::invoke(info.m_realFunction, *this, info, addr, data);
    return;
  }
  if (info.m_memory == nullptr) {
    cout << "debug: writing " << strm.str() << " to " << HEXFORMAT0x4(addr) << " failed with no memory" << endl;
    return;
  }
  cout << "debug: writing " << strm.str() << " to " << HEXFORMAT0x4(addr) << " in memory block " << HEXFORMAT0x4(info.m_startAddr) << " - " << HEXFORMAT0x4(info.m_endAddr) << endl;
  info.m_memory[addr - info.m_startAddr] = data;
}

uint8_t Emulator::DebugReadIOMemory(const ReadMemoryBlockInfo & info, uint16_t addr) const
{
  int val = -1;
  if (info.m_realFunction != nullptr) {
    val = std::invoke(info.m_realFunction, *this, info, addr);
  }
  cout << "debug: reading IO memory " << info.m_id << " " << HEXFORMAT0x4(addr) << " ";
  if (val < 0)
    cout << "failed" << endl;
  else
    cout << HEXFORMAT0x2(val) << endl;
  return val;
}

void Emulator::DebugWriteIOMemory(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  cout << "debug: writing IO memory " << info.m_id << " " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(data) << endl;
  if (info.m_realFunction != nullptr) {
    std::invoke(info.m_realFunction, *this, info, addr, data);
  }
}

uint8_t Emulator::ReadIOMemoryInternal(const ReadMemoryBlockInfo & info, uint16_t addr) const
{
  return ReadIOMemory(info.m_id, addr);
}

void Emulator::WriteIOMemoryInternal(const WriteMemoryBlockInfo & info, uint16_t addr, uint8_t data)
{
  WriteIOMemory(info.m_id, addr, data);
}

uint8_t Emulator::ReadIOMemory(int, uint16_t) const
{
  return 0xff;
}

void Emulator::WriteIOMemory(int, uint16_t, uint8_t data)
{}

/////////////////////////////////////////////////////////////////////////////////////

uint8_t Emulator::ReadIOPort(const ReadIOPortBlockInfo & info, uint16_t)
{
  return 0x00;
}

void Emulator::WriteIOPort(const WriteIOPortBlockInfo & info, uint16_t port, uint8_t data)
{
}

void Emulator::WritePort(register uint16_t port, register uint8_t data)
{
  if (m_options.m_writeIO) 
    WriteIOPortLog(port, data);
  uint8_t shortPort = port & 0xff;
  for (auto & r : m_writeIOPortBlocks) {
    if (shortPort < r.m_startPort)
      continue;
    if (shortPort <= r.m_endPort) {
      if (r.m_function != nullptr) {
        std::invoke(r.m_function, *this, r, shortPort, data);
     }
      return;
    }
  }
  WriteIOPortLog(port, data);
}

uint8_t Emulator::ReadPort(register uint16_t port)
{
  uint8_t shortPort = port & 0xff;
  for (auto & r : m_readIOPortBlocks) {
    if (shortPort < r.m_startPort)
      continue;
    if (shortPort <= r.m_endPort) {
      if (r.m_function != nullptr) { 
        uint8_t val = std::invoke(r.m_function, *this, r, shortPort);
        if (m_options.m_readIO) {
          ReadIOPortLog(port, val);
        }
        return val;
      }
    }
  }
  ReadIOPortLog(port, -1);
  return 0x00;
}

uint8_t Emulator::ReadNull(uint16_t) const
{
  return 0x00;
}

void Emulator::WriteNull(uint16_t, uint8_t)
{
}

uint8_t Emulator::ReadLog(uint16_t addr) const
{
  cerr << "READ " << HEXFORMAT0x4(addr) << endl;
  return 0x00;
}

void Emulator::WriteLog(uint16_t addr, uint8_t val) const
{
  cerr << "WRITE " << HEXFORMAT0x4(addr) << " " << HEXFORMAT0x2(val) << endl;
}

void Emulator::ReadIOPortLog(uint16_t addr, int val) const
{
  cerr << "READ IO PORT " << HEXFORMAT0x2(addr) << " ";
  if (val < 0)
    cerr << "failed" << endl;
  else
    cerr << HEXFORMAT0x2(val) << endl;
}

void Emulator::WriteIOPortLog(uint16_t addr, uint8_t val) const
{
  cerr << "WRITE IO PORT " << HEXFORMAT0x2(addr) << " " << HEXFORMAT0x2(val) << endl;
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

bool Emulator::MountDrive(int driveNum, std::shared_ptr<VirtualDrive>, bool readOnly)
{
  return false;
}

std::string Emulator::DumpRegs() const
{
  return "";
}

void Emulator::MemoryDump() const
{
  uint16_t addr;
  std::vector<uint8_t> dump;
  dump.resize(GetMemorySize());
  VECTOR_ZERO(dump);

  for (auto & r : m_readMemoryBlocks) {
    if (r.m_memory != nullptr) {
      memcpy(&dump[r.m_startAddr], r.m_memory, r.m_endAddr - r.m_startAddr + 1);
    }
    else {
      for (int addr = r.m_startAddr; addr <= r.m_endAddr; ++addr)
        dump[addr] = ReadMemory(addr);
    }
  }

  std::string basename(GetName() + "_dump");

  {
    std::string filename(basename + ".txt");
    ofstream file(filename);
    if (!file.is_open()) {
      cerr << "error: could not open " << filename << endl;
    }
    else {
      file << DumpRegs();
      file << DumpMemory(&dump[0], dump.size());
    }
    cout << "memory dumped to " << filename << endl;
  }

  {
    std::string filename(basename + ".bin");
    ofstream file(filename, ios::out | ios::trunc | ios::binary);
    if (!file.is_open()) {
      cerr << "error: could not open " << filename << endl;
    }
    else {
      file.write((char *)&dump[0], dump.size());
    }
    cout << "memory dumped to " << filename << endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////

int Emulator::Run(const Options & options)
{
  m_options = options;
  // Memory-mapped machines have to use an SDL window. A terminal machine
  // stays on the host console unless --sdl was given.
  if (m_info->m_sdl)
    m_options.m_useSDL = true;

  // set target CPU speed
  const Config::CPU * cpu = GetCPUInfo();
  if (cpu == NULL) {
    cerr << "error: cannot get CPU information" << endl;
    return false;
  }

  m_targetCPUClock_Hz = cpu->m_clockSpeed_MHz * 1000000.0;
  m_actualCPUClock_Hz = m_targetCPUClock_Hz;

  if (m_options.m_verbose) {
    cout << "debug: target CPU speed is " << m_targetCPUClock_Hz << endl;
  }

  // allow any descendant classes to do whatever they need
  if (!Open(options)) {
    cerr << "error: cannot open emulator" << endl;
    return -1;
  }

  // get the drives (keys 0–15 = A–P)
  for (auto & r : options.m_driveFns) {
    std::string fn(r.second);
    VirtualFileIdentifier fileId;
    std::shared_ptr<VirtualDrive> drive = fileId.Open(fn, true);
    if (drive == nullptr)
      return false;
    char letter = (char)('A' + (r.first % 26));
    if (!MountDrive((int)r.first, drive, true)) {
      cerr << "error: cannot mount drive " << letter << " with " << r.second << endl;
      return -1;
    }
    cerr << "info: mounted '" << fn << "' as drive " << letter << endl;
    auto slash = fn.find_last_of("/\\");
    m_mountedDriveNames[r.first] = (slash == std::string::npos) ? fn : fn.substr(slash + 1);
  }

  CompileConfigBlocks();

  // initialize SDL
  if (m_options.m_useSDL) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
      return -1;
    }
    //SDL_StartTextInput();
  }

  MainWindow mainWindow;

  if (options.m_verbose) {
    cerr << "Creating screen" << endl;
    cerr << "font size is " << (int)options.m_fontSize << endl;
  }

  CreateScreen(mainWindow);

  if (!Start()) {
    cerr << "error: cannot start emulator" << endl;
    return -1;
  }

  bool videoTest = false;
  options.m_args.GetValue("--videotest",    videoTest);

  if (videoTest) {
    if (m_memMapScreen) {
      const Config::MemoryMappedScreen * mmapInfo = GetMemoryMappedInfo();
      for (int i = 0; i < mmapInfo->m_screenCols * mmapInfo->m_screenRows; ++i) {
        m_memMapScreen->WriteMemoryAtAddress(i, i);
        m_memMapScreen->Update(true);
        double earliestNextRealTime_s; 
        int64_t earliestNextClockTime;
        RunPollers(earliestNextRealTime_s, earliestNextClockTime);
      }
      m_memMapScreen->Update(true);
    }
    else if (m_terminal) {
      for (int c = 0x20; c <= 0x3f; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n");
      for (int c = 0x40; c <= 0x5f; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n");
      for (int c = 0x60; c <= 0x7e; ++c)
        m_terminal->WriteChar(c);
      m_terminal->WriteString("\r\n\n");
      m_terminal->WriteString(m_info->m_title);
      m_terminal->WriteString("\r\n\n");
    }

    if (m_screen) {
      auto now = std::chrono::system_clock::now();
      auto finish = std::chrono::system_clock::now() + std::chrono::seconds(4);
      while (std::chrono::system_clock::now() < finish) {
        usleep(10000);
        m_screen->Update(true);
        double earliestNextRealTime_s; 
        int64_t earliestNextClockTime;
        RunPollers(earliestNextRealTime_s, earliestNextClockTime);
      }
    }
  }

  m_cycleCounter = 0;
  ArmPollers();

  if (m_options.m_verbose && options.m_turbo) {
    cerr << "turbo mode" << endl;
  }

  if (m_options.m_trace)
    SetTrace(true);

  // One millisecond of emulated time, and at least a few hundred cycles so a
  // slow CPU still does a useful batch per wait.
  const uint64_t quantumCycles = std::max<uint64_t>(
      500, (uint64_t)std::llround(m_targetCPUClock_Hz * 0.001));
  const uint64_t turboBatchCycles = 10000;

  using clock = std::chrono::steady_clock;
  auto origin = clock::now();
  const auto lagCap = std::chrono::milliseconds(50);
  double now = 0;

  for (;;) {
    now = SteadyNowSeconds();

    double earliestNextRealTime_s;
    int64_t earliestNextClockTime;
    RunPollers(earliestNextRealTime_s, earliestNextClockTime);

    uint64_t batchCap = options.m_turbo ? turboBatchCycles : quantumCycles;
    uint64_t cyclesToDo = batchCap;

    if (earliestNextClockTime > (int64_t)m_cycleCounter) {
      uint64_t untilClock = (uint64_t)earliestNextClockTime - m_cycleCounter;
      if (untilClock < cyclesToDo)
        cyclesToDo = untilClock;
    }

    double timeToDo_s = earliestNextRealTime_s - now;
    if (timeToDo_s > 0.0) {
      uint64_t cyclesForTime = (uint64_t)(timeToDo_s * m_targetCPUClock_Hz);
      if (cyclesForTime < cyclesToDo)
        cyclesToDo = cyclesForTime;
    }
    if (cyclesToDo < 1)
      cyclesToDo = 1;

    int remaining = Exec((int)cyclesToDo);
    int cyclesDone = (int)cyclesToDo - remaining;
    if (cyclesDone < 0)
      cyclesDone = 0;
    m_cycleCounter += (uint64_t)cyclesDone;

    if (options.m_turbo || m_targetCPUClock_Hz <= 0.0)
      continue;

    auto emulated = std::chrono::duration<double>(double(m_cycleCounter) / m_targetCPUClock_Hz);
    auto virtualNow = origin + std::chrono::duration_cast<clock::duration>(emulated);
    auto hostNow = clock::now();
    if (hostNow > virtualNow + lagCap) {
      origin = hostNow - std::chrono::duration_cast<clock::duration>(emulated);
    }
    else {
      WaitUntilVirtual(virtualNow);
    }
  }
}

void Emulator::RunPollers()
{
  double earliestNextRealTime_s; 
  int64_t earliestNextClockTime;
  RunPollers(earliestNextRealTime_s, earliestNextClockTime);
}

void Emulator::RunPollers(double & earliestNextRealTime_s, 
                          int64_t & earliestNextClockTime)
{
  double now = SteadyNowSeconds();
  earliestNextRealTime_s = now + 1.0;
  earliestNextClockTime  = m_cycleCounter + 1e+6;

  // run pollers
  for (auto & r : m_pollers.m_list) {
    PollDef & def = r.second;
    if (def.m_pollIsTime) {
      if (now >= def.m_nextTime) {
        def.Execute(now - def.m_lastTime, m_cycleCounter - def.m_lastClock);
        def.m_lastTime  = now;
        def.m_lastClock = m_cycleCounter;
        def.m_nextTime  = def.m_nextTime + def.m_timeInterval;
      }
      earliestNextRealTime_s = std::min<double>(earliestNextRealTime_s, def.m_nextTime);
    }
    else if (!def.m_pollIsTime) {
      if (m_cycleCounter >= def.m_nextClock) {
        def.Execute(now - def.m_lastTime, m_cycleCounter - def.m_lastClock);
        def.m_lastTime  = now;
        def.m_lastClock = m_cycleCounter;
        def.m_nextClock = def.m_nextClock + def.m_clockInterval;
      }
      earliestNextClockTime = std::min<uint64_t>(earliestNextClockTime, def.m_nextClock);
    }
  }

  if (m_resetRequested) {
    m_resetRequested = false;
    Reset();
    ArmPollers();
  }
}


int Emulator::AddRealTimePollDef(double seconds, PollHandler handler)
{
  return m_pollers.Add(seconds, handler);
}

int Emulator::AddCPUTimePollDef(uint64_t cycles, PollHandler handler)
{
  return m_pollers.Add(cycles, handler);
}

void Emulator::CalcCPUSpeed(double secs, uint64_t clocks)
{
  if (secs > 0) {
    m_actualCPUClock_Hz = 1.0 * clocks / secs;
    //cout << "secs " << secs << ", clocks " << clocks << endl;
    if (m_options.m_displayCPUSpeed)
      cout << std::fixed << std::setprecision(3) 
           << (m_actualCPUClock_Hz / 1e+6) 
           << " MHz " 
           << (m_options.m_turbo ? "(turbo)" : "(limited)") << "\n";
  }
}

bool Emulator::LoadFile()
{
  nfd_OpenDialogExt extInfo;
  memset(&extInfo, 0, sizeof(extInfo));
  extInfo.filterList      = "grz;hex";
  extInfo.title           = "Open file";

  nfdchar_t * path = NULL;
  if (NFD_OpenDialogExt(&extInfo, &path) != NFD_OKAY)
    return false;

  return LoadFile(path);
}


bool Emulator::LoadFile(const std::string & path)
{
  BINFileIdentifier bin;
  BINFile * file = bin.Open(path, false);

  if (file == nullptr)
    return false;

  using namespace std::placeholders;

  bool loaded = file->Load(std::bind(&Emulator::SaveBlock, this, _1, _2, _3));
 
  uint16_t execAddr;
  if (loaded && file->GetExecAddr(execAddr)) {
    if (m_options.m_verbose)
      cerr << "info: setting PC to " << HEXFORMAT0x4(execAddr) << endl;
    SetPC(execAddr);    
  }

  delete file;
  return loaded;
 }

bool Emulator::SaveBlock(unsigned addr, const uint8_t * data, unsigned len)
{
  for (unsigned i = 0; i < len; ++i) {
    WriteMemory(addr++, data[i]);
  }
  return true;
}

uint8_t * Emulator::GetMainMemoryPtr()
{
  for (auto & r : m_writeMemoryBlocks) {
    if (r.m_type == Config::Type::eMainRAM)
      return r.m_memory;
  }

  return nullptr;
}

void Emulator::AddTraceInfo(unsigned addr)
{
  if (m_options.m_traceLength == 0) {
    return;
  }

  m_traceQueue.push_front(addr);
  if (m_traceQueue.size() > m_options.m_traceLength) {
    m_traceQueue.pop_back();
  }
}

void Emulator::DisplayTraceInfo()
{
  if (m_traceQueue.size() == 0)
    return;

  cout << "Backtrace of " << m_traceQueue.size() << " entries" << endl;  
  for (int i = m_traceQueue.size()-1; i >= 0; --i) {
    cout << Disassemble(m_traceQueue[i]) << endl;
  }
  getchar();
}

std::string Emulator::Disassemble(unsigned addr) const
{
  stringstream strm;
  strm << HEXFORMAT0x4(addr);
  std::vector<uint8_t> opcodes;
  GetOpcode(addr, opcodes);
  for (auto & r : opcodes) {
    strm << " " << HEXFORMAT2(r);
  }
  strm << std::string((5-opcodes.size())*3, ' ');
  strm << DecodeOpcode(opcodes);

  return strm.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
