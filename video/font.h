#ifndef FONT_H_
#define FONT_H_

#include "SDL_FontCache/SDL_FontCache.h"

class Font
{
  public:
    Font(int charCount, int width, int height);
    ~Font();
    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual bool Open(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg) = 0;
    virtual void RenderChar(int ch, SDL_Renderer * renderer, const SDL_Rect & dstRect) = 0;

  protected:  
    int m_charCount;
    int m_width;
    int m_height;
};

class PixelFont : public Font
{
  public:
    PixelFont(int charCount, int width, int height, uint8_t * data);
    virtual bool Open(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg) override;
    virtual void RenderChar(int ch, SDL_Renderer * renderer, const SDL_Rect & dstRect) override;

  protected:  
    uint8_t * m_data;
    SDL_Texture * m_texture;
};

class TTFFont : public Font
{
  public:
    TTFFont(const std::string & fontName, int charCount, int width, int height);
    ~TTFFont();

    bool Open(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg) override;
    virtual void RenderChar(int ch, SDL_Renderer * renderer, const SDL_Rect & dstRect) override;

  protected:
    std::string m_name;  
    FC_Font * m_font;
    SDL_Color m_bg;
};


#endif // FONT_H_