#include <iostream>

#include "common/misc.h"
#include "terminal/terminal.h"

using namespace std;

Terminal::Terminal(const Options & options, int cols, int rows)
  : m_cols(cols)
  , m_rows(rows)
{
}

bool Terminal::Open()
{
  m_cursorX = 0;
  m_cursorY = 0;
  m_screen->SetCursorPos(m_cursorX, m_cursorY);
  m_screen->EnableCursor(true);
  return true;
}

void Terminal::SetKeyboardHandler(std::function<void (uint8_t)> handler)
{
  m_kbHandler = handler;
  m_keyboard->SetHandler(true, handler);
}

void Terminal::WriteString(const std::string & str)
{
  for (auto & r : str)
    WriteChar(r, false);
  Update(true);  
}

void Terminal::Clear()
{
  cout << "screen cleared" << endl;
  for (int loc = 0; loc < m_cols*m_rows; ++loc)
    m_screen->SetCharAtLoc(loc, ' ');
  m_screen->RefreshScreen();  
  Update(true);
}

void Terminal::WriteChar(uint8_t ch)
{
  WriteChar(ch, true);
}

void Terminal::WriteChar(uint8_t ch, bool update)
{
  if (ch == 0x0d) {  // CR
    m_cursorX = 0;
    m_screen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if (ch == 0x0a) { // LF
    if (m_cursorY == m_rows-1) {
      Scroll(1);
    } 
    else {  
      m_cursorY++;
    }
    m_screen->SetCursorPos(m_cursorX, m_cursorY);
  }
  else if ((ch >= 0x20) && (ch <= 0x7e)) {
    int loc = m_screen->MapPosToLoc(m_cursorX, m_cursorY);
    m_screen->SetCharAtLoc(loc, ch);
    m_screen->RefreshCharAtLoc(loc);
    if (m_cursorX < m_cols-1) {
      m_cursorX++;
    }
    else {
      m_cursorX = 0;
      if (m_cursorY == m_rows-1) {
        Scroll(1);
      }
      else {
        ++m_cursorY;
      }
    }
    m_screen->SetCursorPos(m_cursorX, m_cursorY);
    Update(update);
  }
}

void Terminal::Update(bool hasChanged)
{
  Update(hasChanged);
}

void Terminal::Scroll(int lines)
{
  // move the chars
  int srcAddr = m_cols * lines;
  int dstAddr = 0;
  int len     = m_cols * (m_rows - lines);
  FontChar ch;
  for (int i = 0; i < len; ++i) {
    m_screen->SetCharAtLoc(dstAddr++, m_screen->GetCharAtLoc(srcAddr++));
  }
  ClearToEndOfLine(0, m_rows-1);
  m_screen->RefreshScreen();
}

void Terminal::ClearToEndOfLine(int col, int line)
{
  int addr = col + (line * m_cols);
  while (col++ < m_cols) {
    m_screen->SetCharAtLoc(addr++, ' ');
  }  
}

////////////////////////////////////////////////////////////////////////////////////

ConsoleTerminal::ConsoleTerminal(const Options & options, int cols, int rows)
  : Terminal(options, cols, rows)
{
  ConScreen * scrn = new ConScreen(*this, options, cols, rows);
  m_screen.reset(scrn);
  m_keyboard.reset(new ConKeyboard());
}

void ConsoleTerminal::Clear()
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

ConScreen::ConScreen(Terminal & terminal, const Options & options, int cols, int rows)
  : VirtualScreen(options, cols, rows)
{
}

void ConScreen::OnUpdate()
{}

bool ConScreen::ResizeScreen()
{}

bool ConScreen::SetFont(Font * font, int cols, int rows) 
{ return true; }

void ConScreen::RenderCharAtPos(int x, int y, bool withCursor, bool update)
{}

////////////////////////////////////////////////////////////////////////////////////

ConKeyboard::ConKeyboard()
{}

void ConKeyboard::Reset()
{}

void ConKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{}

void ConKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{}

////////////////////////////////////////////////////////////////////////////////////

SDLTerminal::SDLTerminal(MainWindow & mainWindow, const Options & options, int cols, int rows)
  : Terminal(options, cols, rows)
{
  m_screen.reset(new SDLVirtualScreen(mainWindow, options, cols, rows));
  m_keyboard.reset(new ParallelKeyboard());
}

bool SDLTerminal::Open()
{
  if (!Terminal::Open())
    return false;

  m_cursorX = 0;
  m_cursorY = 0;
  Clear();
  return true;
}

void SDLTerminal::Update(bool hasChanged)
{
  m_screen->Update(hasChanged);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
