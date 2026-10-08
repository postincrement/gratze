#include <iostream>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>

#if __linux__ || __APPLE__
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <signal.h>
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

Terminal::~Terminal()
{}

#if __linux__ || __APPLE__
static struct termios g_originalTermios;
static bool g_haveOriginalTermios = false;
static bool g_consoleActive = false;

// Defined in noca_mode.cc. term.h's macros collide with the rest of this file.
extern void DisableAlternateScreen();
extern void LeaveAlternateScreen();

static void RestoreConsoleTerminal()
{
  if (g_consoleActive) {
    endwin();
    g_consoleActive = false;
  }
  if (g_haveOriginalTermios) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
    g_haveOriginalTermios = false;
    write(STDOUT_FILENO, "\n", 1);
  }
}

static void ConsoleSignal(int sig)
{
  // Curses is not safe to shut down from a signal. Restoring the saved
  // terminal attributes is enough for the shell to be usable again.
  if (g_haveOriginalTermios)
    tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
  signal(sig, SIG_DFL);
  raise(sig);
}

static void InstallConsoleRestore()
{
  static bool installed = false;
  if (installed)
    return;
  installed = true;
  atexit(RestoreConsoleTerminal);
  // ^C is a console character (CP/M warm boot), not a host interrupt.
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, ConsoleSignal);
  signal(SIGHUP, ConsoleSignal);
}
#endif

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
  m_keyboard->SetKeyCharCallback(handler);
}

void Terminal::WriteString(const std::string & str)
{
  for (auto & r : str)
    WriteChar(r, false);
  Update(true);  
}

void Terminal::Clear()
{
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
  m_screen.reset(new ConScreen(*this, options, cols, rows));
  m_keyboard.reset(new ConKeyboard());

  m_useCurses = true;
}

ConsoleTerminal::~ConsoleTerminal()
{
#if __linux__ || __APPLE__
  RestoreConsoleTerminal();
#endif
}

bool ConsoleTerminal::Open()
{
#if __linux__ || __APPLE__
  if (!g_haveOriginalTermios && (tcgetattr(STDIN_FILENO, &g_originalTermios) == 0))
    g_haveOriginalTermios = true;
  InstallConsoleRestore();

  SCREEN * s = newterm(NULL, stdout, stdin);
  if (s == nullptr) {
    m_useCurses = false;
    return true;
  }
  set_term(s);
  DisableAlternateScreen();

  //initscr();
  scrollok(stdscr,TRUE);

  raw();
  keypad(stdscr, TRUE);
  noecho();
  nodelay(stdscr, TRUE);
  // Curses wants to clear on its first refresh, and it enters the
  // alternate screen there. Cancel the clear, leave that screen, and
  // continue from the bottom line of the terminal as it already is.
  clearok(stdscr, FALSE);
  if (curscr != nullptr)
    clearok(curscr, FALSE);
  refresh();
  LeaveAlternateScreen();
  move(LINES - 1, 0);
  refresh();
  g_consoleActive = true;
#endif
  return true;
}

void ConsoleTerminal::Clear()
{}

void ConsoleTerminal::WriteChar(uint8_t ch)
{
#if __linux__ || __APPLE__
  // Output has to go through curses. A direct write is wiped the next
  // time the keyboard poll refreshes the screen.
  if (m_useCurses && (stdscr != nullptr)) {
    int y, x;
    switch (ch) {
      case 0x0d:
        getyx(stdscr, y, x);
        move(y, 0);
        break;
      case 0x0a:
        // addch('\n') returns to column 0 and then erases to the end of
        // the screen, which wipes the line just written.
        getyx(stdscr, y, x);
        if (y >= LINES - 1) {
          scroll(stdscr);
          move(LINES - 1, 0);
        }
        else
          move(y + 1, 0);
        break;
      case 0x08:
        getyx(stdscr, y, x);
        if (x > 0)
          move(y, x - 1);
        break;
      default:
        if ((ch >= 0x20) && (ch < 0x7f))
          addch(ch);
        break;
    }
    refresh();
    fflush(stdout);
    return;
  }
#endif
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
  char keyCode;

#if __linux__ || __APPLE__
  if (m_useCurses) {
    int ch = getch();
    if (ch < 0)
      return;
    // The backspace key arrives as KEY_BACKSPACE or the terminal erase
    // character. CP/M line input accepts BS (0x08).
    if (ch == KEY_BACKSPACE || ch == (unsigned char)erasechar() || ch == 0x7f)
      ch = 0x08;
    keyCode = (char)ch;
  }
  else {
    int fd = STDIN_FILENO;

    //for (;;) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      timeval t;
      t.tv_sec  = 0;
      t.tv_usec = 1;
      int result = select(fd+1, &fds, NULL, NULL, &t);
      if (result < 1)
        return;

      if (read(fd, &keyCode, 1) < 1)
        return ;
      if ((unsigned char)keyCode == 0x7f)
        keyCode = 0x08;
    //}
  }
#endif

#if _WIN32
  for (;;) {
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
  }
#endif

  switch (keyCode) {
    case 0x0a:
      keyCode = 0x0d;
      break;

    default:
      break;
  }

  m_keyboard->OnKeyText(std::string((char *)&keyCode, 1));
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

//void ConKeyboard::OnKeyText(const std::string & str)
//{
//  cerr << "con OnKeyText " << str << endl;
//}

////////////////////////////////////////////////////////////////////////////////////

SDLTerminal::SDLTerminal(MainWindow & mainWindow, const Options & options, int cols, int rows, int hscale, int vscale)
  : Terminal(options, cols, rows)
{
  m_screen.reset  (new SDLVirtualScreen(mainWindow, options, cols, rows));
  m_screen->SetScale(hscale, vscale);
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
