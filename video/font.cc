
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "virtual_screen.h"
#include "mainwindow.h"
#include "SDL_FontCache/SDL_FontCache.h"

using namespace std;

Font::Font(int charCount)
  : m_charCount(charCount)
{}

Font::~Font()
{}

int Font::GetWidth() const
{ return m_width; }

int Font::GetHeight() const
{ return m_height; }

/////////////////////////////////////////////////////////////////////////////

PixelFont::PixelFont(int charCount, int width, int height, uint8_t * data)
  : Font(charCount)
  , m_data(data)
{
  m_width  = width;
  m_height = height;
}

bool PixelFont::Open(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg)
{
  // create the raw surface
  SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0, m_width, m_height * m_charCount, 0, SDL_PIXELFORMAT_RGB24);
  if (surface == NULL) {
    SDL_Log("SDL_CreateRGBSurfaceWithFormat() for font failed: %s", SDL_GetError());
    return false;
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
  m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
  SDL_FreeSurface(surface);

  return true;
}

void PixelFont::RenderChar(int ch, SDL_Renderer * renderer, const SDL_Rect & dstRect)
{
  SDL_Rect srcRect = { 0, ch * GetHeight(), GetWidth(), GetHeight() };
  SDL_RenderCopy(renderer, m_texture, &srcRect, &dstRect);
}

/////////////////////////////////////////////////////////////////////////////

TTFFont::TTFFont(const std::string & fontName, int charCount)
  : Font(charCount)
  , m_name(fontName)
  , m_font(nullptr)
{
}

TTFFont::~TTFFont()
{
  if (m_font)
    FC_FreeFont(m_font);
}

bool TTFFont::Open(SDL_Renderer * renderer, const SDL_Color & fg, const SDL_Color & bg)
{
  if (m_font)
    FC_FreeFont(m_font);

  int pointSize = 20;  

  m_bg = bg;  

  m_font = FC_CreateFont();  
  FC_LoadFont(m_font, renderer, m_name.c_str(), pointSize, FC_MakeColor(fg.r, fg.g, fg.b, 255), TTF_STYLE_NORMAL); 

  if (!m_font) {
    cerr << "error: could not load font " << m_name << endl;
    return false;
  }

  {
    TTF_Font * ttf = TTF_OpenFont(m_name.c_str(), pointSize / 2);
  
    // calculate maximum character width - the hard way
    m_height = TTF_FontHeight(ttf);
    m_width = 0;

    char str[2] = { 0x00, 0x00 };
    for (int i = 0x20; i < 0x7f; ++i) {
      str[0] = i;
      int w, h;
      TTF_SizeUTF8(ttf, str, &w, &h);
      m_width = std::max<int>(w, m_width);
    }

    TTF_CloseFont(ttf);
  }

  cerr << "info: TTF font '" << m_name << "' is " << dec << m_width << "x" << m_height << endl;

  return true;
}

void TTFFont::RenderChar(int ch, SDL_Renderer * renderer, const SDL_Rect & dstRect)
{
  SDL_SetRenderDrawColor(renderer, m_bg.r, m_bg.g, m_bg.b, 255);
  SDL_RenderFillRect(renderer, &dstRect);

  char str[2] = { (char)(ch & 0xff), 0x00 };
  FC_DrawBoxAlign(m_font, renderer, dstRect, FC_ALIGN_CENTER, str); 
}
