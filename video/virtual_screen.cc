
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include "SDL_FontCache/SDL_FontCache.h"

#include "misc.h"
#include "config.h"
#include "virtual_screen.h"
#include "mainwindow.h"
#include "cpu/emulator.h"

using namespace std;

VirtualScreenFactory VirtualScreen::g_virtualScreenFactory;

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen * VirtualScreen::Create(MainWindow & mainWindow, 
                                        Emulator & emulator, 
                                   const Options & options, 
                             const Config::Video & info)
{
  VirtualScreen * screen = g_virtualScreenFactory.CreateInstance(info.m_name, mainWindow, emulator, options, info);
  return screen;
}  

/////////////////////////////////////////////////////////////////////////////////

TextWindow::TextWindow(MainWindow & mainWindow, int rows, int cols, int width, int height)
  : m_mainWindow(mainWindow)
  , m_rows(rows)
  , m_cols(cols)
  , m_width(width)
  , m_height(height)
  , m_colScale(1)
{
  m_lazyUpdates = true;
  m_dirty = true;
  m_updateTimer = std::chrono::system_clock::now();

  m_visibleSize = rows * cols;
  m_visibleMask = m_visibleSize - 1;
  cout << "info: text window is " << m_width << " x " << m_height << " pixels, " << m_visibleSize << " chars, mask is " << HEXFORMAT0x4(m_visibleMask) << endl;
}

bool TextWindow::Open()
{
  return true;
}

void TextWindow::SetColScale(int scale)
{
  m_colScale = scale;
}

void TextWindow::RenderCharAtPos(int pos, SDL_Colour & fg, SDL_Colour & bg)
{ 
  if (m_font) {
    pos = (pos & m_visibleMask);
    SDL_Rect dstRect;
    int x = pos % (m_cols / m_colScale);
    int y = pos / m_cols;
    m_mainWindow.GetScreenCharRect(dstRect, 
                          m_left + (x * m_font->GetWidth() * m_colScale), 
                          m_top + y * m_font->GetHeight(), 
                          m_font->GetWidth() * m_colScale,
                          m_font->GetHeight());

    SDL_RenderSetScale(m_mainWindow.GetRenderer(), m_hscale, m_vscale);    
    m_font->RenderChar(GetCharAtPos(x, y), m_mainWindow.GetRenderer(), dstRect, fg, bg);

    Update(true);
  }
}

void TextWindow::Update(bool hasChanged)
{
  if (!m_lazyUpdates) {
    if (!hasChanged || m_dirty)
      m_mainWindow.Update();
    m_dirty = false;  
    return;
  }

  auto now = std::chrono::system_clock::now();
  if (!m_dirty && hasChanged) {
    m_updateTimer = now + std::chrono::milliseconds(LAZY_UPDATE_MSECS);
    m_dirty = true;
  }

  if (m_dirty && (now > m_updateTimer)) {
    m_mainWindow.Update();
    m_dirty = false;
  }
}

/////////////////////////////////////////////////////////////////////////////////

VirtualScreen::VirtualScreen(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : TextWindow(mainWindow, info.m_screenRows, info.m_screenCols, info.m_screenWidth, info.m_screenHeight)
{
  m_hscale = 1.0;
  m_vscale = 1.0;
}

void VirtualScreen::SetScale(double hscale, double vscale)
{
  m_hscale = hscale;
  m_vscale = vscale;
}

void VirtualScreen::SetOffset(int left, int top)
{
  m_left = left;
  m_top  = top;
}

void VirtualScreen::RefreshScreen()
{
  for (int y = 0; y < m_rows; ++y)
    for (int x = 0; x < m_cols / m_hscale ; ++x)
      RefreshCharAtPos(x, y);
}

VirtualScreen::~VirtualScreen()
{
}

/////////////////////////////////////////////////////////////////////////////////

static unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

static bool UnpackCharacterGeneratorFont(const Config::Font & config, std::vector<uint8_t> & fontData)
{
  if (config.m_charGen == nullptr) {
    cerr << "error: no character generator found" << endl;
    return false;
  }

  CharacterGeneratorROM & charGen = *config.m_charGen;

  if (config.m_width < charGen.m_width) {
    cerr << "error: font width " << (int)config.m_width << " cannot be less than charactr generator width " << (int)charGen.m_width << endl;
    return false;
  }

  if (config.m_height < charGen.m_height) {
    cerr << "error: font height " << (int)config.m_height << " cannot be less than charactr generator height " << (int)charGen.m_height << endl;
    return false;
  }

  if (charGen.m_stride < charGen.m_height) {
    cerr << "error: chargen stride " << (int)charGen.m_stride << " cannot be less than charactr generator height " << (int)charGen.m_height << endl;
    return false;
  }

  if (charGen.m_width > 8) {
    cerr << "error: character generators > 8 pixels wide not supported" << endl;
    return false;
  }

  fontData.resize(config.m_count * config.m_height * ((config.m_width + 7) / 8));

  int shiftBits = config.m_width - charGen.m_width;

  for (int c = 0; c < charGen.m_count; ++c) {
    const uint8_t * src = charGen.m_data + c * charGen.m_stride;
    uint8_t * dst = &fontData[c * config.m_height];

    for (int y = 0; y < charGen.m_height; ++y) {
      *dst++ = charGen.m_reverse ? 
                    reverse(*src++) >> (8 - charGen.m_width - shiftBits) :
                    *src++ << shiftBits
                    ;
    }
  }

  return true;
}


MemoryMappedVideo::MemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : VirtualScreen(mainWindow, emulator, options, info)
  , m_offset(0)
  , m_fontConfig(info.m_font)
  , m_options(options)
{
  // create video memory
  int size_bytes = info.m_endAddr - info.m_startAddr + 1;
  m_memory.resize(size_bytes);
  m_memoryMask = m_memory.size() - 1;

  cout << "info: video memory size is " << size_bytes << " bytes, mask is " << HEXFORMAT0x4(m_memoryMask) << endl;
}

MemoryMappedVideo::~MemoryMappedVideo()
{}

bool MemoryMappedVideo::Open()
{
  return SetFont(m_fontConfig);
}

bool MemoryMappedVideo::SetFont(const Config::Font & font)
{
  m_fontConfig = font;
  m_font.reset();

  // unpack pixel data
  if (m_fontConfig.m_charGen != NULL) {
    if (!UnpackCharacterGeneratorFont(m_fontConfig, m_fontData))
      return false;
  }

  // do extra steps if required
  if (m_fontConfig.m_creator != NULL) {
    if (!(m_fontConfig.m_creator)(m_options, m_fontConfig, m_fontData)) {
      cerr << "error: font creator returned error" << endl;
      return false;
    }
  }

  m_font.reset(new PixelFont(m_fontConfig, &m_fontData[0]));
  if (!m_font) {
    cerr << "error: font could not be opened" << endl;
    return false;
  }

  if (m_mainWindow.GetRenderer() == nullptr) {
    cerr << "error: renderer not available" << endl;
    return false;
  }

  if (!m_font->Open(m_mainWindow.GetRenderer())) {
    cerr << "error: font could not be opened" << endl;
    return false;
  }

  cout << "info: new font set" << endl;

  return VirtualScreen::Open();
}

void MemoryMappedVideo::WriteMemoryAtAddress(int addr, uint8_t ch)
{
  if ((addr >= m_memory.size()) || (m_memory[addr] == ch)) {
    return;
  }

  m_memory[addr] = ch;
  RenderCharAtAddress(addr);
}

uint8_t MemoryMappedVideo::ReadMemoryAtAddress(int addr) const
{
  if (addr >= m_memory.size()) {
    return 0x00;
  }

  return m_memory[addr];
}

FontChar MemoryMappedVideo::GetCharAtPos(int x, int y)
{
  int pos = y * m_cols + x;
  return m_memory[pos & m_visibleMask];
}

void MemoryMappedVideo::RenderCharAtAddress(int addr)
{
  addr = addr & m_visibleMask;
  int col = addr % m_cols;
  int row = addr / m_cols;
  RenderCharAtPos(col, row, m_fgColour, m_bgColour);
}

/////////////////////////////////////////////////////////////////////////////////

SingleColourMemoryMappedVideo::SingleColourMemoryMappedVideo(MainWindow & mainWindow, Emulator & emulator, const Options & options, const Config::Video & info)
  : MemoryMappedVideo(mainWindow, emulator, options, info)
{
  m_fgColour = { 0xff, 0xff, 0xff, 0xff };
  m_bgColour = { 0, 0, 0, 0 };
}

void SingleColourMemoryMappedVideo::SetFontColour(const SDL_Colour & fg, const SDL_Colour & bg)
{
  m_fgColour = fg;
  m_bgColour = bg;

  RefreshScreen();
}

void SingleColourMemoryMappedVideo::GetFontColour(SDL_Colour & fg, SDL_Colour & bg) const
{
  fg = m_fgColour;
  bg = m_bgColour;
}

void SingleColourMemoryMappedVideo::RenderCharAtAddress(int addr)
{
  MemoryMappedVideo::RenderCharAtAddress(addr, m_fgColour, m_bgColour);
}

