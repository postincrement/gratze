
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "misc.h"
#include "config.h"
#include "virtual_screen.h"
#include "mainwindow.h"
#include "cpu/emulator.h"

using namespace std;

VirtualScreenFactory VirtualScreen::g_virtualScreenFactory;

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen * VirtualScreen::Create(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
{
  return g_virtualScreenFactory.CreateInstance(info.m_name, mainWindow, emulator, options, info);
}  

/////////////////////////////////////////////////////////////////////////////////

TextWindow::TextWindow(MainWindow & mainWindow, int rows, int cols, int width, int height)
  : m_mainWindow(mainWindow)
  , m_rows(rows)
  , m_cols(cols)
  , m_width(width)
  , m_height(height)
{
  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  m_visibleSize = rows * cols;
  m_visibleMask = m_visibleSize - 1;
  cout << "info: text window is " << m_width << " x " << m_height << " pixels, " << m_visibleSize << " chars, mask is " << HEXFORMAT0x4(m_visibleMask) << endl;
}

void TextWindow::RenderChar(int pos, SDL_Colour & fg, SDL_Colour & bg)
{ 
  if (m_font) {
    pos = (pos & m_visibleMask);
    SDL_Rect dstRect;
    int x = pos % m_cols;
    int y = pos / m_cols;
    m_mainWindow.GetScreenCharRect(dstRect, x * m_font->GetWidth(), y * m_font->GetHeight(), 
                                                m_font->GetWidth(),     m_font->GetHeight());

    m_font->RenderChar(GetCharAtPos(pos), m_mainWindow.GetRenderer(), dstRect, fg, bg);

    Update(true);
  }
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

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : TextWindow(mainWindow, info.m_screenRows, info.m_screenCols, info.m_screenWidth, info.m_screenHeight)
{
}

VirtualScreen::~VirtualScreen()
{
}

/////////////////////////////////////////////////////////////////////////////////

MemoryMappedVideo::MemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : VirtualScreen(mainWindow, emulator, options, info)
  , m_offset(0)
{
  // create video memory
  int size_bytes = info.m_endAddr - info.m_startAddr + 1;
  m_memory.resize(size_bytes);
  m_memoryMask = m_memory.size() - 1;

  cout << "info: video memory size is " << size_bytes << " bytes, mask is " << HEXFORMAT0x4(m_memoryMask) << endl;

  // create font
  if (info.m_font.m_creator != NULL) {
    (info.m_font.m_creator)(options, info.m_font, m_fontData);
    m_font.reset(new PixelFont(info.m_font.m_count, info.m_font.m_width, info.m_font.m_height, &m_fontData[0]));
    if (!m_font) {
      cerr << "error: font could not be opened" << endl;
    }
    if (mainWindow.GetRenderer() == nullptr) {
      cerr << "error: renderer not available" << endl;
    }
    else if (!m_font->Open(mainWindow.GetRenderer())) {
      cerr << "error: font could not be opened" << endl;
    }
    else {
      cout << "info: pixel font created" << endl;
    }
  }
}

void MemoryMappedVideo::WriteMemory(int addr, uint8_t ch)
{
  if ((addr >= m_memory.size()) || (m_memory[addr] == ch)) {
    return;
  }

  m_memory[addr] = ch;
  RefreshChar(addr);
}

uint8_t MemoryMappedVideo::ReadMemory(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0x00;
  }

  return m_memory[addr];
}

void MemoryMappedVideo::RefreshScreen()
{
  for (int offs = 0; offs < m_visibleSize; ++offs)
    RefreshChar(offs);
}

FontChar MemoryMappedVideo::GetCharAtPos(int addr)
{
  return m_memory[addr & m_visibleMask];
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

void SingleColourMemoryMappedVideo::RefreshChar(int pos)
{
  pos = pos & m_visibleMask;
  RenderChar(pos, m_fgColour, m_bgColour);
}
