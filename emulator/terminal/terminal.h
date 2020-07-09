#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "video/virtual_screen.h"
#include "devices/keypar.h"

struct Terminal
{
  public:
    Terminal(const Options & options, int cols, int rows);

    virtual bool Open();
    virtual void SetKeyboardHandler(std::function<void (uint8_t)> handler);

    virtual void Clear() = 0;
    virtual void Update(bool hasChanged = false) = 0;
    virtual void WriteChar(uint8_t ch) = 0;
    virtual void WriteString(const std::string & str) = 0;

    std::shared_ptr<VirtualScreen> m_screen;
    std::shared_ptr<VirtualKeyboard> m_keyboard;

  protected:  
    int m_rows;
    int m_cols;
    int m_cursorX, m_cursorY;
    bool m_cursorEnabled = true;

    std::function<void (uint8_t)> m_kbHandler = nullptr;
};

/////////////////////////////////////////////////////////////////////////////////

struct ConsoleTerminal : public Terminal
{
  public:
    ConsoleTerminal(const Options & options, int cols, int rows);

    class ConScreen : public VirtualScreen
    {
      public:
        ConScreen(Terminal & terminal, const Options & options, int cols, int rows);
        virtual void Update(bool hasChanged = false) override;
        virtual void SetScale(int hscale, int vscale) override;
        virtual bool SetFont(Font * font, int cols = -1, int rows = -1) override;
        virtual int MapPosToLoc(int x, int y) override;
        virtual void SetCursorPos(int x, int y) override;
        virtual void EnableCursor(bool enable = true) override;
    };

    class ConKeyboard : public VirtualKeyboard
    {
      public:
        ConKeyboard();
        virtual void Reset() override;
        virtual void OnKeyDown(const SDL_Keysym & keysym) override;
        virtual void OnKeyUp(const SDL_Keysym & keysym) override;
    };

    virtual bool Open() override;

    virtual void Clear() override;
    virtual void Update(bool hasChanged = false) override;
    virtual void WriteChar(uint8_t ch) override;
    virtual void WriteString(const std::string & str) override;
};

/////////////////////////////////////////////////////////////////////////////////

struct SDLTerminal : public Terminal
{
  public:
    SDLTerminal(MainWindow & mainWindow, const Options & options, int cols, int rows);

    virtual bool Open() override;
    virtual void SetKeyboardHandler(std::function<void (uint8_t)> handler);

    virtual void Clear() override;
    virtual void Update(bool hasChanged = false) override;
    virtual void WriteChar(uint8_t ch) override;
    virtual void WriteString(const std::string & str) override;

    class SDLScreen : public SDLVirtualScreen
    {
      public:
        SDLScreen(SDLTerminal & terminal, MainWindow & mainWindow, const Options & options, int cols, int rows);

        virtual bool Open() override;

        virtual FontChar GetCharAtPos(int x, int y) override;
        virtual void GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg) override;

        void Scroll(int lines);
        void ClearToEndOfLine(int col, int line);
        void SetCharAtLoc(int loc, uint8_t ch);

        struct CharCell
        {
          CharCell()
            : m_ch(' ')
          { }
          CharCell(uint8_t ch)
            : m_ch(ch)
          { }
          FontChar  m_ch;
          SDL_Color m_fg;
          SDL_Color m_bg;
        };

        SDLTerminal & m_terminal;
        std::vector<CharCell> m_chars;
        SDL_Color m_fg;
        SDL_Color m_bg;
    };

  protected:
    std::shared_ptr<SDLScreen> m_sdlScreen;
};

#endif // TERMINAL_H_
