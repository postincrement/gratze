#ifndef STATUS_PANEL_H_
#define STATUS_PANEL_H_

#include <string>
#include <vector>

#include <SDL.h>

struct FC_Font;

// One disk shown on the status panel under the emulated screen.
struct StatusDrive
{
  std::string m_label;
  std::string m_name;
  bool m_mounted = false;
  bool m_selected = false;
  bool m_busy = false;
};

// Controls and drive lights drawn beneath the emulated image.
class StatusPanel
{
  public:
    static const int kHeight = 44;

    ~StatusPanel();

    void Open(SDL_Renderer * renderer);
    void SetDrives(const std::vector<StatusDrive> & drives);
    void SetCpuHz(double hz);
    void SetMouse(int x, int y);
    void Draw(SDL_Renderer * renderer, const SDL_Rect & area);
    bool HitReset(int x, int y) const;

  protected:
    void DrawReset(SDL_Renderer * renderer, const SDL_Rect & area);
    void DrawDrives(SDL_Renderer * renderer, const SDL_Rect & area, int x, int limit);
    int DrawSpeed(SDL_Renderer * renderer, const SDL_Rect & area);

    FC_Font * m_font = nullptr;
    std::vector<StatusDrive> m_drives;
    double m_cpuHz = 0;
    SDL_Rect m_resetRect = { 0, 0, 0, 0 };
    int m_mouseX = -1;
    int m_mouseY = -1;
};

#endif // STATUS_PANEL_H_
