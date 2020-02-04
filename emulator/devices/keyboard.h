#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <SDL.h>

#include "devices/device.h"

class VirtualKeyboard : public VirtualDevice
{
  public:
    virtual ~VirtualKeyboard();

    virtual void OnKeyDown(const SDL_Keysym & keysym) = 0;
    virtual void OnKeyUp(const SDL_Keysym & keysym) = 0;

    // only implemented for KeyboardScanner - saves a cast
    virtual uint8_t Read(uint16_t rowMask);

    // only implemented for KeyboardScanner - saves a cast
    //virtual uint8_t Read(uint16_t rowMask);
};

#endif // KEYBOARD_H_
