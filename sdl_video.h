#ifndef SDL_VIDEO_H_
#define SDL_VIDEO_H_

#include <SDL2/SDL.h> 
//#include <SDL2/SDL_image.h> 
//#include <SDL2/SDL_timer.h> 

#include <vector>
#include <memory>

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

    bool Open();

    void Clear();

    void WriteChar(unsigned offset, uint8_t ch);

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

    std::vector<uint8_t> m_memory;
    std::unique_ptr<Font> m_font;

    uint32_t m_backgroundColour;
    uint32_t m_foregroundColour;

    SDL_Window * m_window = NULL;
    SDL_Surface * m_winSurface = NULL;
    SDL_Surface * m_fontSurface = NULL;
};

/////////////////////////////////////////////////////

#endif // SDL_VIDEO_H_
