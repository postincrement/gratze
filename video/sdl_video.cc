#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "sdl_video.h"

using namespace std;

#define CHAR_COUNT      256
#define TRS_CHAR_HEIGHT 12

#define LAZY_UPDATE_MSECS   20

MemoryMappedVideo::Font::Font(int width, int height, uint8_t * data)
  : m_width(width)
  , m_height(height)
  , m_data(data)
{}


SDL_Surface * MemoryMappedVideo::Font::GetSurface()
{
  // create the raw surface
  SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0, m_width, m_height * CHAR_COUNT, 0, SDL_PIXELFORMAT_RGB24);
  if (surface == NULL) {
    SDL_Log("SDL_CreateRGBSurfaceWithFormat() for font failed: %s", SDL_GetError());
    exit(1);
  }

  // lock the surface
  SDL_LockSurface(surface);

  // copy pixel data to the surface
  for (int i = 0; i < CHAR_COUNT; ++i) {
    uint8_t * srcPixels = m_data + (i * TRS_CHAR_HEIGHT);
    for (int y = 0; y < m_height; ++y) {
      uint8_t * dstPixels = (uint8_t *)surface->pixels + ((i * m_height) + y) * surface->pitch;
      unsigned mask = 1;
      for (int x = 0; x < m_width; ++x) {
        if (*srcPixels & mask) {
          dstPixels[0] = 0x00; 
          dstPixels[1] = 0xff; 
          dstPixels[2] = 0x00; 
        }
        else {
          dstPixels[0] = 0x0; 
          dstPixels[1] = 0x0; 
          dstPixels[2] = 0x0; 
        }  
        dstPixels += 3;
        mask = mask << 1;
      }
      ++srcPixels;
    }
  }

  // unlock the surface
  SDL_UnlockSurface(surface);

  return surface;
}


/////////////////////////////////////////////////////

MemoryMappedVideo::MemoryMappedVideo(int rows, int cols, int scale, Font * font, uint8_t * data)
  : m_rows(rows)
  , m_cols(cols)
  , m_scale(scale)
  , m_lazyUpdates(false)
  , m_font(font)
{
  m_memory.resize(m_rows * m_cols);
}

MemoryMappedVideo::~MemoryMappedVideo()
{
#if USE_TEXTURES  
#else
  SDL_FreeSurface(m_winSurface);
  SDL_FreeSurface(m_fontSurface);
#endif  
  SDL_DestroyWindow(m_window);
}

bool MemoryMappedVideo::SetLazyUpdate(bool v)
{
  m_lazyUpdates = v;
}

bool MemoryMappedVideo::Open(const std::string & title)
{
  m_panelWidth = 200;

  // get font height and width
  m_charWidth = m_font->GetWidth();
  m_charHeight = m_font->GetHeight();

  // set screen borders
  m_left   = 1 * m_charWidth;
  m_right  = 1 * m_charWidth;
  m_top    = 1 * m_charWidth;
  m_bottom = 1 * m_charWidth;

  // calcuate screen rect
  m_screenRect = { m_left * m_scale, 
                   m_top  * m_scale, 
                   m_cols * m_charWidth  * m_scale, 
                   m_rows * m_charHeight * m_scale };

  int screenPanelWidth  = (m_left + m_cols * m_charWidth  + m_right)  * m_scale;
  int screenPanelHeight = (m_top  + m_rows * m_charHeight + m_bottom) * m_scale;

  // calculate panel rect
  m_panelRect  = { screenPanelWidth, 0, m_panelWidth, screenPanelHeight };

  // create window
  cout << m_top << " " << m_screenRect.h << " " << m_bottom << endl;

  int height = screenPanelHeight;
  int width  = screenPanelWidth + m_panelWidth; 
  m_window = SDL_CreateWindow(title.c_str(), 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED, 
                              width, height, SDL_WINDOW_SHOWN); 

#if USE_TEXTURES
  m_renderer = SDL_CreateRenderer(m_window, -1, 0);

  SDL_Surface * fontSurface = m_font->GetSurface();
  if (fontSurface == NULL)
    return false;

  m_fontTexture = SDL_CreateTextureFromSurface(m_renderer, fontSurface);
  SDL_FreeSurface(fontSurface);

  // set foreground and background colour
  m_backgroundColour = { 0,   0,   0 };
  m_foregroundColour = { 0, 255,   0 };

  SDL_Color bg = { 80,   80,   80 };

  // clear panel
  SDL_Rect bgRect = { 0, 0, screenPanelWidth, screenPanelHeight };
  SDL_SetRenderDrawColor(m_renderer, bg.r, bg.g, bg.b, 255);
  SDL_RenderFillRect(m_renderer, &m_panelRect);
  Update(true);

#else  

  // create a surface for the window
  m_winSurface = SDL_GetWindowSurface(m_window);

  // create a surface for the font
  m_fontSurface = m_font->GetSurface();
  if (m_fontSurface == NULL)
    return false;

  // convert the surface to match the screen
  SDL_ConvertSurface(m_fontSurface, m_winSurface->format, 0);  

  // set foreground and background colour
  m_backgroundColour = SDL_MapRGB(m_winSurface->format,   0,   0,   0);
  m_foregroundColour = SDL_MapRGB(m_winSurface->format,   0, 255,   0);

  auto bg = SDL_MapRGB(m_winSurface->format,   80,   80,   80);

  // clear panel
  SDL_Rect bgRect = { 0, 0, screenPanelWidth, screenPanelHeight };
  SDL_FillRect(m_winSurface, &m_panelRect, bg);

#endif

  // clear the window
//  Clear();

  return true;
}

void MemoryMappedVideo::Clear()
{
  // set memory to spaces
  memset(&m_memory[0], 0x20, m_memory.size());

#if USE_TEXTURES
  SDL_SetRenderDrawColor(m_renderer, m_backgroundColour.r, m_backgroundColour.g, m_backgroundColour.b, 255);
  SDL_RenderFillRect(m_renderer, &m_panelRect);
  Update(true);
#else
  // clear the surface
  SDL_FillRect(m_winSurface, &m_screenRect, m_backgroundColour);

  // Update the window display
	SDL_UpdateWindowSurface(m_window);
#endif  
}

void MemoryMappedVideo::Update(bool hasChanged)
{
  if (!m_lazyUpdates) {
    if (!hasChanged || m_dirty)
      SDL_RenderPresent(m_renderer);
    m_dirty = false;  
    return;
  }

  auto now = std::chrono::system_clock::now();
  if (!m_dirty && hasChanged) {
    m_updateTimer = now + std::chrono::milliseconds(LAZY_UPDATE_MSECS);
    m_dirty = true;
  }

  if (m_dirty && (now > m_updateTimer)) {
    SDL_RenderPresent(m_renderer);
    m_dirty = false;
  }
}

void MemoryMappedVideo::WriteChar(unsigned offset, uint8_t ch)
{
  if (offset >= m_memory.size())
    return;

  m_memory[offset] = ch;

  ch = (ch % CHAR_COUNT);  

  int x = (offset % m_cols) * m_charWidth;  
  int y = (offset / m_cols) * m_charHeight;

  SDL_Rect dstRect = { (m_left + x) * m_scale, (m_top + y) * m_scale, m_charWidth * m_scale, m_charHeight * m_scale };

  // blit char rect
  SDL_Rect srcRect = { 0, ch * m_charHeight, m_charWidth, m_charHeight };

#if USE_TEXTURES

  SDL_RenderCopy(m_renderer, m_fontTexture, &srcRect, &dstRect);  
  Update(true);
#else

  int result = SDL_BlitScaled(m_fontSurface, &srcRect, m_winSurface, &dstRect);
  if (result != 0) {
    SDL_Log("SDL_BlitSurface() for font failed: %s", SDL_GetError());
    exit(1);
  }

  // Update the window display
	//SDL_UpdateWindowSurface(m_window);
  SDL_UpdateWindowSurfaceRects(m_window, &dstRect, 1);
#endif  
}

