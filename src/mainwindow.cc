

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "config.h"
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
  cout << "info: window screen is " << width << "x" << height << endl;

  // save information
  m_screenWidth  = width;
  m_screenHeight = height;

  // side panel width
  m_panelWidth = 200;

  // calcuate screen rect
  m_screenRect = { 0, 0, m_screenWidth, m_screenHeight };

  cout << "info: window screen area is " << m_screenRect.w << "x" << m_screenRect.h << endl;

  // calculate panel rect
  m_panelRect  = { m_screenRect.w, 0, m_panelWidth, m_screenRect.h };

  // create window
  int totalHeight = m_screenRect.h;
  int totalWidth  = m_screenRect.w + m_panelWidth; 

  cerr << "info: main window is " << dec << totalWidth << "x" << totalHeight << endl;

  if (m_window == nullptr) {
    m_window = SDL_CreateWindow(m_title.c_str(), 
                                SDL_WINDOWPOS_CENTERED, 
                                SDL_WINDOWPOS_CENTERED, 
                                totalWidth, totalHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); 
    // create renderer
    m_renderer = SDL_CreateRenderer(m_window, -1, 0);

    cout << "window created" << endl;
  }
  else {
    //SDL_DestroyRenderer(m_renderer);
    SDL_SetWindowSize(m_window, totalWidth, totalHeight);
    cout << "window resized" << endl;
  }

  SDL_Color bg = { 0, 0, 0 };
  SDL_SetRenderDrawColor(m_renderer, bg.r, bg.g, bg.b, 255);

  // set border color
//  SDL_Rect borderRect = { 0, 0, m_screenRect.w, screenAreaHeight };
//  SDL_RenderFillRect(m_renderer, &borderRect);

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
           m_screenRect.x + x,
           m_screenRect.y + y, 
           w, 
           h 
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

