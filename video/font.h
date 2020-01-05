#ifndef FONT_H_
#define FONT_H_

#include "SDL_FontCache/SDL_FontCache.h"
#include "cpu/emuconfig.h"
#include "chargenrom.h"

typedef uint16_t FontChar;

class Font
{
  public:
    Font(int charCount);
    ~Font();
    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual bool Open(SDL_Renderer * m_renderer) = 0;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) = 0;

  protected:  
    int m_charCount;
    int m_width;
    int m_height;
};

class PixelFont : public Font
{
  public:
    PixelFont(const Config::Font & contfig, uint8_t * data);
    virtual bool Open(SDL_Renderer * m_renderer) override;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

  protected:  
    uint8_t * m_data;
    SDL_Texture * m_texture;
};

class TTFFont : public Font
{
  public:
    TTFFont(const std::string & fontName, int charCount);
    ~TTFFont();

    bool Open(SDL_Renderer * m_renderer) override;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

  protected:
    std::string m_name;  
    FC_Font * m_font;
};


#endif // FONT_H_