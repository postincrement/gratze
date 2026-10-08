#include <iostream>
#include <map>

#include <SDL.h>

#include <strings.h>
#include <string.h>

#include "common/misc.h"
#include "devices/keyscan.h"

using namespace std;

namespace {

enum class Modifier
{
  eNone,
  eShift,
  eControl,
  eCapsLock
};

Modifier ModifierOf(const char * name)
{
  if (name == nullptr)
    return Modifier::eNone;
  if (strcasecmp(name, "shift") == 0)
    return Modifier::eShift;
  if ((strcasecmp(name, "control") == 0) || (strcasecmp(name, "ctrl") == 0))
    return Modifier::eControl;
  if ((strcasecmp(name, "capslock") == 0) || (strcasecmp(name, "caps lock") == 0))
    return Modifier::eCapsLock;
  return Modifier::eNone;
}

bool SameName(const char * a, const char * b)
{
  if ((a == nullptr) || (b == nullptr))
    return false;
  if ((a[0] != 0) && (a[1] == 0))
    return strcmp(a, b) == 0;
  return strcasecmp(a, b) == 0;
}

// Physical key for a legend. Scancode names follow the US key positions,
// which is what game mode uses for "same place on the keyboard".
SDL_Scancode ScancodeForLegend(const char * name)
{
  if ((name == nullptr) || (name[0] == 0))
    return SDL_SCANCODE_UNKNOWN;
  if ((strcmp(name, " ") == 0) || (strcasecmp(name, "space") == 0))
    return SDL_SCANCODE_SPACE;
  if ((strcasecmp(name, "break") == 0) || (strcasecmp(name, "pause") == 0))
    return SDL_SCANCODE_PAUSE;
  if (strcasecmp(name, "enter") == 0)
    return SDL_SCANCODE_RETURN;
  if (strcasecmp(name, "shift") == 0)
    return SDL_SCANCODE_LSHIFT;
  if ((strcasecmp(name, "control") == 0) || (strcasecmp(name, "ctrl") == 0))
    return SDL_SCANCODE_LCTRL;
  if (strcasecmp(name, "clear") == 0)
    return SDL_SCANCODE_NUMLOCKCLEAR;

  SDL_Scancode scancode = SDL_GetScancodeFromName(name);
  if (scancode != SDL_SCANCODE_UNKNOWN)
    return scancode;

  if (name[1] != 0)
    return SDL_SCANCODE_UNKNOWN;

  if ((name[0] >= 'A') && (name[0] <= 'Z'))
    return (SDL_Scancode)(SDL_SCANCODE_A + (name[0] - 'A'));
  if ((name[0] >= 'a') && (name[0] <= 'z'))
    return (SDL_Scancode)(SDL_SCANCODE_A + (name[0] - 'a'));
  if ((name[0] >= '1') && (name[0] <= '9'))
    return (SDL_Scancode)(SDL_SCANCODE_1 + (name[0] - '1'));
  if (name[0] == '0')
    return SDL_SCANCODE_0;

  switch (name[0]) {
    case '-': return SDL_SCANCODE_MINUS;
    case '=': return SDL_SCANCODE_EQUALS;
    case '[': return SDL_SCANCODE_LEFTBRACKET;
    case ']': return SDL_SCANCODE_RIGHTBRACKET;
    case '\\': return SDL_SCANCODE_BACKSLASH;
    case ';': return SDL_SCANCODE_SEMICOLON;
    case '\'': return SDL_SCANCODE_APOSTROPHE;
    case '`': return SDL_SCANCODE_GRAVE;
    case ',': return SDL_SCANCODE_COMMA;
    case '.': return SDL_SCANCODE_PERIOD;
    case '/': return SDL_SCANCODE_SLASH;
    default: return SDL_SCANCODE_UNKNOWN;
  }
}

// ASCII produced by a named target key, or -1 when the name is only a position.
int GlyphForName(const char * name)
{
  if ((name == nullptr) || (name[0] == 0))
    return -1;
  if (name[1] == 0)
    return (unsigned char)name[0];
  if ((strcasecmp(name, "return") == 0) || (strcasecmp(name, "enter") == 0))
    return '\r';
  if (strcasecmp(name, "escape") == 0)
    return 0x1b;
  if (strcasecmp(name, "backspace") == 0)
    return '\b';
  if (strcasecmp(name, "tab") == 0)
    return '\t';
  if (strcasecmp(name, "space") == 0)
    return ' ';
  if ((strcasecmp(name, "lf") == 0) || (strcasecmp(name, "linefeed") == 0))
    return '\n';
  if (strcasecmp(name, "delete") == 0)
    return 0x7f;
  return -1;
}

char ShiftedPunctuation(char unshifted)
{
  switch (unshifted) {
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';
    case '-': return '_';
    case '=': return '+';
    case '[': return '{';
    case ']': return '}';
    case '\\': return '|';
    case ';': return ':';
    case '\'': return '"';
    case ',': return '<';
    case '.': return '>';
    case '/': return '?';
    case '`': return '~';
    default: return unshifted;
  }
}

int DecodeUtf8(const std::string & text, size_t & index)
{
  if (index >= text.size())
    return -1;
  unsigned char c0 = (unsigned char)text[index++];
  if (c0 < 0x80)
    return c0;
  if (((c0 & 0xe0) == 0xc0) && (index < text.size())) {
    unsigned char c1 = (unsigned char)text[index++];
    return ((c0 & 0x1f) << 6) | (c1 & 0x3f);
  }
  if (((c0 & 0xf0) == 0xe0) && (index + 1 < text.size())) {
    unsigned char c1 = (unsigned char)text[index++];
    unsigned char c2 = (unsigned char)text[index++];
    return ((c0 & 0x0f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f);
  }
  return -1;
}

} // namespace

bool ScannedKeyboard::Open()
{
  Reset();
  return true;
}

bool ScannedKeyboard::IsScanned() const
{
  return true;
}

void ScannedKeyboard::Reset()
{
  VECTOR_ZERO(m_kbData);
  m_held.clear();
  m_pendingScancode = SDL_SCANCODE_UNKNOWN;
  m_pendingSym = SDLK_UNKNOWN;
  m_pendingShift = false;
  m_shiftOn = 0;
  m_shiftOff = 0;
  m_controlOn = 0;
  m_shiftDown = false;
  m_controlDown = false;
}

void ScannedKeyboard::SetGameMode(bool mode)
{
  m_gameMode = mode;
}

void ScannedKeyboard::SetDebug(bool debug)
{
  m_debug = debug;
}

uint8_t ScannedKeyboard::Read(uint16_t rowMask)
{
  uint16_t mask = 1;
  uint8_t value = 0x00;
  for (int i = 0; i < m_rows; ++i) {
    if (rowMask & mask)
      value |= m_kbData[i];
    mask = mask << 1;
  }
  return value;
}

void ScannedKeyboard::Compile(const ScanLayout & scanLayout)
{
  m_rows = scanLayout.m_rows;
  m_cols = scanLayout.m_cols;
  if (m_rows < 0)
    m_rows = 0;
  if (m_cols < 0)
    m_cols = 0;

  m_kbData.assign(m_rows, 0);
  m_positions.clear();
  m_glyphs.clear();
  m_named.clear();
  m_shiftRowCol = KeyRowColInfo();
  m_controlRowCol = KeyRowColInfo();
  m_returnCell = KeyRowColInfo();
  m_notes.clear();
  Reset();

  if (scanLayout.m_textKeys != nullptr)
    CompileModifiers(scanLayout.m_textKeys);
  if (scanLayout.m_gameKeys != nullptr)
    CompileModifiers(scanLayout.m_gameKeys);

  if (scanLayout.m_textKeys != nullptr)
    CompileGlyphs(scanLayout.m_textKeys, false);
  if (scanLayout.m_shiftedTextKeys != nullptr)
    CompileGlyphs(scanLayout.m_shiftedTextKeys, true);

  const ScanCode * positions = (scanLayout.m_gameKeys != nullptr) ? scanLayout.m_gameKeys : scanLayout.m_textKeys;
  if (positions != nullptr)
    CompilePositions(positions);

  if ((m_returnCell.m_row >= 0) && (m_positions.count(SDL_SCANCODE_KP_ENTER) == 0))
    AddPosition(SDL_SCANCODE_KP_ENTER, m_returnCell, "Keypad Enter");
  if ((m_returnCell.m_row >= 0) && (m_named.count(SDLK_KP_ENTER) == 0))
    AddNamed(SDLK_KP_ENTER, m_returnCell);

  CompileEquivalents(scanLayout);

  if (!m_notes.empty())
    cerr << "kb: " << m_notes << endl;

  if (m_debug) {
    cerr << "kb: " << (m_gameMode ? "game" : "text") << " mode, "
         << m_glyphs.size() << " glyphs, "
         << m_named.size() << " special keys, "
         << m_positions.size() << " positions" << endl;
  }
}

void ScannedKeyboard::Note(const std::string & text)
{
  if (!m_gameMode && !m_debug)
    return;
  if (!m_notes.empty())
    m_notes += "; ";
  m_notes += text;
}

void ScannedKeyboard::CompileModifiers(const ScanCode * keyCodes)
{
  const ScanCode * cursor = keyCodes;
  for (int row = 0; row < m_rows; ++row) {
    for (int col = 0; col < m_cols; ++col) {
      const ScanCode * code = cursor++;
      Modifier modifier = ModifierOf(code->m_name);
      if ((modifier == Modifier::eShift) && (m_shiftRowCol.m_row < 0))
        m_shiftRowCol = KeyRowColInfo(row, col, false);
      else if ((modifier == Modifier::eControl) && (m_controlRowCol.m_row < 0))
        m_controlRowCol = KeyRowColInfo(row, col, false);
    }
  }
}

void ScannedKeyboard::AddGlyph(unsigned char glyph, const KeyRowColInfo & cell)
{
  if (glyph == 0)
    return;
  auto existing = m_glyphs.find(glyph);
  if (existing == m_glyphs.end()) {
    m_glyphs.emplace(glyph, cell);
    return;
  }
  // A shifted legend loses to a key that already produces this glyph.
  if (cell.m_shifted)
    return;
  if ((existing->second.m_row != cell.m_row) || (existing->second.m_col != cell.m_col))
    Note(std::string("glyph '") + (char)glyph + "' is produced by more than one key");
}

void ScannedKeyboard::AddNamed(SDL_Keycode keycode, const KeyRowColInfo & cell)
{
  if (keycode == SDLK_UNKNOWN)
    return;
  auto existing = m_named.find(keycode);
  if (existing == m_named.end()) {
    m_named.emplace(keycode, cell);
    return;
  }
  if ((existing->second.m_row != cell.m_row) || (existing->second.m_col != cell.m_col))
    Note(std::string("host key '") + SDL_GetKeyName(keycode) + "' is already mapped");
}

void ScannedKeyboard::AddPosition(SDL_Scancode scancode, const KeyRowColInfo & cell, const char * name)
{
  if (scancode == SDL_SCANCODE_UNKNOWN) {
    if ((name != nullptr) && (name[0] != 0))
      Note(std::string("no host position for '") + name + "'");
    return;
  }
  auto existing = m_positions.find(scancode);
  if (existing == m_positions.end()) {
    m_positions.emplace(scancode, cell);
    return;
  }
  if ((existing->second.m_row != cell.m_row) || (existing->second.m_col != cell.m_col)) {
    const char * label = ((name != nullptr) && (name[0] != 0)) ? name : SDL_GetScancodeName(scancode);
    Note(std::string("host position '") + label + "' is already mapped");
  }
}

bool ScannedKeyboard::FindCell(const ScanCode * keyCodes, const char * name, KeyRowColInfo & cell) const
{
  if (keyCodes == nullptr)
    return false;
  const ScanCode * cursor = keyCodes;
  for (int row = 0; row < m_rows; ++row) {
    for (int col = 0; col < m_cols; ++col) {
      const ScanCode * code = cursor++;
      if (SameName(name, code->m_name)) {
        cell = KeyRowColInfo(row, col, false);
        return true;
      }
    }
  }
  return false;
}

void ScannedKeyboard::CompileGlyphs(const ScanCode * keyCodes, bool shifted)
{
  const ScanCode * cursor = keyCodes;
  for (int row = 0; row < m_rows; ++row) {
    for (int col = 0; col < m_cols; ++col) {
      const ScanCode * code = cursor++;
      const char * name = code->m_name;
      if ((name == nullptr) || (name[0] == 0))
        continue;
      if (ModifierOf(name) != Modifier::eNone)
        continue;

      KeyRowColInfo cell(row, col, shifted);

      if ((strcasecmp(name, "return") == 0) || (strcasecmp(name, "enter") == 0))
        m_returnCell = KeyRowColInfo(row, col, false);

      // Shifted legends only add glyphs. The key itself was named in the unshifted map.
      if (shifted) {
        if (name[1] == 0)
          AddGlyph((unsigned char)name[0], cell);
        continue;
      }

      int glyph = GlyphForName(name);
      if (glyph > 0)
        AddGlyph((unsigned char)glyph, cell);

      SDL_Keycode keycode = SDLK_UNKNOWN;
      if (strcasecmp(name, "clear") == 0) {
        AddNamed(SDLK_CLEAR, cell);
        AddNamed(SDLK_NUMLOCKCLEAR, cell);
      }
      else if ((strcasecmp(name, "break") == 0) || (strcasecmp(name, "pause") == 0)) {
        AddNamed(SDLK_PAUSE, cell);
      }
      else if (name[1] != 0) {
        keycode = SDL_GetKeyFromName(name);
        if ((keycode != SDLK_UNKNOWN) && ((keycode < 0) || (keycode >= 128) || (keycode != glyph)))
          AddNamed(keycode, cell);
        else if ((keycode == SDLK_UNKNOWN) && (glyph < 0) && (strcasecmp(name, "lf") != 0) && (strcasecmp(name, "linefeed") != 0))
          Note(std::string("no host key for '") + name + "'");
      }
    }
  }
}

void ScannedKeyboard::CompilePositions(const ScanCode * keyCodes)
{
  const ScanCode * cursor = keyCodes;
  for (int row = 0; row < m_rows; ++row) {
    for (int col = 0; col < m_cols; ++col) {
      const ScanCode * code = cursor++;
      const char * name = code->m_name;
      if ((name == nullptr) || (name[0] == 0))
        continue;

      KeyRowColInfo cell(row, col, false);
      Modifier modifier = ModifierOf(name);
      if (modifier == Modifier::eShift) {
        AddPosition(SDL_SCANCODE_LSHIFT, cell, name);
        AddPosition(SDL_SCANCODE_RSHIFT, cell, name);
        continue;
      }
      if (modifier == Modifier::eControl) {
        AddPosition(SDL_SCANCODE_LCTRL, cell, name);
        AddPosition(SDL_SCANCODE_RCTRL, cell, name);
        continue;
      }
      if (strcasecmp(name, "clear") == 0) {
        AddPosition(SDL_SCANCODE_NUMLOCKCLEAR, cell, name);
        AddPosition(SDL_SCANCODE_CLEAR, cell, name);
        continue;
      }
      if ((strcasecmp(name, "break") == 0) || (strcasecmp(name, "pause") == 0)) {
        AddPosition(SDL_SCANCODE_PAUSE, cell, name);
        continue;
      }

      AddPosition(ScancodeForLegend(name), cell, name);
    }
  }
}

void ScannedKeyboard::CompileEquivalents(const ScanLayout & scanLayout)
{
  if (scanLayout.m_textKeys == nullptr)
    return;

  for (const auto & equivalent : scanLayout.m_equivalents) {
    KeyRowColInfo cell;
    if (!FindCell(scanLayout.m_textKeys, equivalent.m_to, cell)) {
      Note(std::string("equivalent target '") + equivalent.m_to + "' is not on this keyboard");
      continue;
    }

    SDL_Scancode scancode = ScancodeForLegend(equivalent.m_from);
    if ((scancode != SDL_SCANCODE_UNKNOWN) && (m_positions.count(scancode) == 0))
      AddPosition(scancode, cell, equivalent.m_from);

    int glyph = GlyphForName(equivalent.m_from);
    if ((glyph > 0) && (m_glyphs.count((unsigned char)glyph) == 0))
      AddGlyph((unsigned char)glyph, cell);

    SDL_Keycode keycode = SDL_GetKeyFromName(equivalent.m_from);
    if ((strcmp(equivalent.m_from, " ") == 0) || (strcasecmp(equivalent.m_from, "space") == 0))
      keycode = SDLK_SPACE;
    if ((keycode != SDLK_UNKNOWN) && (m_named.count(keycode) == 0) && ((keycode < 0) || (keycode >= 128)))
      AddNamed(keycode, cell);
  }
}

///////////////////////////////////////////////////////////////////////////////////

void ScannedKeyboard::ApplyGlyph(const KeyRowColInfo & cell, bool down)
{
  if ((cell.m_row < 0) || (cell.m_row >= (int)m_kbData.size()) || (cell.m_col < 0) || (cell.m_col >= 8))
    return;

  uint8_t mask = (uint8_t)(1u << cell.m_col);
  if (down)
    m_kbData[cell.m_row] |= mask;
  else
    m_kbData[cell.m_row] &= (uint8_t)~mask;
}

void ScannedKeyboard::NoteShift(bool on, bool down)
{
  int & count = on ? m_shiftOn : m_shiftOff;
  if (down)
    ++count;
  else if (count > 0)
    --count;
}

void ScannedKeyboard::NoteControl(bool on, bool down)
{
  if (!on)
    return;
  if (down)
    ++m_controlOn;
  else if (m_controlOn > 0)
    --m_controlOn;
}

void ScannedKeyboard::UpdateShift()
{
  bool on = (m_shiftRowCol.m_row >= 0) && (m_shiftOn > 0) && (m_shiftOff == 0);
  if (on == m_shiftDown)
    return;
  m_shiftDown = on;
  ApplyGlyph(m_shiftRowCol, on);
}

void ScannedKeyboard::UpdateControl()
{
  bool on = (m_controlRowCol.m_row >= 0) && (m_controlOn > 0);
  if (on == m_controlDown)
    return;
  m_controlDown = on;
  ApplyGlyph(m_controlRowCol, on);
}

void ScannedKeyboard::ReleaseScancode(SDL_Scancode scancode)
{
  auto held = m_held.find(scancode);
  if (held == m_held.end())
    return;

  const HeldKey & key = held->second;
  if (key.m_row >= 0)
    ApplyGlyph(KeyRowColInfo(key.m_row, key.m_col), false);
  if (key.m_shiftOn)
    NoteShift(true, false);
  if (key.m_shiftOff)
    NoteShift(false, false);
  if (key.m_controlOn)
    NoteControl(true, false);
  m_held.erase(held);

  if (m_pendingScancode == scancode)
    m_pendingScancode = SDL_SCANCODE_UNKNOWN;

  UpdateShift();
  UpdateControl();
}

void ScannedKeyboard::PressPosition(SDL_Scancode scancode, const KeyRowColInfo & cell)
{
  ReleaseScancode(scancode);

  HeldKey held;
  held.m_row = cell.m_row;
  held.m_col = cell.m_col;
  ApplyGlyph(cell, true);
  m_held.emplace(scancode, held);

  if (m_debug) {
    cerr << "kb: position down row " << cell.m_row << " col " << cell.m_col
         << " (" << SDL_GetScancodeName(scancode) << ")" << endl;
  }
}

void ScannedKeyboard::PressText(SDL_Scancode scancode, SDL_Keycode sym, bool hostShift, const KeyRowColInfo & cell, unsigned char glyph, bool awaitCorrection)
{
  ReleaseScancode(scancode);

  HeldKey held;
  held.m_row = cell.m_row;
  held.m_col = cell.m_col;
  held.m_glyph = glyph;
  held.m_awaitingGlyph = awaitCorrection;
  held.m_shiftOn = cell.m_shifted;
  bool differs = (sym >= 0) && (sym < 128) && (glyph != (unsigned char)sym);
  held.m_shiftOff = hostShift && !cell.m_shifted && differs;

  if (held.m_row >= 0)
    ApplyGlyph(cell, true);
  if (held.m_shiftOn)
    NoteShift(true, true);
  if (held.m_shiftOff)
    NoteShift(false, true);
  if (held.m_controlOn)
    NoteControl(true, true);
  UpdateShift();
  UpdateControl();

  m_held.emplace(scancode, held);
  if (awaitCorrection) {
    m_pendingScancode = scancode;
    m_pendingSym = sym;
    m_pendingShift = hostShift;
  }

  if (m_debug) {
    cerr << "kb: glyph '";
    if ((glyph >= 32) && (glyph < 127))
      cerr << (char)glyph;
    else
      cerr << HEXFORMAT0x2(glyph);
    cerr << "' down row " << cell.m_row << " col " << cell.m_col
         << (held.m_shiftOn ? " shifted" : "")
         << (held.m_shiftOff ? " unshifted" : "") << endl;
  }
}

void ScannedKeyboard::PressModifier(SDL_Scancode scancode, bool shift, bool control)
{
  ReleaseScancode(scancode);

  HeldKey held;
  held.m_shiftOn = shift && (m_shiftRowCol.m_row >= 0);
  held.m_controlOn = control && (m_controlRowCol.m_row >= 0);
  if (held.m_shiftOn)
    NoteShift(true, true);
  if (held.m_controlOn)
    NoteControl(true, true);
  UpdateShift();
  UpdateControl();
  m_held.emplace(scancode, held);
}

bool ScannedKeyboard::LookupGlyph(unsigned char glyph, KeyRowColInfo & cell) const
{
  auto found = m_glyphs.find(glyph);
  if (found == m_glyphs.end())
    return false;
  cell = found->second;
  return true;
}

bool ScannedKeyboard::FindBaseLetter(char lower, KeyRowColInfo & cell) const
{
  char upper = (char)(lower - 'a' + 'A');
  auto lowerKey = m_glyphs.find((unsigned char)lower);
  auto upperKey = m_glyphs.find((unsigned char)upper);

  const KeyRowColInfo * pick = nullptr;
  if ((lowerKey != m_glyphs.end()) && !lowerKey->second.m_shifted)
    pick = &lowerKey->second;
  else if ((upperKey != m_glyphs.end()) && !upperKey->second.m_shifted)
    pick = &upperKey->second;
  else if (lowerKey != m_glyphs.end())
    pick = &lowerKey->second;
  else if (upperKey != m_glyphs.end())
    pick = &upperKey->second;
  else
    return false;

  cell = *pick;
  cell.m_shifted = false;
  return true;
}

void ScannedKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{
  SDL_Scancode scancode = keysym.scancode;
  SDL_Keycode sym = keysym.sym;
  bool hostShift = (keysym.mod & KMOD_SHIFT) != 0;
  bool hostCaps = (keysym.mod & KMOD_CAPS) != 0;
  bool hostCtrl = (keysym.mod & KMOD_CTRL) != 0;

  if (m_gameMode) {
    auto found = m_positions.find(scancode);
    if (found == m_positions.end()) {
      if (m_debug)
        cerr << "kb: unmapped position " << SDL_GetScancodeName(scancode) << endl;
      return;
    }
    PressPosition(scancode, found->second);
    return;
  }

  if ((sym == SDLK_LSHIFT) || (sym == SDLK_RSHIFT)) {
    PressModifier(scancode, true, false);
    return;
  }
  if (sym == SDLK_CAPSLOCK)
    return;
  if ((sym == SDLK_LCTRL) || (sym == SDLK_RCTRL)) {
    PressModifier(scancode, false, true);
    return;
  }

  auto named = m_named.find(sym);
  if (named != m_named.end()) {
    PressText(scancode, sym, hostShift, named->second, 0, false);
    return;
  }

  if (hostCtrl && (sym >= 'a') && (sym <= 'z')) {
    KeyRowColInfo cell;
    if (FindBaseLetter((char)sym, cell))
      PressText(scancode, sym, false, cell, (unsigned char)sym, false);
    return;
  }

  int glyph = -1;
  if ((sym >= 'a') && (sym <= 'z')) {
    bool upper = hostShift ^ hostCaps;
    glyph = upper ? (sym - 'a' + 'A') : (int)sym;
  }
  else if ((sym >= 32) && (sym < 127)) {
    glyph = hostShift ? (unsigned char)ShiftedPunctuation((char)sym) : (int)sym;
  }
  else if ((sym >= 0) && (sym < 32)) {
    glyph = (int)sym;
  }
  else if (sym == SDLK_DELETE) {
    glyph = 0x7f;
  }

  if (glyph > 0) {
    KeyRowColInfo cell;
    if (LookupGlyph((unsigned char)glyph, cell)) {
      bool printable = (glyph >= 32) && (glyph < 127);
      PressText(scancode, sym, hostShift, cell, (unsigned char)glyph, printable);
      return;
    }
  }

  if ((sym >= 32) && (sym < 127)) {
    // Layout-specific text input may still identify this key.
    ReleaseScancode(scancode);
    HeldKey held;
    held.m_awaitingGlyph = true;
    m_held.emplace(scancode, held);
    m_pendingScancode = scancode;
    m_pendingSym = sym;
    m_pendingShift = hostShift;
    return;
  }

  if (m_debug)
    cerr << "kb: unmapped text key " << SDL_GetKeyName(sym) << endl;
}

void ScannedKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{
  ReleaseScancode(keysym.scancode);
}

void ScannedKeyboard::OnKeyText(const std::string & str)
{
  if (m_gameMode || (m_pendingScancode == SDL_SCANCODE_UNKNOWN))
    return;

  auto held = m_held.find(m_pendingScancode);
  if ((held == m_held.end()) || !held->second.m_awaitingGlyph)
    return;

  size_t index = 0;
  int codepoint = DecodeUtf8(str, index);
  if ((codepoint <= 0) || (codepoint > 255)) {
    held->second.m_awaitingGlyph = false;
    return;
  }

  unsigned char glyph = (unsigned char)codepoint;
  if (glyph == held->second.m_glyph) {
    held->second.m_awaitingGlyph = false;
    return;
  }

  KeyRowColInfo cell;
  if (!LookupGlyph(glyph, cell)) {
    held->second.m_awaitingGlyph = false;
    if (m_debug)
      cerr << "kb: host glyph '" << (char)glyph << "' is not on this keyboard" << endl;
    return;
  }

  SDL_Scancode scancode = m_pendingScancode;
  SDL_Keycode sym = m_pendingSym;
  bool hostShift = m_pendingShift;
  PressText(scancode, sym, hostShift, cell, glyph, false);
}
