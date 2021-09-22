
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "common/misc.h"
#include "common/config.h"
#include "video/virtual_screen.h"
#include "src/mainwindow.h"
#include "SDL_FontCache/SDL_FontCache.h"

using namespace std;

Font::Font(int charCount)
  : m_charCount(charCount)
{
  m_texture = NULL;
}

Font::~Font()
{
  if (m_texture != NULL)
    SDL_DestroyTexture(m_texture);
}

int Font::GetCharCount() const
{
  return m_charCount;
}

void Font::Modify(FontChar ch, int row, uint8_t data)
{
  Modify(ch, row, 1, &data);
}

/////////////////////////////////////////////////////////////////////////////

static unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

static bool UnpackCharacterGeneratorFont(const Config::Font & config, std::vector<uint8_t> & fontData)
{
  if (config.m_charGen == nullptr) {
    cerr << "error: no character generator found" << endl;
    return false;
  }

  const CharacterGeneratorROM & charGen = *config.m_charGen;

  if (config.m_width < charGen.m_width) {
    cerr << "error: font width " << (int)config.m_width << " cannot be less than character generator width " << (int)charGen.m_width << endl;
    return false;
  }

  if (charGen.m_stride < charGen.m_height) {
    cerr << "error: chargen stride " << (int)charGen.m_stride << " cannot be less than character generator height " << (int)charGen.m_height << endl;
    return false;
  }

  if (charGen.m_width > 8) {
    cerr << "error: character generators > 8 pixels wide not supported" << endl;
    return false;
  }

  //if (config.m_height < charGen.m_height) {
  //  cerr << "error: font height " << (int)config.m_height << " cannot be less than character generator height " << (int)charGen.m_height << endl;
  //  return false;
  //}
  int copyHeight = std::min(config.m_height, charGen.m_height);

  fontData.resize(config.m_count * config.m_height * ((config.m_width + 7) / 8));

  int shiftBits = config.m_width - charGen.m_width;

  for (int c = 0; c < charGen.m_count; ++c) {
    const uint8_t * src = charGen.m_data + c * charGen.m_stride;
    uint8_t * dst = &fontData[c * config.m_height];

    int y;
    for (y = 0; y < copyHeight; ++y) {
      *dst++ = charGen.m_reverse ? 
                    reverse(*src++) >> (8 - charGen.m_width - shiftBits) :
                    *src++ << shiftBits
                    ;
    }
    for (; y < config.m_height;++y)
      *dst++ = 0x00;
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////////

PixelFont::PixelFont(const Config::Font & config)
  : Font(config.m_count)
  , m_config(config)
{
  //m_width  = config.m_width;
  //m_height = config.m_height;
  //m_pixelWidth = config.m_width;
  //m_pixelHeight = config.m_height;
}

PixelFont::PixelFont(const CharacterGeneratorROM & pixelFont, int count, Config::FontCreator creator)
  : Font(count)
{
  m_config.m_count   = count;
  m_config.m_width   = pixelFont.m_width;
  m_config.m_height  = pixelFont.m_height;
  m_config.m_charGen = &pixelFont;
  m_config.m_creator = creator;

  //m_pixelWidth  = m_config.m_width;
  //m_pixelHeight = m_config.m_height;
}

PixelFont::PixelFont(const CharacterGeneratorROM & pixelFont)
  : PixelFont(pixelFont, pixelFont.m_count, nullptr)
{
}

int PixelFont::GetWidth() const
{ 
  return m_config.m_width; 
}

int PixelFont::GetHeight() const
{ 
  return m_config.m_height; 
}

void PixelFont::SetHeight(int hgt)
{
  m_config.m_height = hgt;
}

bool PixelFont::Open(SDL_Renderer * renderer)
{
  // get pixel data
  std::vector<uint8_t> fontData;
  fontData.resize(m_config.m_count * m_config.m_height);

  // unpack pixel data
  if (m_config.m_charGen != NULL) {
    if (!UnpackCharacterGeneratorFont(m_config, fontData)) {
      return false;
    }
  }

  // do extra steps if required
  if (m_config.m_creator != NULL) {
    if (!(m_config.m_creator)(m_config, fontData)) {
      cerr << "error: font creator returned error" << endl;
      return false;
    }
  }

  if (m_texture)
    SDL_DestroyTexture(m_texture);

  m_texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_RGBA32, 
                                SDL_TEXTUREACCESS_STREAMING, 
                                m_config.m_width, 
                                m_config.m_height * m_charCount);
  if (m_texture == NULL) {
    cerr << "error: cannot create font texture - " << SDL_GetError() << endl;
    return false;
  }

  cout << "info: creating font with " << dec << m_charCount << " chars" << endl;;

  std::vector<uint32_t> pixels;
  pixels.resize(m_config.m_width * m_config.m_height * m_charCount);

  const uint8_t * data = &fontData[0];

  // copy pixel data to the surface with the correct colours
  for (int i = 0; i < m_charCount; ++i) {
    const uint8_t * srcPixels = data + (i * m_config.m_height);
    for (int y = 0; y < m_config.m_height; ++y) {
      uint32_t * dstPixels = &pixels[m_config.m_width * ((i * m_config.m_height) + y)];
      unsigned mask = 1 << (m_config.m_width - 1);
      for (int x = 0; x < m_config.m_width; ++x) {
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
  if (SDL_UpdateTexture(m_texture, NULL, &pixels[0], m_config.m_width * sizeof(uint32_t)) != 0) {
    cerr << "error: cannot update font texture - " << SDL_GetError() << endl;
    return false;
  }

  cout << "info: font texture created" << endl;

  return true;
}

void PixelFont::Modify(FontChar ch, int rowStart, int rowCount, uint8_t * rowData)
{
  if (ch >= m_config.m_count) {
    cerr << "PCG char " << HEXFORMAT0x2(ch) << " not valid" << endl;
    return;
  }
  if (rowStart >= m_config.m_height)
    return;
  if ((rowStart + rowCount) >= m_config.m_height)
    rowCount = m_config.m_height - rowStart;  

  //cout << "font: " << (int)ch << " row " << rowStart << " to " << (rowStart + rowCount - 1) << endl; 
  SDL_Rect rect = { 0, (ch * m_config.m_height) + rowStart, m_config.m_width, rowCount };

  uint8_t * pixels;
  int pitch;
  if (SDL_LockTexture(m_texture, &rect, (void **)&pixels, &pitch)) {
    cerr << "sdl: lock texture failed" << endl;
    return;
  }

  int shiftBits = m_config.m_width - m_config.m_charGen->m_width;

  for (int y = 0; y < rowCount; ++y) {
    uint8_t data = *rowData++;
    //data << shiftBits;
    unsigned mask = 1 << (m_config.m_width - 1);
    uint32_t * dstPixels = (uint32_t *)pixels;
    for (int x = 0; x < m_config.m_width; ++x) {
      if (data & mask) {
        *dstPixels = 0xffffffff;
      }
      else {
        *dstPixels = 0x00000000;
      }  
      dstPixels++;
      mask = mask >> 1;
    }
    pixels += pitch;
  }

  SDL_UnlockTexture(m_texture);
}


void PixelFont::RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  #if 1
  SDL_Rect srcRect = { 0, ch * m_config.m_height, m_config.m_width, m_config.m_height };
  
  // set background
  SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_NONE);
  SDL_SetTextureColorMod(m_texture, bg.r, bg.g, bg.b);
  SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
  SDL_RenderFillRect(renderer, &dstRect);

  // set foreground
  SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(m_texture, fg.r, fg.g, fg.b);
  SDL_RenderCopy(renderer, m_texture, &srcRect, &dstRect);

  #else

  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &dstRect);

  #endif
}

/////////////////////////////////////////////////////////////////////////////

TTFFont::TTFFont(const std::string & fontName, int fontSize)
  : Font(256)
  , m_name(fontName)
  , m_fontSize(fontSize)
  , m_font(nullptr)
{
}

TTFFont::TTFFont(const Config::Font & config, int charCount, const std::string & fontName, int fontSize)
  : TTFFont(fontName, fontSize)
{
  m_pixelFont.reset(new PixelFont(config));
}

TTFFont::~TTFFont()
{
  if (m_font)
    FC_FreeFont(m_font);
}

int TTFFont::GetWidth() const
{ 
  return m_width; 
}

int TTFFont::GetHeight() const
{ 
  return m_height; 
}

bool TTFFont::Open(SDL_Renderer * renderer)
{
  if (m_font)
    FC_FreeFont(m_font);

  if (m_pixelFont && !m_pixelFont->Open(renderer))
    return false;  

  m_font = FC_CreateFont();  
  FC_LoadFont(m_font, renderer, m_name.c_str(), m_fontSize, FC_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL); 

  if (!m_font) {
    cerr << "error: could not load font " << m_name << endl;
    return false;
  }

  // this does not work :(
  /*
  int m_width = FC_GetMaxWidth(m_font);
  int m_height = FC_GetLineSpacing(m_font);
  */

  {
    // calculate maximum character width - the hard way
    TTF_Font * ttf = TTF_OpenFont(m_name.c_str(), m_fontSize);  
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

  m_width += 1;   // allow space between chars
  m_height += 1;  // allow space between lines

  if (m_texture)
    SDL_DestroyTexture(m_texture);

  m_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, m_width, m_height * m_charCount);
  if (m_texture == NULL) {
    cerr << "error: cannot create font texture - " << SDL_GetError() << endl;
    return false;
  }

  cerr << "info: TTF font '" << m_name << " " << (int)m_fontSize << " is " << dec << m_width << "x" << m_height << endl;

  return true;
}

bool TTFFont::UsePixelFont(const FontChar & ch) const
{
  return m_pixelFont && (
    ((ch < 0x20) || ((ch >= 0x7f) && (ch < 0xb0)) || (ch >= 0xff)) 
  );
}

void TTFFont::RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg)
{
  if (UsePixelFont(ch)) {
    return m_pixelFont->RenderChar(ch, renderer, dstRect, fg, bg);
  }

  //cerr << "TTF rendering char " << (int)ch << endl;

  if (m_texture == nullptr) {
    cerr << "font: null texture" << endl;
  }
  else if (renderer == nullptr) {
    cerr << "font: null renderer" << endl;
  }
  else {
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_NONE);
    SDL_SetTextureColorMod(m_texture, bg.r, bg.g, bg.b);
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
    SDL_RenderFillRect(renderer, &dstRect);

    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(m_texture, fg.r, fg.g, fg.b);
    FC_SetDefaultColor(m_font, fg);

    char str[2] = { (char)(ch & 0xff), 0x00 };
    FC_DrawBoxAlign(m_font, renderer, dstRect, FC_ALIGN_CENTER, str); 
  }
}

void TTFFont::Modify(FontChar ch, int rowStart, int rowCount, uint8_t * rowData)
{}

/////////////////////////////////////////////////////////////////////////////////
