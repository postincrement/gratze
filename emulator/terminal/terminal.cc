#include <iostream>

#include <stdlib.h>
#include <unistd.h>

#if __linux__ || __APPLE__
#include <sys/select.h>
#include <sys/types.h>
#include <curses.h>
#endif

#if _WIN32
#include <windows.h>
#include <io.h>
#endif

#include "common/misc.h"
#include "src/emulator.h"
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

void Terminal::AddPollers(Emulator & emulator)
{}

////////////////////////////////////////////////////////////////////////////////////

ConsoleTerminal::ConsoleTerminal(const Options & options, int cols, int rows)
  : Terminal(options, cols, rows)
{
  ConScreen * scrn = new ConScreen(*this, options, cols, rows);
  m_screen.reset(scrn);
  m_keyboard.reset(new ConKeyboard());

#if _WIN32
  setmode(STDOUT_FILENO, O_BINARY);
#endif
}

void ConsoleTerminal::Clear()
{}

void ConsoleTerminal::WriteChar(uint8_t ch)
{
  write(STDOUT_FILENO, &ch, 1);
}

void ConsoleTerminal::WriteString(const std::string & str)
{
  for (auto r : str)
    WriteChar(r);
}

void ConsoleTerminal::AddPollers(Emulator & emulator)
{
  emulator.AddRealTimePollDef(0.01, std::bind(&ConsoleTerminal::CheckConsoleKeyboard, this));
}

void ConsoleTerminal::CheckConsoleKeyboard()
{
  int fd = STDIN_FILENO;

  for (;;) {
#if __linux__ || __APPLE__
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeval t;
    t.tv_sec  = 0;
    t.tv_usec = 0;
    int result = select(fd+1, &fds, NULL, NULL, &t);
    if (result < 1)
      return;
    int ch = getch();
#endif

#if _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD events;
    INPUT_RECORD buffer;
    PeekConsoleInput( handle, &buffer, 1, &events );
    if (events <= 0)
      return;

    ReadConsoleInput(handle, &buffer, 1, &events);
    if (buffer.EventType != KEY_EVENT)
      return;

    int ch = buffer.Event.KeyEvent.uChar.AsciiChar;
    if (ch == 0)
      continue;
#endif

    switch (ch) {
      case 0x0a:
        ch = 0x0d;
      default:
        break;
    }

#if __linux__
    m_keyboard->OnASCIIKeyDown(ch);
#endif

#if _WIN32
    if (buffer.Event.KeyEvent.bKeyDown)
      m_keyboard->OnASCIIKeyDown(ch);
    else  
      m_keyboard->OnASCIIKeyUp(ch);
#endif
    return;
  }
}

////////////////////////////////////////////////////////////////////////////////////

ConScreen::ConScreen(Terminal & terminal, const Options & options, int cols, int rows)
  : VirtualScreen(options, cols, rows)
{
}

void ConScreen::OnUpdate()
{}

bool ConScreen::ResizeScreen()
{
  return true;
}

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
