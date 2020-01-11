
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "src/misc.h"
#include "src/config.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"
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

PixelFont::PixelFont(const Config::Font & config, uint8_t * data)
  : Font(config.m_count)
  , m_data(data)
{
  m_width  = config.m_width;
  m_height = config.m_height;
}

bool PixelFont::Open(SDL_Renderer * renderer)
{
  m_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, m_width, m_height * m_charCount);
  if (m_texture == nullptr) {
    cerr << "error: cannot create font texture" << endl;
    return false;
  }

  cout << "info: creating font with " << dec << m_charCount << " chars" << endl;;

  std::vector<uint32_t> pixels;
  pixels.resize(m_width * m_height * m_charCount);

  // copy pixel data to the surface with the correct colours
  for (int i = 0; i < m_charCount; ++i) {
    uint8_t * srcPixels = m_data + (i * m_height);
    for (int y = 0; y < m_height; ++y) {
      uint32_t * dstPixels = &pixels[m_width * ((i * m_height) + y)];
      unsigned mask = 1 << (m_width - 1);
      for (int x = 0; x < m_width; ++x) {
        if (*srcPixels & mask) {
          *dstPixels = 0xffffffff;
        }
        else {
          *dstPixels = 0x00000000;
        }  
        dstPixels++;
        mask = mask >> 1;
      }
      ++srcPixels;
    }
  }

  // create texture
  if (SDL_UpdateTexture(m_texture, NULL, &pixels[0], m_width * sizeof(uint32_t)) != 0) {
    cerr << "error: cannot create font texture - " << SDL_GetError() << endl;
    return false;
  }

  cout << "info: font texture created" << endl;

  return true;
}

void PixelFont::RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  SDL_Rect srcRect = { 0, ch * GetHeight(), GetWidth(), GetHeight() };
  SDL_SetTextureColorMod(m_texture, 255, 255, 255);
  SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
  SDL_RenderFillRect(renderer, &dstRect);
  SDL_SetTextureColorMod(m_texture, fg.r, fg.g, fg.b);
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

bool TTFFont::Open(SDL_Renderer * renderer)
{
  if (m_font)
    FC_FreeFont(m_font);

  int pointSize = 20;  

  m_font = FC_CreateFont();  
  FC_LoadFont(m_font, renderer, m_name.c_str(), pointSize, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL); 

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

void TTFFont::RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
  SDL_RenderFillRect(renderer, &dstRect);

  char str[2] = { (char)(ch & 0xff), 0x00 };
  FC_DrawBoxAlign(m_font, renderer, dstRect, FC_ALIGN_CENTER, str); 
}
