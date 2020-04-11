#ifndef FONT_H_
#define FONT_H_

#include "SDL_FontCache/SDL_FontCache.h"
#include "src/emuconfig.h"
#include "video/chargenrom.h"

typedef int FontChar;

class Font
{
  public:
    Font(int charCount);
    ~Font();

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;

    virtual bool Open(SDL_Renderer * m_renderer) = 0;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) = 0;
    virtual void Modify(FontChar ch, int row, uint8_t data) = 0;

  protected:  
    SDL_Texture * m_texture;
    int m_charCount;
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

    void SetHeight(int hgt);

    virtual int GetWidth() const override;
    virtual int GetHeight() const override;

    virtual void Modify(FontChar ch, int row, uint8_t data) override;

  protected:  
    uint8_t * m_data;
    Config::Font m_config;
};

class TTFFont : public Font 
{
  public:
    TTFFont(const std::string & fontName, int fontSize);
    TTFFont(const Config::Font & config, int charCount, const std::string & fontName, int fontSize);
    ~TTFFont();

    virtual int GetWidth() const override;
    virtual int GetHeight() const override;

    bool Open(SDL_Renderer * m_renderer) override;
    virtual void RenderChar(FontChar ch, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg) override;
    
    virtual void Modify(FontChar ch, int row, uint8_t data) override;

  protected:
    bool UsePixelFont(const FontChar & ch) const;

    std::unique_ptr<PixelFont> m_pixelFont;
    std::string m_name;  
    int m_fontSize;
    FC_Font * m_font;

  private:  
    int m_width;
    int m_height;
};

#endif // FONT_H_