#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <functional>

#include <SDL.h>

#include "devices/device.h"

//
//  Base class for all virtual keyboard imlementations
//

class VirtualKeyboard : public VirtualDevice
{
  public:
    virtual ~VirtualKeyboard();

    virtual void Reset() override;

    // called when SDL key pressed
    virtual void OnKeyDown(const SDL_Keysym & keysym);

    // called when SDL key released
    virtual void OnKeyUp(const SDL_Keysym & keysym);

    // called for SDL text input, or non-SDL input
    virtual void OnKeyChar(char ch);

    // calls OnKeyChar for each character in the string
    virtual void OnKeyText(const std::string & str);

    // only implemented for ScannedKeyboard - saves a cast
    virtual uint8_t Read(uint16_t rowMask);

    // set callback for when text chars pressed
    // used to trigger interrupt driven devices like parallel ports
    void SetKeyCharCallback(std::function<void (uint8_t)> handler);

  protected:  
    std::function<void (uint8_t)> m_keyCharCallBack = nullptr;
};

#endif // KEYBOARD_H_
