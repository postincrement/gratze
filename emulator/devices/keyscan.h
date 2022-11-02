
#ifndef KEYSCAN_H_
#define KEYSCAN_H_

#include <map>
#include <vector>

#include "devices/keyboard.h"

class ScannedKeyboard : public VirtualKeyboard
{
  public:
    bool Open();

    void SetGameMode(bool mode);

    virtual void Reset() override;

    uint8_t Read(uint16_t rowMask) override;

    virtual void OnKeyDown(const SDL_Keysym & keysym) override;
    virtual void OnKeyUp(const SDL_Keysym & keysym) override;
    virtual void OnKeyChar(char ch) override;

    //
    //  keyboard map
    //
    struct ScanCode {
      const char * m_name = 0;
    };

    struct ScanLayout
    {
      struct Equivalent
      {
        const char * m_from;
        const char * m_to;
      };

      int m_cols;
      int m_rows;

      const ScanCode * m_gameKeys;
      const ScanCode * m_textKeys;
      const ScanCode * m_shiftedTextKeys;

      std::vector<Equivalent> m_equivalents;
    };

    void Compile(const ScanLayout & scanLayout);

  protected:  
    struct KeyRowColInfo
    {
      KeyRowColInfo() = default;
      KeyRowColInfo(int row, int col, bool shifted = false)
        : m_row(row)
        , m_col(col)
        , m_shifted(shifted)
        { }
      int m_row = -1;
      int m_col = -1;
      bool m_shifted = false;   // destination keycode needs to be shifted 
    };

    KeyRowColInfo m_shiftRowCol;
    KeyRowColInfo m_controlRowCol;
    KeyRowColInfo m_capsLockRowCol;

  //
  // game mode
  //
  protected:  
    typedef std::map<SDL_Keycode, KeyRowColInfo> SDLKeyRowColMap;

    void Compile(SDLKeyRowColMap & keyMap, int row, int cols, const ScanCode * keyCodes);

    SDLKeyRowColMap m_gameKeys;

    bool m_leftShift;
    bool m_rightShift;

  //
  // text mode
  //
  protected:
    typedef std::map<char, KeyRowColInfo> TextKeyRowColMap;

    void Compile(TextKeyRowColMap & keyMap, int row, int cols, const ScanCode * keyCodes, bool isShifted);

    bool AddEquivalent(TextKeyRowColMap & keyMap, const std::string & fromName, const std::string & toName);

    TextKeyRowColMap m_textKeys;

  //
  //  key scanner
  //
  protected:  
    bool FindKey(const std::string & name, SDL_Keycode & keycode) const;

    void GameKeyAction(const SDL_Keysym & keysym, bool down);
    void TextKeyAction(const SDL_Keysym & keysym, bool down);

    void ActivateKey(const KeyRowColInfo & rowCol, bool down);

    bool m_gameMode = false;

    int m_rows = -1;
    int m_cols = -1;
    std::vector<uint8_t> m_kbData;
};

#endif // KEYSCAN_H_
