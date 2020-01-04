#ifndef Z80_PIO_H
#define Z80_PIO_H

#include <functional>

class Z80PIO
{
  public:
    Z80PIO();
    void Reset();

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    virtual void ReceiveData(int port, uint8_t data);

    void SetInterruptHandler(std::function<void (uint8_t)> handler)
    { m_interruptHandler = handler; }

    struct Port
    {
      Port(int port);
      void Reset();

      void WriteControl(uint8_t data);
      void WriteData(uint8_t data);

      uint8_t ReadControl();
      uint8_t ReadData();

      bool ReceiveData(uint8_t data);

      int m_port;
      int m_state;
      uint8_t m_data = 0;
      bool m_ie = false;
      int8_t m_mode;
      uint8_t m_mode3 = 0;
      uint8_t m_vector = 0;
      uint8_t m_intMask = 0;
      uint8_t m_control = 0;
    };

  protected:
    Port m_ports[2] = { 0, 1 };  
    std::function<void (uint8_t)> m_interruptHandler;
};

#endif // Z80_PIO_H
