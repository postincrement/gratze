
#ifndef VIRTUALSCREEN_H_
#define VIRTUALSCREEN_H_

#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include <SDL.h> 

#include "options.h"
#include "video/font.h"

class MainWindow;
class Emulator;

class VirtualScreen
{
  public:
    VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options);

    ~VirtualScreen();

    bool Open(int width, int height, int dataLen);

    void SetColors(const SDL_Color & fg, const SDL_Color & bg);

    void Refresh();

    void Update(bool hasChanged = false);

    bool SetFont(Font * font);
    bool SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg);
    void GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const;

    virtual void Write(int offs, uint8_t ch);
    virtual uint8_t Read(int offs) const;

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
    std::unique_ptr<Font> m_font;

    bool m_lazyUpdates;
    bool m_dirty;
    std::chrono::system_clock::time_point m_updateTimer;
};

#endif // VIRTUALSCREEN_H_