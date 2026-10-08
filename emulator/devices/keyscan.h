#ifndef KEYSCAN_H_
#define KEYSCAN_H_

#include <map>
#include <string>
#include <vector>

#include "devices/keyboard.h"

// Shared matrix keyboard for machines whose ROM scans a row of key bits
// (TRS-80 memory map, Microbee 6545 addresses, Super-80 PIO, and others).
//
// Text mode maps the host glyph, including Shift, onto the matrix key that
// produces that glyph. Game mode maps each matrix key to the host key in the
// same physical position so programs that depend on key location still work.
// Read() returns the same bit layout the original scan expects.
class ScannedKeyboard : public VirtualKeyboard
{
  public:
    bool Open();

    void SetGameMode(bool mode);
    void SetDebug(bool debug);

    virtual bool IsScanned() const override;

    virtual void Reset() override;

    uint8_t Read(uint16_t rowMask) override;

    virtual void OnKeyDown(const SDL_Keysym & keysym) override;
    virtual void OnKeyUp(const SDL_Keysym & keysym) override;
    virtual void OnKeyText(const std::string & str) override;

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

      int m_cols = 0;
      int m_rows = 0;

      // Host key in the same physical position as each matrix cell.
      // Null uses the unshifted legend as that host key name.
      const ScanCode * m_gameKeys = nullptr;

      // Glyph produced by each cell with Shift up, then with Shift down.
      const ScanCode * m_textKeys = nullptr;
      const ScanCode * m_shiftedTextKeys = nullptr;

      // Extra host keys that activate a named target key (Backspace -> Left).
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
      bool m_shifted = false;
    };

    struct HeldKey
    {
      int m_row = -1;
      int m_col = -1;
      bool m_shiftOn = false;
      bool m_shiftOff = false;
      bool m_controlOn = false;
      bool m_awaitingGlyph = false;
      unsigned char m_glyph = 0;
    };

    void CompileModifiers(const ScanCode * keyCodes);
    void CompileGlyphs(const ScanCode * keyCodes, bool shifted);
    void CompilePositions(const ScanCode * keyCodes);
    void CompileEquivalents(const ScanLayout & scanLayout);

    void Note(const std::string & text);

    void AddGlyph(unsigned char glyph, const KeyRowColInfo & cell);
    void AddNamed(SDL_Keycode keycode, const KeyRowColInfo & cell);
    void AddPosition(SDL_Scancode scancode, const KeyRowColInfo & cell, const char * name);
    bool FindCell(const ScanCode * keyCodes, const char * name, KeyRowColInfo & cell) const;

    void PressPosition(SDL_Scancode scancode, const KeyRowColInfo & cell);
    void PressText(SDL_Scancode scancode, SDL_Keycode sym, bool hostShift, const KeyRowColInfo & cell, unsigned char glyph, bool awaitCorrection);
    void PressModifier(SDL_Scancode scancode, bool shift, bool control);
    void ReleaseScancode(SDL_Scancode scancode);

    void ApplyGlyph(const KeyRowColInfo & cell, bool down);
    void NoteShift(bool on, bool down);
    void NoteControl(bool on, bool down);
    void UpdateShift();
    void UpdateControl();

    bool LookupGlyph(unsigned char glyph, KeyRowColInfo & cell) const;
    bool FindBaseLetter(char lower, KeyRowColInfo & cell) const;

    KeyRowColInfo m_shiftRowCol;
    KeyRowColInfo m_controlRowCol;
    KeyRowColInfo m_returnCell;

    typedef std::map<SDL_Scancode, KeyRowColInfo> PositionMap;
    typedef std::map<unsigned char, KeyRowColInfo> GlyphMap;
    typedef std::map<SDL_Keycode, KeyRowColInfo> NamedMap;

    PositionMap m_positions;
    GlyphMap m_glyphs;
    NamedMap m_named;

    std::map<SDL_Scancode, HeldKey> m_held;
    SDL_Scancode m_pendingScancode = SDL_SCANCODE_UNKNOWN;
    SDL_Keycode m_pendingSym = SDLK_UNKNOWN;
    bool m_pendingShift = false;

    int m_shiftOn = 0;
    int m_shiftOff = 0;
    int m_controlOn = 0;
    bool m_shiftDown = false;
    bool m_controlDown = false;

    bool m_gameMode = false;
    bool m_debug = false;
    std::string m_notes;

    int m_rows = 0;
    int m_cols = 0;
    std::vector<uint8_t> m_kbData;
};

#endif // KEYSCAN_H_
