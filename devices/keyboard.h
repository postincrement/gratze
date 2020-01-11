#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <SDL.h>

#include "devices/device.h"

class VirtualKeyboard : public VirtualDevice
{
  public:
    virtual void OnKeyDown(const SDL_Keysym & keysym) = 0;
    virtual void OnKeyUp(const SDL_Keysym & keysym) = 0;
};

#endif // KEYBOARD_H_
