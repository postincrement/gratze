#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <functional>

#include <SDL.h>

#include "devices/device.h"

class VirtualKeyboard : public VirtualDevice
{
  public:
    virtual ~VirtualKeyboard();

    virtual void Reset() override;

    virtual void OnKeyDown(const SDL_Keysym & keysym);
    virtual void OnKeyUp(const SDL_Keysym & keysym);
    virtual void OnKeyText(const std::string & str);
    virtual void OnKeyChar(char ch);

    // only implemented for ScannedKeyboard - saves a cast
    virtual uint8_t Read(uint16_t rowMask);

    void SetASCIICallback(std::function<void (uint8_t)> handler);

  protected:  
    std::function<void (uint8_t)> m_asciiCallBack = nullptr;
};

#endif // KEYBOARD_H_
