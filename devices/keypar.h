#ifndef KEYPAR_H_
#define KEYPAR_H_

#include <map>
#include <vector>
#include <functional>

#include "devices/keyboard.h"

class ParallelKeyboard : public VirtualKeyboard
{
  public:
    struct Mapping {
      bool m_defaultUpper   = true;
      bool m_arrowsToWASD   = true;
      bool m_bsToDelete     = true;
      bool m_deleteToBs     = false;
      bool m_shiftEnterToLF = true;
    };

    ParallelKeyboard();
    ParallelKeyboard(const Mapping & mapping);
    virtual void OnKeyDown(const SDL_Keysym & keysym) override;
    virtual void OnKeyUp(const SDL_Keysym & keysym) override;

    void SetHandler(bool down, std::function<void (uint8_t)> handler);

    virtual void Reset() override;

  protected:  
    int ConvertKeySymToASCII(const SDL_Keysym & keysym);

    Mapping m_mapping;
    std::function<void (uint8_t)> m_keyHandlers[2] { nullptr, nullptr };
};

#endif // KEYPAR_H_