
#ifndef VIRTUALSCREEN_H_
#define VIRTUALSCREEN_H_

#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include <SDL2/SDL.h> 

#include "options.h"

class PixelFont
{
  public:
    PixelFont(int charCount, int width, int height, uint8_t * data);

    int GetWidth() const
    { return m_width; }

    int GetHeight() const
    { return m_height; }

    SDL_Texture * GetTexture(SDL_Renderer * m_renderer, const SDL_Color & fg, const SDL_Color & bg);

  protected:  
    int m_charCount;
    int m_width;
    int m_height;
    uint8_t * m_data;
};

class MainWindow;

class VirtualScreen
{
  public:
    VirtualScreen(MainWindow & mainWindow, const Options & options, int width, int height, int dataLen);

    ~VirtualScreen();

    bool Open(int width, int height, int dataLen);

    void SetColors(const SDL_Color & fg, const SDL_Color & bg);

    void Refresh();

    void Update(bool hasChanged = false);

    void SetFont(PixelFont * font);

    virtual void WriteChar(int offs, uint8_t ch);
    virtual uint8_t ReadChar(int offs) const;

    int GetFontChar(int offs);
    void RenderChar(int offs);

  protected:  
    MainWindow & m_mainWindow;
    int m_width;
    int m_height;
    int m_offsMask;
    int m_start = 0;
    int m_cols;
    int m_rows;

    SDL_Color m_bgColour;
    SDL_Color m_fgColour;

    std::vector<std::uint8_t> m_memory;
    std::unique_ptr<PixelFont> m_font;
    SDL_Texture * m_fontTexture;

    bool m_lazyUpdates;
    bool m_dirty;
    std::chrono::system_clock::time_point m_updateTimer;
};

#endif // VIRTUALSCREEN_H_