
#ifndef VIRTUALSCREEN_H_
#define VIRTUALSCREEN_H_

#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include <SDL.h> 

#include "factory.h"
#include "options.h"
#include "video/font.h"

class MainWindow;
class Emulator;

class VirtualScreen;

namespace Config {
  class Video;
}

using VirtualScreenFactory = Factory<VirtualScreen, std::string, MainWindow &, Emulator &, const Options &, const Config::Video &>;

class TextWindow
{
  public:
    TextWindow(MainWindow & mainWindow, int rows, int cols, int width, int height);

    virtual bool Open();

    virtual void Update(bool hasChanged = false);

    virtual void RefreshCharAtPos(int x, int y) = 0;
    virtual void RenderCharAtPos(int addr, SDL_Colour & fg, SDL_Colour & bg);

    virtual FontChar GetCharAtPos(int x, int y) = 0;
    virtual void SetColScale(int scale);

  protected:  
    MainWindow & m_mainWindow;

    int m_rows;
    int m_cols;

    int m_width;
    int m_height;

    double m_hscale = 1;
    double m_vscale = 1;

    int m_colScale;

    int m_left = 0;
    int m_top = 0;

    int m_visibleSize = 0;
    int m_visibleMask = 0;

    bool m_lazyUpdates;
    bool m_dirty;
    std::chrono::system_clock::time_point m_updateTimer;
    std::unique_ptr<Font> m_font;
};

/////////////////////////////////////////////////////////////////////////////////

class VirtualScreen : public TextWindow
{
  public:
    VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);
    ~VirtualScreen();

    virtual void SetScale(double hscale, double vscale);
    virtual void SetOffset(int left, int top);

    template<class Type>
    static void AddType(const std::string & name)
    {
      if (!g_virtualScreenFactory.HasKey(name))
        g_virtualScreenFactory.AddConcreteClass<Type>(name);
    }

    static VirtualScreen * Create(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);  

    virtual void WriteMemoryAtAddress(int addr, uint8_t ch) = 0;
    virtual uint8_t ReadMemoryAtAddress(int addr) const = 0;
    virtual void RenderCharAtAddress(int addr) = 0;

    virtual void RefreshScreen();

    virtual void SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg) = 0;
    virtual void GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const = 0;

  protected:  
    static VirtualScreenFactory g_virtualScreenFactory;
};

/////////////////////////////////////////////////////////////////////////////////

class MemoryMappedVideo : public VirtualScreen
{
  public:
    MemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);
    ~MemoryMappedVideo();

    virtual bool Open() override;

    virtual void WriteMemoryAtAddress(int addr, uint8_t ch);
    virtual uint8_t ReadMemoryAtAddress(int addr) const;

    virtual bool SetFont(const Config::Font & font);

    virtual FontChar GetCharAtPos(int x, int y) override;

  protected:  
    Config::Font m_fontConfig;
    Options m_options;
    std::vector<uint8_t> m_fontData;
    std::vector<uint8_t> m_memory;
    int m_offset;

    int m_memoryMask = 0;
};

/////////////////////////////////////////////////////////////////////////////////

class SingleColourMemoryMappedVideo : public MemoryMappedVideo
{
  public:
    SingleColourMemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);

    virtual void SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg);
    virtual void GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const;
    virtual void RenderCharAtAddress(int addr);

  protected:  
    SDL_Color m_bgColour;
    SDL_Color m_fgColour;
};

#endif // VIRTUALSCREEN_H_