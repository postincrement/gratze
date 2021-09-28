#include <iostream>
#include <map>
#include <SDL_keyboard.h>

#include <strings.h>
#include <string.h>

#include "common/misc.h"
#include "devices/keyscan.h"

using namespace std;

bool ScannedKeyboard::Open()
{
  Reset();
  return true;
}

void ScannedKeyboard::Reset()
{
  VECTOR_ZERO(m_kbData);
  m_leftShift  = false;
  m_rightShift = false;
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

void ScannedKeyboard::Compile(const ScanLayout & scanLayout)
{
  m_rows = scanLayout.m_rows;
  m_cols = scanLayout.m_cols;

  m_kbData.resize(((m_cols + 7) / 8) * m_rows);
  
  // find modifier keys
  const ScanCode * keyCodeMap = scanLayout.m_textKeys;
  for (int row = 0; row < m_rows; ++row) {
    for (int col = 0; col < m_cols; ++col) {
      const ScanCode * code = keyCodeMap++;
      if (code->m_name == 0) {
        continue;
      }
      if (strcasecmp(code->m_name, "shift") == 0) {
        cerr << "kb: shift key is " << col << "," << row << endl;
        m_shiftRowCol = KeyRowColInfo(row, col, false);
      }
      else if (strcasecmp(code->m_name, "control") == 0) {
        m_controlRowCol = KeyRowColInfo(row, col, false);
      }
      else if (strcasecmp(code->m_name, "capslock") == 0) {
        m_capsLockRowCol = KeyRowColInfo(row, col, false);
      }
    }
  }

  // compile text keymaps for text mode
  m_textKeys.clear();
  Compile(m_textKeys, m_rows, m_cols, scanLayout.m_textKeys,        false);
  Compile(m_textKeys, m_rows, m_cols, scanLayout.m_shiftedTextKeys, true);

  // compile game key map, if present
  m_gameKeys.clear();
  if (scanLayout.m_gameKeys != NULL) {
    Compile(m_gameKeys, m_rows, m_cols, scanLayout.m_gameKeys);
    if (m_shiftRowCol.m_col != 0) {
      if (m_gameKeys.count(SDLK_LSHIFT) == 0)
        m_gameKeys.insert(SDLKeyRowColMap::value_type(SDLK_LSHIFT, m_shiftRowCol));
      if (m_gameKeys.count(SDLK_RSHIFT) == 0)
        m_gameKeys.insert(SDLKeyRowColMap::value_type(SDLK_RSHIFT, m_shiftRowCol));
    }
    if (m_controlRowCol.m_col != 0) {
      if (m_gameKeys.count(SDLK_LCTRL) == 0)
        m_gameKeys.insert(SDLKeyRowColMap::value_type(SDLK_LCTRL, m_controlRowCol));
      if (m_gameKeys.count(SDLK_RCTRL) == 0)
        m_gameKeys.insert(SDLKeyRowColMap::value_type(SDLK_RCTRL, m_controlRowCol));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////

bool ScannedKeyboard::FindKey(const std::string & name, SDL_Keycode & keycode) const
{
  // convert our key name to SDL_Keycode
  keycode = SDL_GetKeyFromName(name.c_str());
  return keycode != SDLK_UNKNOWN;
}

void ScannedKeyboard::Compile(SDLKeyRowColMap & keyMap,
                                            int rowCount, 
                                            int colCount,
                               const ScanCode * keyCodeMap)
{
  for (int row = 0; row < rowCount; ++row) {
    for (int col = 0; col < colCount; ++col) {
      const ScanCode * code = keyCodeMap++;

      if (code->m_name == 0) {
        continue;
      }

      SDL_Keycode keycode;
      bool destIsShifted;
      if (FindKey(code->m_name, keycode)) {
        keyMap.insert(
          SDLKeyRowColMap::value_type(keycode, 
                                    KeyRowColInfo(row, col)
        ));
      }
      else {
        cerr << "error: unknown SDL keycode name '" << code->m_name << "'" << endl;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////

bool ScannedKeyboard::AddEquivalent(TextKeyRowColMap & keyMap,
                               const std::string & fromName, 
                               const std::string & toName)
{
#if 0
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
#endif

  return true;
}



void ScannedKeyboard::Compile(TextKeyRowColMap & keyMap,
                                             int rowCount, 
                                             int colCount,
                                const ScanCode * keyCodeMap,
                                           bool shifted)
{
  for (int row = 0; row < rowCount; ++row) {
    for (int col = 0; col < colCount; ++col) {
      const ScanCode * code = keyCodeMap++;

      if (code->m_name == 0) {
        continue;
      }

      keyMap.insert(
        TextKeyRowColMap::value_type(code->m_name[0], 
                                     KeyRowColInfo(row, col, shifted)
        ));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////

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
  // see if the keycode code is mapped to a rowcol
  SDLKeyRowColMap::iterator r = m_gameKeys.find(keysym.sym);
  if (r == m_gameKeys.end()) {
    cerr << "warning: unmapped keyboard " << (down ? "down" : "up") << " code " << HEXFORMAT0x8(keysym.sym) << endl;
    return;
  }

  KeyRowColInfo & rowCol = r->second;

  const char * keyName = SDL_GetKeyName(r->first);
  cerr << "info: mapped key '" << keyName << "' " << (down ? "down" : "up") << " to row " << rowCol.m_row << ", col " << rowCol.m_col << " in " << (m_gameMode ? "game" : "text") << " mode" << endl;
  ActivateKey(rowCol, down);
}

void ScannedKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{
  if (m_gameMode)
    KeyAction(keysym, true);
}

void ScannedKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{
  if (m_gameMode)
    KeyAction(keysym, false);
}

void ScannedKeyboard::OnKeyChar(char ch)
{
  if (m_gameMode)
    return;  
}
