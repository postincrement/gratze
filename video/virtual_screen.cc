
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "src/misc.h"
#include "src/config.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"

using namespace std;

MemoryMappedScreenFactory MemoryMappedScreen::g_memoryMappedScreenFactory;

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(MainWindow & mainWindow, const Options & options, int cols, int rows)
  : m_mainWindow(mainWindow)
  , m_options(options)
  , m_cols(cols)
  , m_rows(rows)
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

bool VirtualScreen::Open()
{
  return true;
}

void VirtualScreen::RenderCharAtPos(int x, int y, bool withCursor)
{ 
  if (m_font) {
    SDL_Rect dstRect;
    m_mainWindow.GetScreenCharRect(dstRect, 
                                   (x * m_font->GetWidth()/* * m_colScale*/), 
                                    y * m_font->GetHeight(), 
                                    m_font->GetWidth()/* * m_colScale*/,
                                    m_font->GetHeight(),
                                    m_hscale * m_colScale, 
                                    m_vscale);

    SDL_Colour fg, bg;
    GetColourAtPos(x, y, fg, bg);

    SDL_RenderSetScale(m_mainWindow.GetRenderer(), m_hscale * m_colScale, m_vscale);

    RenderChar(GetCharAtPos(x, y), withCursor, m_mainWindow.GetRenderer(), dstRect, fg, bg);

    Update(true);
  }
}

void VirtualScreen::RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  //cout << "render char " << HEXFORMAT0x2(ch) << " " << (isgraph(ch) ? (char)ch : '.') << " cursor = " << withCursor << endl;
  if (withCursor)
    m_font->RenderChar(ch, renderer, dstRect, bg, fg);
  else  
    m_font->RenderChar(ch, renderer, dstRect, fg, bg);
}

void VirtualScreen::Update(bool hasChanged)
{
  if (!m_lazyUpdates) {
    if (!hasChanged || m_dirty)
      m_mainWindow.Update();
    m_dirty = false;  
    return;
  }

  auto now = std::chrono::system_clock::now();
  if (!m_dirty && hasChanged) {
    m_updateTimer = now + std::chrono::milliseconds(LAZY_UPDATE_MSECS);
    m_dirty = true;
  }

  if (m_dirty && (now > m_updateTimer)) {
    m_mainWindow.Update();
    m_dirty = false;
  }
}

void VirtualScreen::RefreshScreen()
{
  for (int y = 0; y < m_rows; ++y)
    for (int x = 0; x < m_cols / m_colScale; x++)
      RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY));
}

void VirtualScreen::EnableCursor(bool enable)
{
  if (enable == m_cursorEnabled)
    return;

  m_cursorEnabled = enable;
  RenderCharAtPos(m_cursorX, m_cursorY, m_cursorEnabled);
}

void VirtualScreen::SetCursorPos(int x, int y)
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

bool VirtualScreen::SetFont(Font * font)
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

  ResizeScreen();

  return true;
}

bool VirtualScreen::ResizeScreen()
{
  int newWidth  = m_cols * m_font->GetWidth();
  int newHeight = m_rows * m_font->GetHeight();

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
  
  m_mainWindow.Open(newWidth, newHeight);
  m_font->Open(m_mainWindow.GetRenderer());

  return true;
}

void VirtualScreen::SetScale(int hscale, int vscale)
{
  m_hscale = hscale;
  m_vscale = vscale;

  cout << "info: screen scale is " << hscale << "," << vscale << endl;
}

void VirtualScreen::SetColScale(int scale)
{
  m_colScale = scale;
}

int VirtualScreen::MapPosToLoc(int x, int y)
{
  return y * m_cols + (x * m_colScale);
}

bool VirtualScreen::MapLocToPos(int & x, int & y, int loc)
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

FontChar VirtualScreen::GetCharAtPos(int x, int y)
{
  int addr = MapPosToLoc(x, y);
  return GetCharAtLoc(addr);
}

void VirtualScreen::GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg)
{
  int loc = MapPosToLoc(x, y);
  return GetColourAtLoc(loc, fg, bg);
}


void VirtualScreen::RefreshCharAtLoc(int loc)
{
  int x, y;
  if (MapLocToPos(x, y, loc))
    RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY));
}

FontChar VirtualScreen::GetCharAtLoc(int loc) const
{
  cerr << "screen: GetCharAtLoc or GetCharAtPos not defined" << endl;
  return 0;
}

void VirtualScreen::GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg)
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
  : VirtualScreen(mainWindow, options, info.m_screenCols, info.m_screenRows)
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


void MemoryMappedScreen::WriteMemoryAtAddress(int addr, uint8_t data)
{
  // make sure address is in correct range
  if (addr >= m_memory.size()) {
    return;
  }

  // put data into memory
  m_memory[addr] = data;

  // calculate location using offset
  int loc = addr - m_offset;

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

/////////////////////////////////////////////////////////////////////////////////

SingleColourMemoryMappedScreen::SingleColourMemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info)
  : MemoryMappedScreen(mainWindow, options, info)
{
  m_fgColour = { 0xff, 0xff, 0xff, 0xff };
  m_bgColour = { 0, 0, 0, 0 };
}

void SingleColourMemoryMappedScreen::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  RefreshScreen();
}

void SingleColourMemoryMappedScreen::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{
  fg = m_fgColour;
  bg = m_bgColour;
}

void SingleColourMemoryMappedScreen::GetColourAtLoc(int loc, SDL_Colour & fg, SDL_Colour & bg)
{
  return GetFontColour(fg, bg);  
}

