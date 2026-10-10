#include <cstdio>
#include <unistd.h>

#include "SDL_FontCache/SDL_FontCache.h"
#include "src/status_panel.h"

namespace {

const char * PanelFont()
{
  static const char * fonts[] = {
#ifdef __APPLE__
    "/System/Library/Fonts/SFNSMono.ttf",
    "/System/Library/Fonts/Monaco.ttf",
    "/System/Library/Fonts/Supplemental/Courier New.ttf",
#endif
    "./fonts/UbuntuMono-R.ttf",
    nullptr
  };

  for (const char ** name = fonts; *name != nullptr; ++name) {
    if (access(*name, R_OK) == 0)
      return *name;
  }
  return nullptr;
}

bool Inside(const SDL_Rect & rect, int x, int y)
{
  return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

std::string ShortName(const std::string & name)
{
  if (name.size() <= 22)
    return name;
  return name.substr(0, 20) + "..";
}

} // namespace

StatusPanel::~StatusPanel()
{
  if (m_font != nullptr) {
    FC_FreeFont(m_font);
    m_font = nullptr;
  }
}

void StatusPanel::Open(SDL_Renderer * renderer)
{
  if (m_font != nullptr || renderer == nullptr)
    return;

  const char * font = PanelFont();
  if (font == nullptr)
    return;

  m_font = FC_CreateFont();
  if (m_font == nullptr)
    return;
  if (!FC_LoadFont(m_font, renderer, font, 14, FC_MakeColor(230, 230, 230, 255), TTF_STYLE_NORMAL)) {
    FC_FreeFont(m_font);
    m_font = nullptr;
  }
}

void StatusPanel::SetDrives(const std::vector<StatusDrive> & drives)
{
  m_drives = drives;
}

void StatusPanel::SetCpuHz(double hz)
{
  m_cpuHz = hz;
}

void StatusPanel::SetMouse(int x, int y)
{
  m_mouseX = x;
  m_mouseY = y;
}

bool StatusPanel::HitReset(int x, int y) const
{
  return Inside(m_resetRect, x, y);
}

void StatusPanel::DrawReset(SDL_Renderer * renderer, const SDL_Rect & area)
{
  m_resetRect = { area.x + 10, area.y + 8, 72, area.h - 16 };
  bool hover = Inside(m_resetRect, m_mouseX, m_mouseY);
  if (hover)
    SDL_SetRenderDrawColor(renderer, 78, 78, 90, 255);
  else
    SDL_SetRenderDrawColor(renderer, 54, 54, 62, 255);
  SDL_RenderFillRect(renderer, &m_resetRect);
  SDL_SetRenderDrawColor(renderer, 168, 168, 180, 255);
  SDL_RenderDrawRect(renderer, &m_resetRect);

  if (m_font != nullptr)
    FC_Draw(m_font, renderer, (float)(m_resetRect.x + 14), (float)(m_resetRect.y + 4), "Reset");
}

int StatusPanel::DrawSpeed(SDL_Renderer * renderer, const SDL_Rect & area)
{
  char text[32];
  std::snprintf(text, sizeof text, "%.3f MHz", m_cpuHz / 1.0e6);
  if (m_font == nullptr)
    return area.x + area.w - 8;

  int width = (int)FC_GetWidth(m_font, "%s", text);
  int x = area.x + area.w - width - 12;
  FC_DrawColor(m_font, renderer, (float)x, (float)(area.y + 12), FC_MakeColor(230, 230, 230, 255), "%s", text);
  return x - 12;
}

void StatusPanel::DrawDrives(SDL_Renderer * renderer, const SDL_Rect & area, int x, int limit)
{
  for (const StatusDrive & drive : m_drives) {
    if (x >= limit)
      break;

    SDL_Rect led = { x, area.y + (area.h - 12) / 2, 12, 12 };
    if (drive.m_busy)
      SDL_SetRenderDrawColor(renderer, 70, 210, 90, 255);
    else if (drive.m_selected)
      SDL_SetRenderDrawColor(renderer, 220, 170, 50, 255);
    else if (drive.m_mounted)
      SDL_SetRenderDrawColor(renderer, 50, 110, 60, 255);
    else
      SDL_SetRenderDrawColor(renderer, 70, 70, 76, 255);
    SDL_RenderFillRect(renderer, &led);

    x += 18;
    if (m_font == nullptr)
      continue;

    std::string text = drive.m_label;
    if (!drive.m_name.empty())
      text += " " + ShortName(drive.m_name);
    else if (!drive.m_mounted)
      text += " empty";

    SDL_Color colour = drive.m_mounted ? FC_MakeColor(230, 230, 230, 255) : FC_MakeColor(140, 140, 148, 255);
    FC_DrawColor(m_font, renderer, (float)x, (float)(area.y + 12), colour, "%s", text.c_str());
    x += (int)FC_GetWidth(m_font, "%s", text.c_str()) + 18;
  }
}

void StatusPanel::Draw(SDL_Renderer * renderer, const SDL_Rect & area)
{
  SDL_SetRenderDrawColor(renderer, 28, 28, 32, 255);
  SDL_RenderFillRect(renderer, &area);
  SDL_SetRenderDrawColor(renderer, 80, 80, 90, 255);
  SDL_RenderDrawLine(renderer, area.x, area.y, area.x + area.w, area.y);

  DrawReset(renderer, area);
  int speedLeft = DrawSpeed(renderer, area);
  DrawDrives(renderer, area, m_resetRect.x + m_resetRect.w + 18, speedLeft);
}
