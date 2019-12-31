
#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>
#include <chrono>

#include <SDL.h>

#include "virtual_screen.h"

#define LAZY_UPDATE_MSECS   20

class MainWindow
{
  public:
    MainWindow();
    ~MainWindow();

    bool Open(int scale, int screenWidth, int screenHeight);

    SDL_Renderer * GetRenderer();

    void Update();

    void GetScreenCharRect(SDL_Rect & rect, int x, int y, int w, int h);

  protected:
    int m_scale;
    int m_screenHeight;
    int m_screenWidth;
    int m_panelWidth;
    int m_left;
    int m_right;
    int m_top;
    int m_bottom;

    std::string m_title;

    SDL_Rect m_panelRect;

    SDL_Window * m_window;
    SDL_Renderer * m_renderer;

    // virtual video screen
    SDL_Rect  m_screenRect;
    std::unique_ptr<VirtualScreen> m_screen;
};

#endif // MAINWINDOW_H_
