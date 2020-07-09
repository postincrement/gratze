#include <iostream>

#include "common/misc.h"
#include "terminal/terminal.h"

using namespace std;

Terminal::Terminal(const Options & options, int cols, int rows)
  : m_cols(cols)
  , m_rows(rows)
{
  m_cursorX = 0;
  m_cursorY = 0;
  m_cursorEnabled = true;
}

bool Terminal::Open()
{
  m_cursorX = 0;
  m_cursorY = 0;
  m_cursorEnabled = true;
}

void Terminal::SetKeyboardHandler(std::function<void (uint8_t)> handler)
{
  m_kbHandler = handler;
}

////////////////////////////////////////////////////////////////////////////////////

ConsoleTerminal::ConsoleTerminal(const Options & options, int cols, int rows)
  : Terminal(options, cols, rows)
{
  ConScreen * scrn = new ConScreen(*this, options, cols, rows);
  m_screen.reset(scrn);
  //  );
  m_keyboard.reset(new ConKeyboard());
}

bool ConsoleTerminal::Open()
{
  return true;
}

void ConsoleTerminal::Clear()
{}

void ConsoleTerminal::Update(bool hasChanged)
{}

void ConsoleTerminal::WriteChar(uint8_t ch)
{
  cout << ch;
}

void ConsoleTerminal::WriteString(const std::string & str)
{
  cout << str;
}

////////////////////////////////////////////////////////////////////////////////////

ConsoleTerminal::ConScreen::ConScreen(Terminal & terminal, const Options & options, int cols, int rows)
  : VirtualScreen(options, cols, rows)
{
}

void ConsoleTerminal::ConScreen::Update(bool hasChanged) 
{}

void ConsoleTerminal::ConScreen::SetScale(int hscale, int vscale) 
{}

bool ConsoleTerminal::ConScreen::SetFont(Font * font, int cols, int rows) 
{ return true; }

int ConsoleTerminal::ConScreen::MapPosToLoc(int x, int y)
{ return 0; }

void ConsoleTerminal::ConScreen::SetCursorPos(int x, int y)
{}

void ConsoleTerminal::ConScreen::EnableCursor(bool enable)
{}

////////////////////////////////////////////////////////////////////////////////////

ConsoleTerminal::ConKeyboard::ConKeyboard()
{}

void ConsoleTerminal::ConKeyboard::Reset()
{}

void ConsoleTerminal::ConKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{}

void ConsoleTerminal::ConKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{}


////////////////////////////////////////////////////////////////////////////////////

SDLTerminal::SDLTerminal(MainWindow & mainWindow, const Options & options, int cols, int rows)
  : Terminal(options, cols, rows)
{
  m_sdlScreen.reset(new SDLScreen(*this, mainWindow, options, cols, rows));
  m_screen = m_sdlScreen;

  m_keyboard.reset(new ParallelKeyboard());
}

bool SDLTerminal::Open()
{
  if (!Terminal::Open())
    return false;

  m_sdlScreen->SetCursorPos(m_cursorX, m_cursorY);
  m_sdlScreen->EnableCursor(true);

  return true;
}

void SDLTerminal::Update(bool hasChanged)
{
  m_sdlScreen->Update(hasChanged);
}

void SDLTerminal::SetKeyboardHandler(std::function<void (uint8_t)> handler)
{
  Terminal::SetKeyboardHandler(handler);
  m_kbHandler = handler;
}

void SDLTerminal::Clear()
{
}

void SDLTerminal::WriteString(const std::string & str)
{
  for (auto & r : str)
    WriteChar(r);
}

void SDLTerminal::WriteChar(uint8_t ch)
{
  if (ch == 0x0d) {  // CR
    m_cursorX = 0;
    m_sdlScreen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if (ch == 0x0a) { // LF
    if (m_cursorY == m_rows-1) {
      m_sdlScreen->Scroll(1);
    } 
    else {  
      m_cursorY++;
    }
    m_sdlScreen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if ((ch >= 0x20) && (ch <= 0x7e)) {
    int loc = m_screen->MapPosToLoc(m_cursorX, m_cursorY);
    m_sdlScreen->SetCharAtLoc(loc, ch);
    m_sdlScreen->RefreshCharAtLoc(loc);
    if (m_cursorX < m_cols-1) {
      m_cursorX++;
    }
    else {
      m_cursorX = 0;
      if (m_cursorY == m_rows-1) {
        m_sdlScreen->Scroll(1);
      }
      else {
        ++m_cursorY;
      }
    }
    m_sdlScreen->SetCursorPos(m_cursorX, m_cursorY);
    Update(true);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SDLTerminal::SDLScreen::SDLScreen(SDLTerminal & terminal, MainWindow & mainWindow, const Options & options, int cols, int rows)
  : SDLVirtualScreen(mainWindow, options, cols, rows)
  , m_terminal(terminal)
{
  m_fg = { 0, 255, 0, 255 };
  m_bg = { 0, 0, 0, 0 };
  m_chars.resize(cols * rows);

  for (int i = 0; i < m_rows*m_cols; ++i)
    SetCharAtLoc(i, ' ');
}

bool SDLTerminal::SDLScreen::Open()
{
  if (!VirtualScreen::Open())
    return false;

  return m_terminal.Open();
}

FontChar SDLTerminal::SDLScreen::GetCharAtPos(int x, int y)
{
  int addr = (y * m_cols) + x;
  return m_chars[addr].m_ch;
}

void SDLTerminal::SDLScreen::SetCharAtLoc(int loc, uint8_t ch)
{
  m_chars[loc].m_ch = ch;
  m_chars[loc].m_fg = m_fg;
  m_chars[loc].m_bg = m_bg;
}


void SDLTerminal::SDLScreen::GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg)
{
  int addr = (y * m_cols) + x;
  fg = m_chars[addr].m_fg;
  bg = m_chars[addr].m_bg;
}

void SDLTerminal::SDLScreen::Scroll(int lines)
{
  // move the chars
  int srcAddr = m_cols * lines;
  int dstAddr = 0;
  int len     = m_cols * (m_rows - lines);
  for (int i = i; i < len; ++i)
    m_chars[dstAddr++] = m_chars[srcAddr++];
  ClearToEndOfLine(0, m_rows-1);
  RefreshScreen();
}

void SDLTerminal::SDLScreen::ClearToEndOfLine(int col, int line)
{
  int addr = col + (line * m_cols);
  while (col++ < m_cols) {
    SetCharAtLoc(addr++, ' ');
  }  
}
