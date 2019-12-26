#include "sdl_video.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>


using namespace std;

#define CHAR_COUNT      128
#define TRS_CHAR_HEIGHT 12


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

  cout << "ch = " << m_width << "x" << m_height << endl;
  cout << "w = " << surface->w << ", h = " << surface->h << ", pitch = " << (int)surface->pitch << endl;

  // lock the surface
  SDL_LockSurface(surface);

  //SDL_PixelFormat * format = surface->format;
  //format->palette->colors[0] = { 0,   0, 0 };
  //format->palette->colors[1] = { 0, 255, 0 };


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
  , m_font(font)
{
  m_memory.resize(m_rows * m_cols);
}

MemoryMappedVideo::~MemoryMappedVideo()
{
  SDL_FreeSurface(m_winSurface);
  SDL_FreeSurface(m_fontSurface);
  SDL_DestroyWindow(m_window);
}


bool MemoryMappedVideo::Open()
{
  // get font height and width
  m_charWidth = m_font->GetWidth();
  m_charHeight = m_font->GetHeight();

  // set border
  m_left   = 1 * m_charWidth;
  m_right  = 1 * m_charWidth;
  m_top    = 1 * m_charWidth;
  m_bottom = 1 * m_charWidth;

  // create window
  int height = (m_top + m_rows * m_charHeight + m_bottom) * m_scale;
  int width  = (m_left + m_cols * m_charWidth + m_right) * m_scale;
  m_window = SDL_CreateWindow("Emulator", 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED, 
                              width, height, SDL_WINDOW_SHOWN); 

  // create a surface for the window
  m_winSurface = SDL_GetWindowSurface(m_window);

  // set foreground and background colour
  m_backgroundColour = SDL_MapRGB(m_winSurface->format,   0,   0,   0);
  m_foregroundColour = SDL_MapRGB(m_winSurface->format,   0, 255,   0);

  // create a surface for the font
  m_fontSurface = m_font->GetSurface();
  if (m_fontSurface == NULL)
    return false;

  // clear the window
  Clear();

  return true;
}

void MemoryMappedVideo::Clear()
{
  // clear the surface
  SDL_FillRect(m_winSurface, NULL, m_backgroundColour);

  // set memory to spaces
  memset(&m_memory[0], 0x20, m_memory.size());

  // Update the window display
	SDL_UpdateWindowSurface(m_window);
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

  // fill char rect
  //SDL_FillRect(m_winSurface, &dstRect, m_foregroundColour);

  // blit char rect
  SDL_Rect srcRect = { 0, ch * m_charHeight, m_charWidth, m_charHeight };

  int result = SDL_BlitScaled(m_fontSurface, &srcRect, m_winSurface, &dstRect);
  if (result != 0) {
    SDL_Log("SDL_BlitSurface() for font failed: %s", SDL_GetError());
    exit(1);
  }

  // Update the window display
	SDL_UpdateWindowSurface(m_window);
}

