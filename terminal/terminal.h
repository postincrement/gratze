#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "video/virtual_screen.h"
#include "devices/keypar.h"

struct Terminal
{
  public:
    class Screen : public VirtualScreen
    {
      public:
        Screen(Terminal & terminal, MainWindow & mainWindow, const Options & options, int cols, int rows);

        virtual bool Open() override;

        virtual FontChar GetCharAtPos(int x, int y) override;
        virtual void GetColourAtPos(int x, int y, SDL_Colour & fg, SDL_Colour & bg) override;

        struct CharCell
        {
          FontChar  m_ch;
          SDL_Color m_fg;
          SDL_Color m_bg;
        };

        Terminal & m_terminal;
        std::vector<CharCell> m_chars;
    };

    Terminal(MainWindow & mainWindow, const Options & options, int cols, int rows);

    bool Open();

    void Clear();

    void Update(bool hasChanged = false);

    void WriteChar(uint8_t ch);
    void WriteString(const std::string & str);

    int m_rows;
    int m_cols;
    int m_cursorX, m_cursorY;
    bool m_cursorEnabled = true;

    std::shared_ptr<Screen> m_screen;
    ParallelKeyboard m_keyboard;
};

#endif // TERMINAL_H_
