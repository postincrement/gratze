#ifndef SYNERTEK_6546_H_
#define SYNERTEK_6546_H_


#include <vector>
#include <functional>

#include "devices/device.h"


class Synertek6545 : public VirtualDevice
{
  public:
    const int m_maxReg = 19;    // don't include register 31

    enum StatusBits
    {
      eVBlanking   = 0x20,
      eLpenFull    = 0x40,
      eUpdateReady = 0x80
    };

    Synertek6545();

    virtual void Reset() override;

    virtual uint8_t GetStatus() const;
    virtual void SetStatus(uint8_t status);

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    void SetScreenShapeHandler(std::function<void (int cols, int rows, int lines)> handler);
    void SetStartAddressHandler(std::function<void (uint16_t addr)> handler);
    void SetCursorAddressHandler(std::function<void (uint16_t addr)> handler);
    void SetCursorShapeHandler(std::function<void (uint8_t start, uint8_t end, int blinkRate)> handler);
    void SetUpdateHandler(std::function<bool (bool, uint16_t & addr)> handler);

    void UpdateStatus();

    void Reg31();

  protected:
    int m_state = 0;
    uint8_t m_status = 0;
    uint8_t m_regSel = 0;
    bool m_writePending;
    std::vector<uint8_t> m_regs;
    std::function<void (int, int, int)> m_screenShapeHandler = nullptr;
    std::function<void (uint16_t)> m_startAddressHandler = nullptr;
    std::function<void (uint16_t)> m_cursorAddressHandler = nullptr;
    std::function<void (uint8_t start, uint8_t end, int blinkRate)> m_cursorShapeHandler = nullptr;
    std::function<bool (bool, uint16_t &)> m_updateHandler = nullptr;
};

#endif // SYNERTEK_6546_H_