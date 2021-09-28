#include <iostream>
#include <map>
#include <SDL_keyboard.h>

#include <strings.h>
#include <string.h>

#include "common/misc.h"
#include "devices/keyscan.h"

using namespace std;

uint8_t VirtualKeyboard::Read(uint16_t rowMask)
{
  return 0;
}

void ScannedKeyboard::Reset()
{
  m_leftShift  = false;
  m_rightShift = false;
}

void ScannedKeyboard::Compile(const ScanLayout & scanLayout)
{
  m_rows = scanLayout.m_rows;
  m_cols = scanLayout.m_cols;

  m_kbData.resize(((m_cols + 7) / 8) * m_rows);

  // compile text keymaps
  m_textKeys.clear();
  Compile(m_textKeys, m_rows, m_cols, scanLayout.m_keyCodes,        false);
  Compile(m_textKeys, m_rows, m_cols, scanLayout.m_shiftedKeyCodes, true);
  for (auto & equivalent : scanLayout.m_equivalents)
    AddEquivalent(m_textKeys, equivalent.m_from, equivalent.m_to);

  // find text modifiers
  {
    const ScanCode * keyCodeMap = scanLayout.m_keyCodes;
    for (int row = 0; row < m_rows; ++row) {
      for (int col = 0; col < m_cols; ++col) {
        const ScanCode * code = keyCodeMap++;
        if (code->m_name == 0) {
          continue;
        }
        if (strcasecmp(code->m_name, "shift") == 0) {
          cerr << "kb: shift key is " << col << "," << row << endl;
          m_shiftRowCol = KeyRowColInfo(row, col, false, false);
        }
        else if (strcasecmp(code->m_name, "control") == 0) {
          m_controlRowCol = KeyRowColInfo(row, col, false, false);
        }
        else if (strcasecmp(code->m_name, "capslock") == 0) {
          m_capsLockRowCol = KeyRowColInfo(row, col, false, false);
        }
      }
    }
  }

  // compile game key maps
  m_gameKeys.clear();
  if (scanLayout.m_gameKeys != NULL) {
    Compile(m_gameKeys, m_rows, m_cols, scanLayout.m_gameKeys, false);
  }
}

static struct VirtualKeyMap {
  char m_key;
  SDL_Keycode m_code;
} const g_virtualKeys[] = {
  { '~',  SDLK_BACKQUOTE },
  { '!',  SDLK_1 },
  { '@',  SDLK_2 },
  { '#',  SDLK_3 },
  { '$',  SDLK_4 },
  { '%',  SDLK_5 },
  { '^',  SDLK_6 },
  { '&',  SDLK_7 },
  { '*',  SDLK_8 },
  { '(',  SDLK_9 },
  { ')',  SDLK_0 },
  { '_',  SDLK_MINUS },
  { '+',  SDLK_EQUALS },
  { '{',  SDLK_LEFTBRACKET },
  { '}',  SDLK_RIGHTBRACKET },
  { '|',  SDLK_BACKSLASH },
  { ':',  SDLK_SEMICOLON },
  { '"',  SDLK_QUOTE },
  { '<',  SDLK_COMMA },
  { '>',  SDLK_PERIOD },
  { '?',  SDLK_SLASH }
};

bool ScannedKeyboard::AddEquivalent(KeyRowColMap & keyMap,
                               const std::string & fromName, 
                              const std::string & toName)
{
  stringstream strm;
  strm << " for equivalent mapping from " << fromName << " to " << toName << endl;

  // find the "from" name
  SDL_Keycode fromKeycode;
  bool fromShiftSource;
  if (!FindKey(fromName, fromKeycode, fromShiftSource)) {
    cerr << "error: cannot find source key " << strm.str();
    return false;
  }

  // find the "to" name
  SDL_Keycode toKeycode;
  bool toShiftSource;
  if (!FindKey(toName, toKeycode, toShiftSource)) {
    cerr << "error: cannot find destination key " << strm.str();
    return false;
  }

  KeyRowColMap::iterator r = keyMap.find(toKeycode);
  if (r == keyMap.end()) {
    cerr << "error: no row/col found for destination key " << strm.str();
    return false;
  }

  cout << "info: mapping " << fromName << "(" HEXFORMAT0x4(fromKeycode) << ") to " << r->second.m_row << "," << r->second.m_col << endl;
  keyMap.insert(KeyRowColMap::value_type(fromKeycode, r->second));

  return true;
}

bool ScannedKeyboard::FindKey(const std::string & name, SDL_Keycode & keycode, bool & shifted) const
{
  // check for shifted "virtual" keys
  if (name.length() == 1) {
    for (int i = 0; i < sizeof(g_virtualKeys) / sizeof(g_virtualKeys[0]); ++i) {
      if (g_virtualKeys[i].m_key == name[0]) {
        keycode = g_virtualKeys[i].m_code;
        shifted = true;
        return true;
      }
    }
  }

  // convert our keycode to SDL_Keycode
  keycode = SDL_GetKeyFromName(name.c_str());
  if (keycode == SDLK_UNKNOWN) {
    return false;
  }

  shifted = false;
  return true;
}

void ScannedKeyboard::Compile(KeyRowColMap & keyMap,
                                         int rowCount, 
                                         int colCount,
                            const ScanCode * keyCodeMap, 
                                        bool hostIsShifted)
{
  for (int row = 0; row < rowCount; ++row) {
    for (int col = 0; col < colCount; ++col) {
      const ScanCode * code = keyCodeMap++;

      if (code->m_name == 0) {
        continue;
      }

      if (strcasecmp(code->m_name, "shift") == 0) {
        if (!hostIsShifted) {
          cerr << "kb: shift key is " << col << "," << row << endl;
          m_shiftRowCol = KeyRowColInfo(row, col, false, false);
        }
      }
      else if (strcasecmp(code->m_name, "control") == 0) {
        if (!hostIsShifted) {
          m_controlRowCol = KeyRowColInfo(row, col, false, false);
        }
      }
      else if (strcasecmp(code->m_name, "capslock") == 0) {
        if (!hostIsShifted) {
          m_capsLockRowCol = KeyRowColInfo(row, col, false, false);
        }
      }
      else {
        SDL_Keycode keycode;
        bool destIsShifted;
        if (FindKey(code->m_name, keycode, destIsShifted)) {
          cout << "kb: mapped host '" << code->m_name << "' mapped to device row " << row << ", col " << col;
          if (hostIsShifted) cout << " + shift";
          cout << endl;
          keyMap.insert(
            KeyRowColMap::value_type(keycode, 
                                     KeyRowColInfo(row, col, destIsShifted, hostIsShifted)
          ));
        }
        else {
          cerr << "error: unknown keycode name '" << code->m_name << "'" << endl;
        }
      }
    }
  }
}

bool ScannedKeyboard::Open()
{
  VECTOR_ZERO(m_kbData);
  return true;
}

void ScannedKeyboard::SetGameMode(bool mode)
{
  m_gameMode = mode;
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

  //if (value != 0x00)
  //  cout << "kb: read with rowmask " << HEXFORMAT0x2(rowMask) << " = " << HEXFORMAT0x2(value) << endl;

  return value;
}

template<typename Type>
void MaskDown(bool down, Type mask, Type & val)
{
  if (down)
    val |= mask;
  else
    val &= !mask;
}


void ScannedKeyboard::ActivateKey(const KeyRowColInfo & rowCol, bool down)
{
  if (rowCol.m_row < 0)
    return;

  uint8_t value = m_kbData[rowCol.m_row];

  MaskDown<uint8_t>(down, 1 << rowCol.m_col, m_kbData[rowCol.m_row]);

  if (down)
    cout << "kb: activating row " << rowCol.m_row << ", col " << rowCol.m_col << endl;
}

void ScannedKeyboard::KeyAction(const SDL_Keysym & keysym, bool down)
{
  // handle modifiers in game mode
  switch (keysym.sym) {
    case SDLK_LSHIFT:
      m_leftShift = down;
      if (m_gameMode)
        ActivateKey(m_shiftRowCol, down);
      return;

    case SDLK_RSHIFT:
      m_rightShift = down;
      if (m_gameMode)
        ActivateKey(m_shiftRowCol, down);
      return;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      if (m_gameMode)
        ActivateKey(m_controlRowCol, down);
      return;

    case SDLK_CAPSLOCK:
      if (m_gameMode)
        ActivateKey(m_capsLockRowCol, down);
      return;
  }

  // select the correct keymap
  KeyRowColMap & keyMap = m_gameMode ? m_gameKeys : m_textKeys;

  // get the status of the shift keys
  bool shiftStatus = (m_leftShift || m_rightShift);

  // see if the keycode code is mapped to a rowcol
  SDL_Keycode ascii = keysym.sym;
  int count = keyMap.count(ascii);
  if (count == 0) {
    cerr << "warning: unmapped keyboard " << (down ? "down" : "up") << " code " << HEXFORMAT0x8(keysym.sym) << endl;
    return;
  }

  // get first mapping for this keycode
  KeyRowColMap::iterator r = keyMap.find(ascii);
  KeyRowColInfo rowCol;

  // handle game mode
  if (m_gameMode) {
    int i;
    for (i = 0; i < count; ++i) {
      if (!r->second.m_hostIsShifted) {
        rowCol = r->second;
        break;
      }
      ++r;
    }
    if (i < count) {
      const char * keyName = SDL_GetKeyName(r->first);
      cerr << "info: mapped key '" << keyName << "' " << (down ? "down" : "up") << " to row " << rowCol.m_row << ", col " << rowCol.m_col << " in " << (m_gameMode ? "game" : "text") << " mode" << endl;
      ActivateKey(rowCol, down);
    }
    return;
  }

  // handle text mode
  int i;
  for (i = 0; i < count; ++i) {
    if (r->second.m_hostIsShifted == shiftStatus) {
      rowCol = r->second;
      break;
    }
    ++r;
  }
  if (i == count) {
    for (i = 0; i < count; ++i) {
      if (r->second.m_hostIsShifted) {
        rowCol = r->second;
        break;
      }
    }
  }
  if (i < count) {
    if (m_virtualShift != r->second.m_destIsShifted) {
      m_virtualShift = r->second.m_destIsShifted;
      cerr << "info: virtual shift now " << (m_virtualShift ? "up" : "down") << endl;
      ActivateKey(m_shiftRowCol, m_virtualShift);
    }
    if (down) {
      const char * keyName = SDL_GetKeyName(r->first);
      cerr << "info: mapped key '" << keyName << "' " << (down ? "down" : "up") << " to row " << rowCol.m_row << ", col " << rowCol.m_col << " in " << (m_gameMode ? "game" : "text") << " mode" << endl;
    }
    ActivateKey(rowCol, down);
  }
}

void ScannedKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{
  KeyAction(keysym, true);
}

void ScannedKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{
  KeyAction(keysym, false);
}

void ScannedKeyboard::OnKeyText(const std::string & str)
{
}
