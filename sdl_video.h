#ifndef SDL_VIDEO_H_
#define SDL_VIDEO_H_

#include <SDL2/SDL.h> 

#include <vector>
#include <memory>
#include <chrono>

#define USE_TEXTURES    1

class MemoryMappedVideo
{
  public:
    class Font
    {
      public:
        Font(int width, int height, uint8_t * data);

        int GetWidth() const
        { return m_width; }

        int GetHeight() const
        { return m_height; }

        SDL_Surface * GetSurface();

      protected:  
        int m_width;
        int m_height;
        uint8_t * m_data;
    };

    MemoryMappedVideo(int rows, int cols, int scale, Font * font, uint8_t * data = NULL);
    ~MemoryMappedVideo();

    bool SetLazyUpdate(bool v);

    bool Open(const std::string & title);

    void Clear();

    void WriteChar(unsigned offset, uint8_t ch);

    void Update(bool hasChanged = false);

  protected:
    int m_rows;
    int m_cols;
    int m_scale;
    int m_charWidth;
    int m_charHeight;
    int m_top;
    int m_bottom;
    int m_left;
    int m_right;

    int m_panelWidth;

    bool m_lazyUpdates;
    bool m_dirty;
    std::chrono::system_clock::time_point m_updateTimer;

    SDL_Rect m_screenRect;
    SDL_Rect m_panelRect;

    std::vector<uint8_t> m_memory;
    std::unique_ptr<Font> m_font;

    SDL_Window * m_window = NULL;

#if USE_TEXTURES
    SDL_Renderer * m_renderer;
    SDL_Texture  * m_fontTexture;
    SDL_Color m_backgroundColour;
    SDL_Color m_foregroundColour;
#else       
    SDL_Surface * m_winSurface = NULL;
    SDL_Surface * m_fontSurface = NULL;
    uint32_t m_backgroundColour;
    uint32_t m_foregroundColour;
#endif    
};

/////////////////////////////////////////////////////

#endif // SDL_VIDEO_H_
