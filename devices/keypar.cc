#include <iostream>
#include <map>
#include <SDL_keyboard.h> 

#include <strings.h>
#include <string.h>

#include "src/misc.h"
#include "devices/keypar.h"

using namespace std;


ParallelKeyboard::ParallelKeyboard()
  : ParallelKeyboard(Mapping())
{
}

ParallelKeyboard::ParallelKeyboard(const Mapping & mapping)
  : m_mapping(mapping)
{
}

void ParallelKeyboard::Reset()
{
}

void ParallelKeyboard::SetHandler(bool down, std::function<void (uint8_t)> handler)
{
  m_keyHandlers[down ? 0 : 1] = handler;
}

int ParallelKeyboard::ConvertKeySymToASCII(const SDL_Keysym & keysym)
{
  int ascii = keysym.sym;

  //cout << "kb: " << HEXFORMAT0x4(keysym.sym) << ", mod " << HEXFORMAT0x4(keysym.mod) << endl;

  bool makeUpper = m_mapping.m_defaultUpper == ((keysym.mod & KMOD_SHIFT) == 0);
  if (islower(ascii)) {
    ascii = makeUpper ? toupper(ascii) : tolower(ascii);
    if (keysym.mod & KMOD_CTRL)
      ascii = toupper(ascii) - 0x40;
  }
  else {
    if (m_mapping.m_arrowsToWASD) {
      switch (keysym.sym) {
        case SDLK_LEFT:
          ascii = 'A' - 0x40;
          break;
        case SDLK_RIGHT:
          ascii = 'S' - 0x40;
          break;
        case SDLK_UP:
          ascii = 'W' - 0x40;
          break;
        case SDLK_DOWN:
          ascii = 'Z' - 0x40;
          break;
        default:
        break;   
      }
    }

    if (m_mapping.m_bsToDelete && (keysym.sym == SDLK_BACKSPACE)) {
      ascii = SDLK_DELETE;
    }

    if (m_mapping.m_deleteToBs && (keysym.sym == SDLK_DELETE)) {
      ascii = SDLK_BACKSPACE;
    }
    
    if (m_mapping.m_shiftEnterToLF && (keysym.sym == SDLK_RETURN) && (keysym.mod & KMOD_SHIFT))
      ascii = 0x0a;
  }

  if ((ascii >= 0x80) || (ascii < 0))
    return -1;

  return ascii;
}

void ParallelKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{
  int ascii = ConvertKeySymToASCII(keysym);
  if ((ascii >= 0) && m_keyHandlers[0])
    m_keyHandlers[0](ascii);
}

void ParallelKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{
  int ascii = ConvertKeySymToASCII(keysym);
  if ((ascii >= 0) && m_keyHandlers[1])
    m_keyHandlers[1](ascii);
}
