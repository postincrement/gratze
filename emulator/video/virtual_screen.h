
#ifndef VIRTUALSCREEN_H_
#define VIRTUALSCREEN_H_

#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <map>
#include <set>

#include <SDL.h>

#include "common/factory.h"
#include "src/options.h"
#include "video/font.h"

class MainWindow;

class VirtualScreen;

namespace Config {
  class Video;
}

class VirtualScreen
{
  public:
    VirtualScreen(MainWindow & mainWindow, const Options & options, int cols, int rows);
    virtual ~VirtualScreen();

    virtual bool Open();

    virtual int GetRows() const;
    virtual int GetCols() const;

    virtual void Update(bool hasChanged = false);

    virtual void SetScale(int hscale, int vscale);
    virtual void SetColScale(int scale);

    virtual void EnableCursor(bool enable = true);
    virtual void SetCursorPos(int x, int y);

    virtual void RefreshCharAtLoc(int loc);

    virtual void RefreshScreen();
    virtual bool ResizeScreen();

    virtual int MapPosToLoc(int x, int y);
    virtual bool MapLocToPos(int & x, int & y, int addr);

    // decendant classes must implement one of the following interfaces
    virtual FontChar GetCharAtPos(int x, int y);
    virtual void GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg);

    virtual FontChar GetCharAtLoc(int addr) const;
    virtual void GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg);

    virtual bool SetFont(Font * font, int cols = -1, int rows = -1);

  private:
    virtual void RenderChar(FontChar ch, bool withCursor, SDL_Renderer * renderer, const SDL_Rect & dstRect, const SDL_Colour & fg, const SDL_Colour & bg);
    virtual void RenderCharAtPos(int x, int y, bool withCursor);

  protected:
    MainWindow & m_mainWindow;
    Options m_options;

    int m_rows;
    int m_cols;

    int m_width;
    int m_height;

    int m_hscale = 1;
    int m_vscale = 1;
    int m_colScale = 1;

    int m_cursorX = 0;
    int m_cursorY = 0;
    bool m_cursorEnabled = false;

    int m_visibleSize = 0;
    int m_visibleMask = 0;

    bool m_lazyUpdates;
    bool m_dirty;
    std::chrono::system_clock::time_point m_updateTimer;
    std::unique_ptr<Font> m_font;
};

/////////////////////////////////////////////////////////////////////////////////

class MemoryMappedScreen;

using MemoryMappedScreenFactory = Factory<MemoryMappedScreen, std::string, MainWindow &, const Options &, const Config::MemoryMappedScreen &>;

class MemoryMappedScreen : public VirtualScreen
{
  public:
    MemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);
    virtual ~MemoryMappedScreen();

    // CPU access
    virtual void WriteMemoryAtAddress(int addr, uint8_t ch);
    virtual uint8_t ReadMemoryAtAddress(int addr) const;

    // overrides from VirtualScreen
    virtual void GetColourAtLoc(int loc, SDL_Colour & fg, SDL_Colour & bg) override = 0;
    virtual FontChar GetCharAtLoc(int loc) const override;

    // new functions
    virtual void SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg) = 0;
    virtual void GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const = 0;

    virtual void SetOffset(uint16_t offset);

    static MemoryMappedScreen * Create(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    template<class Type>
    static void AddType(const std::string & name)
    {
      if (!g_memoryMappedScreenFactory.HasKey(name))
        g_memoryMappedScreenFactory.AddConcreteClass<Type>(name);
    }

    virtual void SetCharData(FontChar ch, int line, uint8_t val, bool update = true);

    void InitUsage();
    void UpdateUsage(int loc, FontChar oldChar, FontChar newChar);

    struct CharUsage
    {
      std::set<int> m_locs;
    };

  protected:
    std::vector<uint8_t> m_memory;
    int m_memoryMask = 0;
    int m_offset;
    static MemoryMappedScreenFactory g_memoryMappedScreenFactory;

    bool m_trackUsage = false;
    std::map<FontChar, CharUsage> m_usage;
};

/////////////////////////////////////////////////////////////////////////////////

class SingleColourMemoryMappedScreen : public MemoryMappedScreen
{
  public:
    SingleColourMemoryMappedScreen(MainWindow & mainWindow, const Options & options, const Config::MemoryMappedScreen & info);

    virtual void SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg);
    virtual void GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const;

    virtual void GetColourAtLoc(int addr, SDL_Colour & fg, SDL_Colour & bg);

  protected:
    SDL_Color m_bgColour;
    SDL_Color m_fgColour;
};

/////////////////////////////////////////////////////////////////////////////////

#endif // VIRTUALSCREEN_H_
