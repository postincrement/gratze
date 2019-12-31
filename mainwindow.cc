

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "mainwindow.h"
#include "virtual_screen.h"

#include "SDL_FontCache/SDL_FontCache.h"

using namespace std;

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
  SDL_DestroyWindow(m_window);
}

bool MainWindow::Open(int scale, int screenWidth, int screenHeight)
{
  // save information
  m_scale        = scale;
  m_screenWidth  = screenWidth;
  m_screenHeight = screenHeight;

  // side panel width
  m_panelWidth = 200;

  // set screen borders
  m_left   = 10;
  m_right  = 10;
  m_top    = 10;
  m_bottom = 10;

  // calcuate screen rect
  m_screenRect = { m_left * m_scale, 
                   m_top  * m_scale, 
                   m_screenWidth  * m_scale, 
                   m_screenHeight * m_scale };

  int screenAreaWidth  = (m_left + m_screenWidth  + m_right)  * m_scale;
  int screenAreaHeight = (m_top  + m_screenHeight + m_bottom) * m_scale;

  // calculate panel rect
  m_panelRect  = { screenAreaWidth, 0, m_panelWidth, screenAreaHeight };

  // create window
  int height = screenAreaHeight;
  int width  = screenAreaWidth + m_panelWidth; 
  m_window = SDL_CreateWindow(m_title.c_str(), 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED, 
                              width, height, SDL_WINDOW_SHOWN); 

  // create renderer
  m_renderer = SDL_CreateRenderer(m_window, -1, 0);

  SDL_Color bg = { 0, 0, 0 };
  SDL_SetRenderDrawColor(m_renderer, bg.r, bg.g, bg.b, 255);

  // set border color
  SDL_Rect borderRect = { 0, 0, screenAreaWidth, screenAreaHeight };
  SDL_RenderFillRect(m_renderer, &borderRect);

  // set panel color
  SDL_Color panelColor = { 80,   80,   80 };
  SDL_SetRenderDrawColor(m_renderer, panelColor.r, panelColor.g, panelColor.b, 255);
  SDL_RenderFillRect(m_renderer, &m_panelRect);

  Update();

  return true;
}

void MainWindow::GetScreenCharRect(SDL_Rect & rect, int x, int y, int w, int h)
{
  rect = { 
           m_screenRect.x + x * m_scale,
           m_screenRect.y + y * m_scale, 
           w * m_scale, 
           h * m_scale 
         };
}

SDL_Renderer * MainWindow::GetRenderer()
{
  return m_renderer;
}

void MainWindow::Update()
{
  SDL_RenderPresent(m_renderer);
}


//void Emulator::DrawText(int x, int y, const char * text)
//{
//  m_video->DrawText(x, y, text);
//}

