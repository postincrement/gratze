#ifndef FONT_H_
#define FONT_H_

#include "SDL_FontCache/SDL_FontCache.h"
#include "src/emuconfig.h"
#include "video/chargenrom.h"

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
    SDL_Texture * m_texture;
    int m_charCount;
    int m_width;
    int m_height;
};

class CharacterGeneratorROM;

class PixelFont : public Font
{
  public:
    PixelFont(const CharacterGeneratorROM & pixelFont);
    PixelFont(const CharacterGeneratorROM & pixelFont, int count, Config::FontCreator creator);
    PixelFont(const Config::Font & fontConfig);
    virtual bool Open(SDL_Renderer * m_renderer) override;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

  protected:  
    uint8_t * m_data;
    Config::Font m_config;
  private:
    int m_pixelWidth;
    int m_pixelHeight;  
};

class TTFFont : public Font 
{
  public:
    TTFFont(const std::string & fontName, int fontSize);
    TTFFont(const Config::Font & config, int charCount, const std::string & fontName, int fontSize);
    ~TTFFont();

    bool Open(SDL_Renderer * m_renderer) override;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;

  protected:
    bool UsePixelFont(const FontChar & ch) const;

    std::unique_ptr<PixelFont> m_pixelFont;
    std::string m_name;  
    int m_fontSize;
    FC_Font * m_font;
};


#endif // FONT_H_