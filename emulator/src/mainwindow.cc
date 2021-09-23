

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "common/config.h"
#include "mainwindow.h"
#include "video/virtual_screen.h"

#include "SDL_FontCache/SDL_FontCache.h"

using namespace std;

MainWindow::MainWindow()
{
  m_window = nullptr;
}

MainWindow::~MainWindow()
{
  SDL_DestroyWindow(m_window);
}

bool MainWindow::Open(int width, int height)
{
  // save information
  m_screenWidth  = width;
  m_screenHeight = height;

  // calcuate screen rect
  m_screenRect = { m_leftBorder, m_topBorder, m_screenWidth, m_screenHeight };

  // create window
  int totalHeight = m_topBorder  + m_screenRect.h + m_topBorder;
  int totalWidth  = m_leftBorder + m_screenRect.w + m_leftBorder; 

  cout << "info: main window is "        << dec << totalWidth << "x" << totalHeight << endl;
  cout << "info: virtual screen is "     << width << "x" << height << endl;
  cout << "info: window screen area is " << m_screenRect.w << "x" << m_screenRect.h << endl;

  if (m_window == nullptr) {
    m_window = SDL_CreateWindow(m_title.c_str(), 
                                SDL_WINDOWPOS_CENTERED, 
                                SDL_WINDOWPOS_CENTERED, 
                                totalWidth, totalHeight, 
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); 
    // create renderer
    m_renderer = SDL_CreateRenderer(m_window, -1,     
                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  }
  else {
    SDL_SetWindowSize(m_window, totalWidth, totalHeight);
    SDL_DestroyTexture(m_screenTexture);
  }

  // create texture
  m_screenTexture = SDL_CreateTexture(m_renderer, 
                              SDL_PIXELFORMAT_RGBA32, 
                              SDL_TEXTUREACCESS_TARGET, 
                              m_screenRect.w, 
                              m_screenRect.h);

  Update();

  return true;
}

void MainWindow::GetScreenCharRect(SDL_Rect & rect, int x, int y, int w, int h, int hscale, int vscale)
{
  rect = { x, y, w, h };
}

SDL_Renderer * MainWindow::GetRenderer()
{
  return m_renderer;
}

void MainWindow::Update()
{
  SDL_SetRenderTarget(m_renderer, NULL);
  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);

  SDL_SetRenderTarget(m_renderer, NULL);
  SDL_RenderCopy(m_renderer, m_screenTexture, NULL, &m_screenRect);
  SDL_RenderPresent(m_renderer);

  SDL_SetRenderTarget(m_renderer, m_screenTexture);
}

