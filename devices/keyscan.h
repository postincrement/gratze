
#ifndef KEYSCAN_H_
#define KEYSCAN_H_

#include <map>
#include <vector>

#include "devices/keyboard.h"

class KeyboardScanner : public VirtualKeyboard
{
  public:
    struct ScanCode {
      const char * m_name = 0;
      uint16_t m_mod = 0;
    };
    struct ScanLayout 
    {
      int m_cols;
      int m_rows;

      const ScanCode * m_keyCodes;
      const ScanCode * m_shiftedKeyCodes;
      const ScanCode * m_ctrlKeyCodes;
    };

    struct KeyRowColInfo
    {
      KeyRowColInfo() = default;
      KeyRowColInfo(int row, int col, bool shiftSource, bool shiftOut)
        : m_row(row)
        , m_col(col)
        , m_shiftSource(shiftSource)
        , m_shiftOut(shiftOut)
        { }  
      int m_row = -1;
      int m_col = -1;
      bool m_shiftSource = false;
      bool m_shiftOut = false;
    };

    typedef std::multimap<SDL_Keycode, KeyRowColInfo> KeyRowColMap;

    bool Open();

    virtual void Reset() override;

    uint8_t Read(uint16_t rowMask);

    void Compile(const ScanLayout & scanLayout);
    void Compile(int row, int cols, const ScanCode * keyCodes, bool shifted);

    void KeyAction(const SDL_Keysym & keysym, bool down);
    void ActivateKey(const KeyRowColInfo & rowCol, bool down);

    virtual void OnKeyDown(const SDL_Keysym & keysym) override;
    virtual void OnKeyUp(const SDL_Keysym & keysym) override;

  public:
    std::vector<uint8_t> m_kbData;

    int m_rows = -1;
    int m_cols = -1;
  
    KeyRowColInfo m_shiftKey;
    KeyRowColInfo m_controlKey;
    KeyRowColInfo m_capsLockKey;

    bool m_defaultUpper = true;
    uint8_t m_shiftStatus;

    KeyRowColMap m_keys;
};

#endif // KEYSCAN_H_
