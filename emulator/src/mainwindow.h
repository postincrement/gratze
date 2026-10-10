
#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <memory>
#include <chrono>

#include <SDL.h>

#include "video/virtual_screen.h"
#include "src/status_panel.h"

#define LAZY_UPDATE_MSECS   20

class MainWindow
{
  public:
    MainWindow();
    ~MainWindow();

    bool Open(int width, int height);

    void SetTitle(const std::string & title);

    SDL_Renderer * GetRenderer();

    void Update();

    void GetScreenCharRect(SDL_Rect & rect, int x, int y, int w, int h, int hscale, int vscale);

    void SetDrives(const std::vector<StatusDrive> & drives);
    void SetCpuHz(double hz);
    void SetMouse(int x, int y);
    bool HitReset(int x, int y) const;

  protected:
    int m_screenHeight;
    int m_screenWidth;
    int m_leftBorder = 10;
    int m_topBorder = 10;

    std::string m_title;

    SDL_Window * m_window;
    SDL_Renderer * m_renderer;
    SDL_Texture * m_screenTexture;

    // virtual video screen
    SDL_Rect  m_screenRect;
    StatusPanel m_panel;
    std::unique_ptr<VirtualScreen> m_screen;
};

#endif // MAINWINDOW_H_
