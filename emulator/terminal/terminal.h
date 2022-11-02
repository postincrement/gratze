#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "video/virtual_screen.h"
#include "devices/keypar.h"

class Emulator;

//
// Base class for a terminal abstraction
// Keyboard and display device like a serial terminal
//

struct Terminal
{
  public:
    Terminal(const Options & options, int cols, int rows);

    virtual bool Open();
    virtual void SetKeyboardHandler(std::function<void (uint8_t)> handler);

    virtual void Clear();
    virtual void WriteString(const std::string & str);
    virtual void WriteChar(uint8_t ch);

    virtual void Scroll(int lines);
    virtual void ClearToEndOfLine(int col, int line);

    virtual void Update(bool hasChanged = false);

    virtual void AddPollers(Emulator & emulator);

    std::shared_ptr<VirtualScreen> m_screen;
    std::shared_ptr<VirtualKeyboard> m_keyboard;

  protected:  

    virtual void WriteChar(uint8_t ch, bool update);
    int m_cols = 0;
    int m_rows = 0;
    int m_cursorX = 0;
    int m_cursorY = 0;
    std::function<void (uint8_t)> m_kbHandler = nullptr;
};

/////////////////////////////////////////////////////////////////////////////////

class ConScreen : public VirtualScreen
{
  public:
    ConScreen(Terminal & terminal, const Options & options, int cols, int rows);

    virtual void OnUpdate() override;
    virtual bool ResizeScreen() override;
    virtual bool SetFont(Font * font, int cols = -1, int rows = -1) override;
    virtual void RenderCharAtPos(int x, int y, bool withCursor, bool update = true) override;
};

class ConKeyboard : public VirtualKeyboard
{
  public:
    ConKeyboard();
    virtual void Reset() override;
    virtual void OnKeyDown(const SDL_Keysym & keysym) override;
    virtual void OnKeyUp(const SDL_Keysym & keysym) override;
    //virtual void OnKeyText(const std::string & str) override;
};

/////////////////////////////////////////////////////////////////////////////////

//
//  A terminal that uses the host native terminal implementation
//

struct ConsoleTerminal : public Terminal
{
  public:
    ConsoleTerminal(const Options & options, int cols, int rows);

    virtual bool Open() override;

    virtual void Clear() override;
    virtual void WriteChar(uint8_t ch) override;
    virtual void WriteString(const std::string & str) override;
    virtual void AddPollers(Emulator & emulator) override;
    void CheckConsoleKeyboard();

  protected:
    bool m_useCurses;  
};

/////////////////////////////////////////////////////////////////////////////////

//
//  A terminal that uses an SDL window
//

struct SDLTerminal : public Terminal
{
  public:
    SDLTerminal(MainWindow & mainWindow, const Options & options, int cols, int rows, int hscale, int vscale);
    virtual bool Open() override;
    virtual void Update(bool hasChanged = false) override;
};

#endif // TERMINAL_H_
