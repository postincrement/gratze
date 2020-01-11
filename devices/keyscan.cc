#include <iostream>
#include <map>
#include <SDL_keyboard.h> 

#include <strings.h>
#include <string.h>

#include "src/misc.h"
#include "devices/keyscan.h"

using namespace std;

void KeyboardScanner::Reset()
{
  m_shiftStatus = 0;
}

void KeyboardScanner::Compile(const ScanLayout & scanLayout)
{
  m_keys.clear();

  m_rows = scanLayout.m_rows;
  m_cols = scanLayout.m_cols;

  m_kbData.resize(((m_cols + 7) / 8) * m_rows);
  Compile(m_rows, m_cols, scanLayout.m_keyCodes,        false);
  Compile(m_rows, m_cols, scanLayout.m_shiftedKeyCodes, true);
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

void KeyboardScanner::Compile(int rowCount, int colCount, const ScanCode * keyCodeMap, bool shifted)
{
  for (int row = 0; row < rowCount; ++row) {
    for (int col = 0; col < colCount; ++col) {
      const ScanCode * code = keyCodeMap++;
      if (code->m_name == 0) {
        continue;
      }
      if (strcasecmp(code->m_name, "shift") == 0) {
        if (!shifted) {
          //cerr << "kb: shift key is " << col << "," << row << endl;
          m_shiftKey = KeyRowColInfo(row, col, false, false);
        }
      }
      else if (strcasecmp(code->m_name, "control") == 0) {
        if (!shifted) {
          m_controlKey = KeyRowColInfo(row, col, false, false);
        }
      }
      else if (strcasecmp(code->m_name, "capslock") == 0) {
        if (!shifted) {
          m_capsLockKey = KeyRowColInfo(row, col, false, false);
        }
      }
      else {
        bool found = false;

        // check for shifted "virtual" keys
        if (strlen(code->m_name) == 1) {
          for (int i = 0; i < sizeof(g_virtualKeys) / sizeof(g_virtualKeys[0]); ++i) {
            if (g_virtualKeys[i].m_key == code->m_name[0]) {
              const char * keyName = SDL_GetKeyName(g_virtualKeys[i].m_code);
              m_keys.insert(KeyRowColMap::value_type(g_virtualKeys[i].m_code, KeyRowColInfo(row, col, true, shifted)));
              //cout << "kb: mapped virtual '" << code->m_name << "' to name '" << keyName << "' = " << HEXFORMAT0x8(g_virtualKeys[i].m_code) << " + shift" << endl;
              found = true;
            }
          }
        }

        if (!found) {
          // convert our keycode to SDL_Keycode
          SDL_Keycode keyCode = SDL_GetKeyFromName(code->m_name);
          if (keyCode != SDLK_UNKNOWN) {
            m_keys.insert(KeyRowColMap::value_type(keyCode, KeyRowColInfo(row, col, false, shifted)));
            //cout << "kb: mapped '" << code->m_name << "' to " << HEXFORMAT0x8(keyCode) << endl;
          }
          else {
            cerr << "error: unknown keycode name '" << code->m_name << "'" << endl;
          }
        }
      }
    }
  }
}

bool KeyboardScanner::Open()
{
  memset(&m_kbData[0], 0, m_kbData.size());
  return true;
}

uint8_t KeyboardScanner::Read(uint16_t rowMask)
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


void KeyboardScanner::ActivateKey(const KeyRowColInfo & rowCol, bool down)
{
  if (rowCol.m_row < 0)
    return;

  uint8_t value = m_kbData[rowCol.m_row];

  MaskDown<uint8_t>(down, 1 << rowCol.m_col, m_kbData[rowCol.m_row]);

  //if (down)
  //  cout << "kb: activating row " << rowCol.m_row << ", col " << rowCol.m_col << endl;
}

void KeyboardScanner::KeyAction(const SDL_Keysym & keysym, bool down)
{
  // handle modifiers
  switch (keysym.sym) {
    case SDLK_LSHIFT:
      if (down) {
        m_shiftStatus |= 1;
        m_shiftStatus &= ~4;
      }
      else {
        m_shiftStatus &= !1;
      }
      //cerr << "kb: shift key " << (down ? "down" : "up") << endl;
      ActivateKey(m_shiftKey, down);
      return;

    case SDLK_RSHIFT:
      if (down) {
        m_shiftStatus |= 2;
        m_shiftStatus &= ~4;
      }
      else {
        m_shiftStatus &= !2;
        m_kbData[7] &= !1;
      }
      ActivateKey(m_shiftKey, down);
      return;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      ActivateKey(m_controlKey, down);
      return;

    case SDLK_CAPSLOCK:
      ActivateKey(m_capsLockKey, down);
      return;
  }

  SDL_Keycode ascii = keysym.sym;

  bool shiftDown = (m_shiftStatus != 0);

/*
  bool makeUpper = (m_defaultUpper != shiftDown);

  if (islower(sym)) {
    sym = makeUpper ? toupper(sym) : tolower(sym);
    if (keysym.mod & KMOD_CTRL)
      sym = toupper(sym) - 0x40;
  }
*/

  int count = m_keys.count(ascii);
  if ((count == 0) && islower(ascii)) {
    ascii = toupper(ascii);
    count = m_keys.count(ascii);
  }
  if (count == 0) {
    cerr << "warning: unmapped keyboard " << (down ? "down" : "up") << " code " << HEXFORMAT0x8(keysym.sym) << endl;
    return;
  }
  //cerr << "info: found " << count << " entries for " << HEXFORMAT0x8(keysym.sym) << endl;

  KeyRowColMap::iterator r = m_keys.find(ascii);
  int i;
  for (i = 0; i < count; ++i) {
    //cout << "kb: rec " << i << " has shift status " << r->second.m_shiftSource << " " <<  r->second.m_shiftOut << endl;
    if (r->second.m_shiftSource == ((m_shiftStatus & 3) != 0))
      break;
    ++r;  
  }
  if (i >= count) {
    //cerr << "warning: mapped keyboard " << (down ? "down" : "up") << " code " << HEXFORMAT0x8(keysym.sym) << " with unmatched shift state " << ((shiftDown ? "down" : "up")) << endl;
    return;
  }

  // if shift status 
  KeyRowColInfo & rowCol = r->second;

  if (down) {
    // activate keys with the correct shift sense
    if (shiftDown == rowCol.m_shiftOut) {
      //cerr << "kb: no virtual shift change" << endl;
      m_shiftStatus &= ~4;
    }

    // activate keys that need to be shifted
    else if (rowCol.m_shiftOut) {
      //cerr << "kb: virtual shift key down" << endl;
      memset(&m_kbData[0], 0x00, sizeof(m_kbData));
      ActivateKey(m_shiftKey, true);
      m_shiftStatus = 4;
    }

    // acivate keys that need to be unshifted
    else {
      //cerr << "kb: virtual shift key up" << endl;
      memset(&m_kbData[0], 0x00, sizeof(m_kbData));
      ActivateKey(m_shiftKey, false);
      m_shiftStatus = 0;
    }
  }

  ActivateKey(rowCol, down);
}

void KeyboardScanner::OnKeyDown(const SDL_Keysym & keysym)
{
  KeyAction(keysym, true);
}

void KeyboardScanner::OnKeyUp(const SDL_Keysym & keysym)
{
  KeyAction(keysym, false);
}
