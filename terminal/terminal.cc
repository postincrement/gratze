#include <iostream>


#include "terminal/terminal.h"

using namespace std;


Terminal::Terminal(MainWindow & mainWindow, const Options & options, int cols, int rows)
  : m_cols(cols)
  , m_rows(rows)
{
  m_cursorX = 0;
  m_cursorY = 0;
  m_cursorEnabled = true;

  m_screen.reset(new Screen(*this, mainWindow, options, cols, rows));
}


bool Terminal::Open()
{
  m_cursorX = 0;
  m_cursorY = 0;
  m_screen->SetCursorPos(m_cursorX, m_cursorY);
  m_screen->EnableCursor(true);
}

void Terminal::Update(bool hasChanged)
{
  m_screen->Update(hasChanged);
}

void Terminal::Clear()
{
}

void Terminal::WriteString(const std::string & str)
{
  for (auto & r : str)
    WriteChar(r);
}

void Terminal::WriteChar(uint8_t ch)
{
  if (ch == 0x0d) {  // CR
    m_cursorX = 0;
    m_screen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if (ch == 0x0a) { // LF
    m_cursorY++;
    if (m_cursorY == m_rows) {
      m_cursorY = 0;
    }
    m_screen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if ((ch >= 0x20) && (ch <= 0x7e)) {
    int newCursorX = m_cursorX+1;
    m_screen->SetCursorPos(newCursorX, m_cursorY);
    int loc = m_screen->MapPosToLoc(m_cursorX, m_cursorY);
    m_screen->m_chars[loc].m_ch = ch;
    m_screen->RenderCharAtLoc(loc);
    m_cursorX = newCursorX;
    Update(true);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Terminal::Screen::Screen(Terminal & terminal, MainWindow & mainWindow, const Options & options, int cols, int rows)
  : VirtualScreen(mainWindow, options, cols, rows)
  , m_terminal(terminal)
{
  m_chars.resize(cols * rows);
}

bool Terminal::Screen::Screen::Open()
{
  return m_terminal.Open();
}

FontChar Terminal::Screen::GetCharAtPos(int x, int y)
{
  int addr = (y * m_cols) + x;
  return m_chars[addr].m_ch;
}

void Terminal::Screen::GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg)
{
  int addr = (y * m_cols) + x;
  fg = m_chars[addr].m_fg;
  bg = m_chars[addr].m_bg;
}

