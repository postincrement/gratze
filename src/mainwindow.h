
#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>
#include <chrono>

#include <SDL.h>

#include "video/virtual_screen.h"

#define LAZY_UPDATE_MSECS   20

class MainWindow
{
  public:
    MainWindow();
    ~MainWindow();

    bool Open(int width, int height);

    SDL_Renderer * GetRenderer();

    void Update();

    void GetScreenCharRect(SDL_Rect & rect, int x, int y, int w, int h, int hscale, int vscale);

  protected:
    int m_screenHeight;
    int m_screenWidth;
    int m_panelWidth;
    int m_left = 12;   // divisible by 2, 3, 4, 6
    int m_top = 12;    // divisible by 2, 3, 5, 6

    std::string m_title;

    SDL_Rect m_panelRect;

    SDL_Window * m_window;
    SDL_Renderer * m_renderer;

    // virtual video screen
    SDL_Rect  m_screenRect;
    std::unique_ptr<VirtualScreen> m_screen;
};

#endif // MAINWINDOW_H_
