
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "virtual_screen.h"
#include "mainwindow.h"
#include "SDL_FontCache/SDL_FontCache.h"

using namespace std;

PixelFont::PixelFont(int charCount, int width, int height, uint8_t * data)
  : m_charCount(charCount)
  , m_width(width)
  , m_height(height)
  , m_data(data)
{}

SDL_Texture * PixelFont::GetTexture(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg)
{
  // create the raw surface
  SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0, m_width, m_height * m_charCount, 0, SDL_PIXELFORMAT_RGB24);
  if (surface == NULL) {
    SDL_Log("SDL_CreateRGBSurfaceWithFormat() for font failed: %s", SDL_GetError());
    exit(1);
  }

  // lock the surface
  SDL_LockSurface(surface);

  // copy pixel data to the with the correct colours
  for (int i = 0; i < m_charCount; ++i) {
    uint8_t * srcPixels = m_data + (i * m_height);
    for (int y = 0; y < m_height; ++y) {
      uint8_t * dstPixels = (uint8_t *)surface->pixels + ((i * m_height) + y) * surface->pitch;
      unsigned mask = 1;
      for (int x = 0; x < m_width; ++x) {
        if (*srcPixels & mask) {
          dstPixels[0] = fg.r; 
          dstPixels[1] = fg.g; 
          dstPixels[2] = fg.b; 
        }
        else {
          dstPixels[0] = bg.r; 
          dstPixels[1] = bg.g; 
          dstPixels[2] = bg.b; 
        }  
        dstPixels += 3;
        mask = mask << 1;
      }
      ++srcPixels;
    }
  }

  SDL_UnlockSurface(surface);

  // create texture
  SDL_Texture * texture = SDL_CreateTextureFromSurface(m_renderer, surface);
  SDL_FreeSurface(surface);

  return texture;
}

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(MainWindow & mainWindow, const Options & options, int width, int height, int dataLen)
  : m_mainWindow(mainWindow)
  , m_width(width)
  , m_height(height)
{
  m_memory.resize(dataLen);
  memset(&m_memory[0], 0x20, dataLen);

  m_offsMask = dataLen - 1;
  m_start = 0;

  m_fgColour = { 255, 255, 255 };
  m_bgColour = {   0,   0,   0 };

  m_fontTexture = nullptr;

  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  cout << "info: virtual screen is " << width << " x " << height << endl;
}

VirtualScreen::~VirtualScreen()
{}

void VirtualScreen::SetColors(const SDL_Color & fg, const SDL_Color & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  Refresh();
}

void VirtualScreen::SetFont(PixelFont * font)
{
  m_font.reset(font);
  m_fontTexture = m_font ? m_font->GetTexture(m_mainWindow.GetRenderer(), m_fgColour, m_bgColour) : nullptr;
  cout << "info: virtual screen font is " << m_font->GetWidth() << " x " << m_font->GetHeight() << endl;
  m_cols = m_width  / m_font->GetWidth();
  m_rows = m_height / m_font->GetHeight();
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
    SDL_Rect srcRect = { 0, fontChar * m_font->GetHeight(), m_font->GetWidth(), m_font->GetHeight() };
    SDL_RenderCopy(m_mainWindow.GetRenderer(), m_fontTexture, &srcRect, &dstRect);
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
    RenderChar(offs++);
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



    
