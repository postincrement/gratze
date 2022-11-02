#include <iostream>
#include <map>
#include <SDL_keyboard.h>

#include <strings.h>
#include <string.h>

#include "common/misc.h"
#include "devices/keypar.h"

using namespace std;

void VirtualKeyboard::Reset()
{}

VirtualKeyboard::~VirtualKeyboard()
{}

// called when SDL key pressed
void VirtualKeyboard::OnKeyDown(const SDL_Keysym & keysym)
{}

// called when SDL key released
void VirtualKeyboard::OnKeyUp(const SDL_Keysym & keysym)
{}

// called for SDL text input, or non-SDL input
void VirtualKeyboard::OnKeyChar(char ch)
{
  if (m_keyCharCallBack)
    m_keyCharCallBack(ch);
}

// set handler for text chars
void VirtualKeyboard::SetKeyCharCallback(std::function<void (uint8_t)> callback)
{
  m_keyCharCallBack = callback;
}

void VirtualKeyboard::OnKeyText(const std::string & str)
{
  for (auto r : str) {
    OnKeyChar(r);
  }
}

uint8_t VirtualKeyboard::Read(uint16_t rowMask)
{
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

ParallelKeyboard::ParallelKeyboard()
  : ParallelKeyboard(Mapping())
{
}

ParallelKeyboard::ParallelKeyboard(const Mapping & mapping)
  : m_mapping(mapping)
{
}

void ParallelKeyboard::OnKeyChar(char ch)
{
  VirtualKeyboard::OnKeyChar(ch);
}
