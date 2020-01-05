
#ifndef KEYSCAN_H_
#define KEYSCAN_H_

#include <map>
#include <vector>

#include <SDL.h>

class KeyboardScanner
{
  public:
    struct ScanLayout 
    {
      int m_cols;
      int m_rows;

      const char ** m_keyCodes;
      const char ** m_shiftedKeyCodes;
      const char ** m_ctrlKeyCodes;
    };

    struct KeyRowColInfo
    {
      KeyRowColInfo() = default;
      KeyRowColInfo(int row, int col)
        : m_row(row)
        , m_col(col)
        { }  
      int m_row = -1;
      int m_col = -1;
      int m_shifted = 0;
    };

    typedef std::map<uint16_t, KeyRowColInfo> KeyRowColMap;

    bool Open();

    uint8_t Read(uint16_t rowMask);

    void Compile(const ScanLayout & scanLayout);
    void Compile(int row, int cols, const char ** keyCodes, KeyRowColMap & keyRowCols);

    void OnKeyDown(const SDL_Keysym & keysym);
    void OnKeyUp(const SDL_Keysym & keysym);

  public:
    KeyRowColInfo m_shiftKey;
    std::vector<uint8_t> m_kbData;

    int m_rows = -1;
    int m_cols = -1;
  
    uint8_t m_shiftDown;

    // 0 = keys
    // 1 - shifted keys
    // 2 = ctrl keys
    std::array<KeyRowColMap, 3> m_keys;
};

#endif // KEYSCAN_H_
