
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "common/misc.h"
#include "src/config.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"

using namespace std;

MemoryMappedScreenFactory MemoryMappedScreen::g_memoryMappedScreenFactory;

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(const Options & options, int cols, int rows)
  : m_options(options)
  , m_cols(cols)
  , m_rows(rows)
{
}

VirtualScreen::~VirtualScreen()
{}

bool VirtualScreen::Open()
{
  return true;
}

int VirtualScreen::GetRows() const
{
  return m_rows;
}

int VirtualScreen::GetCols() const
{
  return m_cols;
}

/////////////////////////////////////////////////////////////////////////////////

SDLVirtualScreen::SDLVirtualScreen(MainWindow & mainWindow, const Options & options, int cols, int rows)
  : VirtualScreen(options, cols, rows)
  , m_mainWindow(mainWindow)
{
  m_hscale   = 1;
  m_vscale   = 1;
  m_colScale = 1;

  // this will be set when the font is set
  m_width = 0;
  m_height = 0;

  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  m_visibleSize = m_rows * m_cols;
  m_visibleMask = m_visibleSize - 1;
  cout << "info: text window is " << m_cols << " x " << m_rows << " chars, " << m_visibleSize << " chars total, mask is " << HEXFORMAT0x4(m_visibleMask) << endl;
}

SDLVirtualScreen::~SDLVirtualScreen()
{}

void SDLVirtualScreen::RenderCharAtPos(int x, int y, bool withCursor, bool update)
{
  if (m_font) {
    SDL_Rect dstRect;
    m_mainWindow.GetScreenCharRect(dstRect,
                                   (x * m_font->GetWidth()),
                                    y * m_font->GetHeight(),
                                    m_font->GetWidth(),
                                    m_font->GetHeight(),
                                    m_hscale * m_colScale,
                                    m_vscale);

    SDL_Colour fg, bg;
    GetColourAtPos(x, y, fg, bg);

    SDL_RenderSetScale(m_mainWindow.GetRenderer(), m_hscale * m_colScale, m_vscale);

    RenderChar(GetCharAtPos(x, y), withCursor, m_mainWindow.GetRenderer(), dstRect, fg, bg);

    if (update)
      Update(true);
  }
}

void SDLVirtualScreen::RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  //cout << "render char " << HEXFORMAT0x2(ch) << " " << (isgraph(ch) ? (char)ch : '.') << " cursor = " << withCursor << endl;
  if (withCursor)
    m_font->RenderChar(ch, renderer, dstRect, bg, fg);
  else
    m_font->RenderChar(ch, renderer, dstRect, fg, bg);
}

void SDLVirtualScreen::Update(bool hasChanged)
{
  if (!m_lazyUpdates) {
    if (!hasChanged || m_dirty)
      OnUpdate();
    m_dirty = false;
    return;
  }

  auto now = std::chrono::system_clock::now();
  if (!m_dirty && hasChanged) {
    m_updateTimer = now + std::chrono::milliseconds(LAZY_UPDATE_MSECS);
    m_dirty = true;
  }

  if (m_dirty && (now > m_updateTimer)) {
    OnUpdate();
    m_dirty = false;
  }
}

void SDLVirtualScreen::OnUpdate()
{
  m_mainWindow.Update();
}

void SDLVirtualScreen::RefreshScreen()
{
  for (int y = 0; y < m_rows; ++y)
    for (int x = 0; x < m_cols / m_colScale; x++)
      RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY));
}

void SDLVirtualScreen::EnableCursor(bool enable)
{
  if (enable == m_cursorEnabled)
    return;

  m_cursorEnabled = enable;
  RenderCharAtPos(m_cursorX, m_cursorY, m_cursorEnabled);
}

void SDLVirtualScreen::SetCursorPos(int x, int y)
{
//  if ((x == m_cursorX) && (y == m_cursorY))
//    return;

  if (m_cursorEnabled)
    RenderCharAtPos(m_cursorX, m_cursorY, false);

  m_cursorX = x;
  m_cursorY = y;

  if (m_cursorEnabled)
    RenderCharAtPos(m_cursorX, m_cursorY, true);
}

bool SDLVirtualScreen::SetFont(Font * font, int cols, int rows)
{
  m_font.reset(font);

  if (!m_font) {
    cerr << "error: font could not be opened" << endl;
    return false;
  }

  if (m_mainWindow.GetRenderer() == nullptr) {
    cerr << "error: renderer not available" << endl;
    return false;
  }

  if (!m_font->Open(m_mainWindow.GetRenderer())) {
    cerr << "error: font could not be opened" << endl;
    return false;
  }

  cout << "info: font size = " << m_font->GetWidth() << "x" << m_font->GetHeight() << endl;

  if ((cols > 0) && (rows > 0)) {
    m_cols = cols;
    m_rows = rows;
    cout << "info: new screen size = " << m_cols << "x" << m_rows << endl;
  }

  ResizeScreen();

  return true;
}

bool SDLVirtualScreen::ResizeScreen()
{
  int newWidth  = m_cols * m_font->GetWidth() * m_hscale;
  int newHeight = m_rows * m_font->GetHeight() * m_vscale;

  if ((m_width == 0) || (m_height == 0)) {
    cout << "info: screen size set to " << newWidth << "x" << newHeight << endl;
  }
  else {
    if ((newHeight == m_height) && (newWidth == m_width))
      return false;
    cout << "info: screen resize requested old: " << m_width << "x" << m_height << ", new: " << newWidth << "x" << newHeight << endl;
  }

  m_width  = newWidth;
  m_height = newHeight;

  m_visibleSize = m_rows * m_cols;
  m_visibleMask = m_visibleSize - 1;
  cout << "info: text window is " << m_cols << " x " << m_rows << " chars, " << m_visibleSize << " chars total, mask is " << HEXFORMAT0x4(m_visibleMask) << endl;

  m_mainWindow.Open(newWidth, newHeight);
  m_font->Open(m_mainWindow.GetRenderer());

  return true;
}

void SDLVirtualScreen::SetScale(int hscale, int vscale)
{
  m_hscale = hscale;
  m_vscale = vscale;

  cout << "info: screen scale is " << hscale << "," << vscale << endl;
}

void SDLVirtualScreen::SetColScale(int scale)
{
  m_colScale = scale;
}

int SDLVirtualScreen::MapPosToLoc(int x, int y)
{
  return y * m_cols + (x * m_colScale);
}

bool SDLVirtualScreen::MapLocToPos(int & x, int & y, int loc)
{
  if (loc >= m_visibleSize)
    return false;

  x = loc % m_cols;
  if (m_colScale != 1) {
    if ((x % m_colScale) != 0)
      return false;
    x /= m_colScale;
  }

  y = loc / m_cols;

  return true;
}

FontChar SDLVirtualScreen::GetCharAtPos(int x, int y)
{
  int addr = MapPosToLoc(x, y);
  return GetCharAtLoc(addr);
}

void SDLVirtualScreen::GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg)
{
  int loc = MapPosToLoc(x, y);
  return GetColourAtLoc(loc, fg, bg);
}


void SDLVirtualScreen::RefreshCharAtLoc(int loc, bool update)
{
  int x, y;
  if (MapLocToPos(x, y, loc))
    RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY), update);
}

FontChar SDLVirtualScreen::GetCharAtLoc(int loc) const
{
  cerr << "screen: GetCharAtLoc or GetCharAtPos not defined" << endl;
  return 0;
}

void SDLVirtualScreen::GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg)
{
  cerr << "screen: GetColourAtLoc or GetColourAtPos not defined" << endl;
}

/////////////////////////////////////////////////////////////////////////////////

MemoryMappedScreen * MemoryMappedScreen::Create(MainWindow & mainWindow,
                                             const Options & options,
                                       const Config::MemoryMappedScreen & info)
{
  MemoryMappedScreen * screen = g_memoryMappedScreenFactory.CreateInstance(info.m_name, mainWindow, options, info);
  return screen;
}

MemoryMappedScreen::MemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : SDLVirtualScreen(mainWindow, options, info.m_screenCols, info.m_screenRows)
  , m_offset(0)
{
  m_width    = info.m_screenWidth;
  m_height   = info.m_screenHeight;

  // create video memory
  int size_bytes = info.m_endAddr - info.m_startAddr + 1;
  m_memory.resize(size_bytes);
  m_memoryMask = m_memory.size() - 1;

  cout << "info: video memory size is " << size_bytes << " bytes, mask is " << HEXFORMAT0x4(m_memoryMask) << endl;
}

MemoryMappedScreen::~MemoryMappedScreen()
{}

void MemoryMappedScreen::SetOffset(uint16_t offset)
{
  m_offset = offset;
  RefreshScreen();  
}

void MemoryMappedScreen::WriteMemoryAtAddress(int addr, uint8_t data)
{
  // make sure address is in correct range
  if (addr >= m_memory.size()) {
    return;
  }

  // calculate location using offset
  int loc = addr - m_offset;

  // get current character
  if (!m_hasPCG) {
    m_memory[addr] = data;
  }
  else {
    FontChar oldChar = GetCharAtLoc(loc);
    m_memory[addr] = data;
    UpdatePCG(loc, oldChar, GetCharAtLoc(loc));
  }

  // redraw the new character
  RefreshCharAtLoc(loc);
}

uint8_t MemoryMappedScreen::ReadMemoryAtAddress(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0x00;
  }

  return m_memory[addr];
}

FontChar MemoryMappedScreen::GetCharAtLoc(int loc) const
{
  return m_memory[loc & m_memoryMask];
}

bool MemoryMappedScreen::IsPCG(FontChar ch) const
{
  return false;
}

void MemoryMappedScreen::OnUpdate()
{
  if (m_hasPCG && (m_pcgDirty.size() > 0)) {
    for (auto & ch : m_pcgDirty) {
      if (IsPCG(ch)) {
        auto & s = m_pcgLocs[ch];
        for (auto & loc : s.m_locs) {
          RefreshCharAtLoc(loc, false);
        }
      }
    }
    m_pcgDirty.clear();
  }

  SDLVirtualScreen::OnUpdate();
}

/////////////////////////////////////////////////////////////////////////////////

void MemoryMappedScreen::InitPCG()
{
  m_hasPCG = true;
  m_pcgLocs.clear();
  for (int loc = 0;loc < m_memory.size(); ++loc) {
    FontChar ch = GetCharAtLoc(loc);
    auto r = m_pcgLocs.find(ch);
    if (r != m_pcgLocs.end()) {
      r->second.m_locs.insert(loc);
    }
    else {
      CharUsage u;
      u.m_locs.insert(loc);
      m_pcgLocs[ch] = u;
    }
  }
}

void MemoryMappedScreen::UpdatePCG(int loc, FontChar oldChar, FontChar newChar)
{
  auto r = m_pcgLocs.find(oldChar);
  if (r == m_pcgLocs.end()) {
    cerr << "mmap: char " << HEXFORMAT0x4(oldChar) << " not in used map" << endl;
  }
  else {
    r->second.m_locs.erase(loc);
  }

  r = m_pcgLocs.find(newChar);
  if (r != m_pcgLocs.end()) {
    r->second.m_locs.insert(loc);
  }
  else {
    CharUsage u;
    u.m_locs.insert(loc);
    m_pcgLocs[newChar] = u;
  }
}

void MemoryMappedScreen::SetPCG(FontChar ch, int line, uint8_t val)
{
  SetPCG(ch, line, 1, &val);
}

void MemoryMappedScreen::SetPCG(FontChar ch, int row, int rowCount, uint8_t * ptr)
{
  if (!m_hasPCG || !IsPCG(ch))
    return;

  m_font->Modify(ch, row, rowCount, ptr);
  m_pcgDirty.insert(ch);
}


bool MemoryMappedScreen::SetFont(Font * font, int cols, int rows)
{
  if (!SDLVirtualScreen::SetFont(font, cols, rows)) 
    return false;

  RewritePCG();

  return true;
}

void MemoryMappedScreen::RewritePCG()
{
  if (m_hasPCG && m_rewritePCGHandler)
    m_rewritePCGHandler();
}

void MemoryMappedScreen::SetRewritePCGHandler(std::function<void ()> handler)
{
  m_rewritePCGHandler = handler;
}

/////////////////////////////////////////////////////////////////////////////////

MonoMemoryMappedScreen::MonoMemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : MemoryMappedScreen(mainWindow, options, info)
{
  m_fgColour = { 0xff, 0xff, 0xff, 0xff };
  m_bgColour = { 0, 0, 0, 0 };
}

void MonoMemoryMappedScreen::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  RefreshScreen();
}

void MonoMemoryMappedScreen::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{
  fg = m_fgColour;
  bg = m_bgColour;
}

void MonoMemoryMappedScreen::GetColourAtLoc(int loc, SDL_Colour & fg, SDL_Colour & bg)
{
  return GetFontColour(fg, bg);
}

/////////////////////////////////////////////////////////////////////////////////

ColourMemoryMappedScreen::ColourMemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : MemoryMappedScreen(mainWindow, options, info)
{}

void ColourMemoryMappedScreen::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{}

void ColourMemoryMappedScreen::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{}

