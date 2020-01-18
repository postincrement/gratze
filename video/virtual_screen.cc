
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "src/misc.h"
#include "src/config.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"
#include "src/emulator.h"

using namespace std;

VirtualScreenFactory VirtualScreen::g_virtualScreenFactory;

/////////////////////////////////////////////////////////////////////////////////

TextWindow::TextWindow(MainWindow & mainWindow, int rows, int cols, int width, int height)
  : m_mainWindow(mainWindow)
  , m_rows(rows)
  , m_cols(cols)
  , m_width(width)
  , m_height(height)
  , m_colScale(1)
{
  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  m_visibleSize = rows * cols;
  m_visibleMask = m_visibleSize - 1;
  cout << "info: text window is " << m_width << " x " << m_height << " pixels, " << m_visibleSize << " chars, mask is " << HEXFORMAT0x4(m_visibleMask) << endl;
}

bool TextWindow::Open()
{
  return true;
}

void TextWindow::SetColScale(int scale)
{
  m_colScale = scale;
}

void TextWindow::RenderCharAtPos(int x, int y, bool withCursor)
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

void TextWindow::RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_font->RenderChar(ch, renderer, dstRect, fg, bg);
}

void TextWindow::Update(bool hasChanged)
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

void TextWindow::RefreshScreen()
{
  for (int y = 0; y < m_rows; ++y)
    for (int x = 0; x < m_cols / m_colScale; x++)
      RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY));
}

void TextWindow::EnableCursor(bool enable)
{
  if (enable == m_cursorEnabled)
    return;

  m_cursorEnabled = enable;
  RenderCharAtPos(m_cursorX, m_cursorY, m_cursorEnabled);
}

void TextWindow::SetCursorPos(int x, int y)
{ 
  if ((x == m_cursorX) && (y == m_cursorY))
    return;

  if (m_cursorEnabled)
    RenderCharAtPos(m_cursorX, m_cursorY, false);

  m_cursorX = x;
  m_cursorY = y;
  
  if (m_cursorEnabled)
    RenderCharAtPos(m_cursorX, m_cursorY, true);
}

bool TextWindow::SetFont(Font * font)
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

  cout << "info: font set = " << m_font->GetWidth() << "x" << m_font->GetHeight() << endl;

  ResizeScreen();

  return true;
}

bool TextWindow::ResizeScreen()
{
  int newWidth  = m_cols * m_font->GetWidth();
  int newHeight = m_rows * m_font->GetHeight();

  cout << "info: screen resize requested old: " << m_width << "x" << m_height << ", new: " << newWidth << "x" << newHeight << endl; 

  if ((newHeight == m_height) && (newWidth == m_width))
    return false;

  m_width  = newWidth;
  m_height = newHeight;
  
  m_mainWindow.Open(newWidth, newHeight);
  m_font->Open(m_mainWindow.GetRenderer());

  return true;
}

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen * VirtualScreen::Create(MainWindow & mainWindow, 
                                        Emulator & emulator, 
                                   const Options & options, 
                             const Config::Video & info)
{
  VirtualScreen * screen = g_virtualScreenFactory.CreateInstance(info.m_name, mainWindow, emulator, options, info);
  return screen;
}  

VirtualScreen::VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : TextWindow(mainWindow, info.m_screenRows, info.m_screenCols, info.m_screenWidth, info.m_screenHeight)
{
  m_hscale = 1;
  m_vscale = 1;
}

void VirtualScreen::SetScale(int hscale, int vscale)
{
  m_hscale = hscale;
  m_vscale = vscale;

  cout << "info: screen scale is " << hscale << "," << vscale << endl;
}

FontChar VirtualScreen::GetCharAtPos(int x, int y)
{
  int addr = MapPosToAddress(x, y);
  return GetCharAtAddress(addr);
}

void VirtualScreen::GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg)
{
  int addr = MapPosToAddress(x, y);
  return GetColourAtAddress(addr, fg, bg);
}

void VirtualScreen::RenderCharAtAddress(int addr)
{
  int x, y;
  if (MapAddressToPos(x, y, addr))
    RenderCharAtPos(x, y, m_cursorEnabled && (x == m_cursorX) && (y == m_cursorY));
}

VirtualScreen::~VirtualScreen()
{
}

/////////////////////////////////////////////////////////////////////////////////

MemoryMappedVideo::MemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : VirtualScreen(mainWindow, emulator, options, info)
  , m_offset(0)
  , m_fontConfig(info.m_font)
  , m_options(options)
{
  // create video memory
  int size_bytes = info.m_endAddr - info.m_startAddr + 1;
  m_memory.resize(size_bytes);
  m_memoryMask = m_memory.size() - 1;

  cout << "info: video memory size is " << size_bytes << " bytes, mask is " << HEXFORMAT0x4(m_memoryMask) << endl;
}

MemoryMappedVideo::~MemoryMappedVideo()
{}


bool MemoryMappedVideo::Open()
{
  if (!m_options.m_font.empty()) {
    SetFont(new TTFFont(m_fontConfig, m_options.m_font, 128));
  }
  else {
    SetFont(new PixelFont(m_fontConfig));
  }

  return VirtualScreen::Open();
}

int MemoryMappedVideo::MapPosToAddress(int x, int y)
{
  return y * m_cols + (x * m_colScale);
}

bool MemoryMappedVideo::MapAddressToPos(int & x, int & y, int addr)
{
  if (addr >= m_visibleSize)
    return false;

  x = addr % m_cols;
  if (m_colScale != 1) {
    if ((x % m_colScale) != 0)
      return false;
    x /= m_colScale;  
  }

  y = addr / m_cols;

  return true;
}

FontChar MemoryMappedVideo::GetCharAtAddress(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0x00;
  }

  return m_memory[addr];
}

void MemoryMappedVideo::WriteMemoryAtAddress(int addr, uint8_t ch)
{
  if ((addr >= m_memory.size()) || (m_memory[addr] == ch)) {
    return;
  }

  m_memory[addr] = ch;
  RenderCharAtAddress(addr);
}

uint8_t MemoryMappedVideo::ReadMemoryAtAddress(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0x00;
  }

  return m_memory[addr];
}

/////////////////////////////////////////////////////////////////////////////////

SingleColourMemoryMappedVideo::SingleColourMemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : MemoryMappedVideo(mainWindow, emulator, options, info)
{
  m_fgColour = { 0xff, 0xff, 0xff, 0xff };
  m_bgColour = { 0, 0, 0, 0 };
}

void SingleColourMemoryMappedVideo::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  RefreshScreen();
}

void SingleColourMemoryMappedVideo::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{
  fg = m_fgColour;
  bg = m_bgColour;
}

void SingleColourMemoryMappedVideo::GetColourAtAddress(int addr, SDL_Colour & fg, SDL_Colour & bg)
{
  return GetFontColour(fg, bg);  
}

