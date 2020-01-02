
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "config.h"
#include "virtual_screen.h"
#include "mainwindow.h"
#include "cpu/emulator.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options)
  : m_mainWindow(mainWindow)
{
  // clear video memory
  cout << "info: video memory size is " << emulator.GetInfo().m_video.m_memorySize_k << "k" << endl;
  m_memory.resize(emulator.GetInfo().m_video.m_memorySize_k * 1024);
  memset(&m_memory[0], 0x20, m_memory.size());
  m_offsMask = m_memory.size() - 1;

  m_cols   = emulator.GetInfo().m_video.m_screenCols;
  m_rows   = emulator.GetInfo().m_video.m_screenRows;
  m_width  = emulator.GetInfo().m_video.m_screenWidth;
  m_height = emulator.GetInfo().m_video.m_screenHeight;

  m_start = 0;

  m_fgColour = { 255, 255, 255 };
  m_bgColour = {   0,   0,   0 };

  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  cout << "info: virtual screen is " << m_width << " x " << m_height << endl;
}

VirtualScreen::~VirtualScreen()
{}

/*
void VirtualScreen::SetColours(const SDL_Color & fg, const SDL_Color & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  Refresh();
}
*/

bool VirtualScreen::SetFont(Font * font)
{
  m_font.reset(font);
  if (!SetFontColour(m_fgColour, m_bgColour))
    return false;

  cout << "info: virtual screen font is " << m_font->GetWidth() << " x " << m_font->GetHeight() << endl;
  return true;
}

bool VirtualScreen::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;
  if (!m_font || !m_font->Open(m_mainWindow.GetRenderer(), m_fgColour, m_bgColour)) {
    cerr << "error: could not create font" << endl;
    return false;
  }
  Refresh();
  return true;
}

void VirtualScreen::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{
  fg = m_fgColour;
  bg = m_bgColour;
}

void VirtualScreen::WriteChar(int offs, uint8_t ch)
{
  if (ch != ReadChar(offs)) {
    m_memory[offs & m_offsMask] = ch;
    RenderChar(offs);
  }
}

uint8_t VirtualScreen::ReadChar(int offs) const
{
  return m_memory[offs & m_offsMask];
}

void VirtualScreen::RenderChar(int offs)
{ 
  if (m_font) {
    offs &= m_offsMask;

    SDL_Rect dstRect;
    int x = (offs - m_start) % m_cols;
    int y = (offs - m_start) / m_cols;
    m_mainWindow.GetScreenCharRect(dstRect, x * m_font->GetWidth(), y * m_font->GetHeight(), 
                                                m_font->GetWidth(),     m_font->GetHeight());

    int fontChar = GetFontChar(offs);
    m_font->RenderChar(fontChar, m_mainWindow.GetRenderer(), dstRect);

    Update(true);
  }
}

int VirtualScreen::GetFontChar(int offs)
{
  return ReadChar(offs);
}

void VirtualScreen::Refresh()
{
  for (int offs = 0; offs < (m_width * m_height); ++offs)
    RenderChar(offs);
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
