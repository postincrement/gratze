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
void VirtualKeyboard::OnKeyText(const std::string & str)
{
  for (auto r : str)
    OnKeyChar(r);
}

void VirtualKeyboard::OnKeyChar(char ch)
{
  if (m_asciiCallBack)
    m_asciiCallBack(ch);
}

// called to set handler for key up/down
void VirtualKeyboard::SetASCIICallback(std::function<void (uint8_t)> callback)
{
  m_asciiCallBack = callback;
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
