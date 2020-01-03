
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

    virtual void Update(bool hasChanged = false);

    virtual void RefreshChar(int pos) = 0;
    virtual void RenderChar(int offs, SDL_Colour & fg, SDL_Colour & bg);

    virtual FontChar GetCharAtPos(int offs) = 0;

  protected:  
    MainWindow & m_mainWindow;

    int m_rows;
    int m_cols;

    int m_width;
    int m_height;

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

    template<class Type>
    static void AddType(const std::string & name)
    {
      if (!g_virtualScreenFactory.HasKey(name))
        g_virtualScreenFactory.AddConcreteClass<Type>(name);
    }

    static VirtualScreen * Create(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info);  

    virtual void WriteMemory(int offs, uint8_t ch) = 0;
    virtual uint8_t ReadMemory(int offs) const = 0;

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

    virtual void WriteMemory(int offs, uint8_t ch);
    virtual uint8_t ReadMemory(int offs) const;

    virtual void RefreshScreen();

    virtual FontChar GetCharAtPos(int offs) override;

  protected:  
    static VirtualScreenFactory g_virtualScreenFactory;

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
    virtual void RefreshChar(int pos);

  protected:  
    SDL_Color m_bgColour;
    SDL_Color m_fgColour;
};

#endif // VIRTUALSCREEN_H_