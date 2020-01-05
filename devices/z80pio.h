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

    virtual void SetData(int port, uint8_t data);
    virtual uint8_t GetData(int port) const;

    void SetInterruptHandler(std::function<void (uint8_t)> handler);
    void SetReadHandler(int port, std::function<uint8_t ()> handler);

    struct Port
    {
      Port(int port);
      void Reset();

      void WriteControl(uint8_t data);
      void WriteData(uint8_t data);

      uint8_t ReadControl();
      uint8_t ReadData();

      void SetData(uint8_t data);
      uint8_t GetData() const;

      int m_port;
      int m_state;
      uint8_t m_data = 0;
      bool m_ie = false;
      int8_t m_mode;
      uint8_t m_mode3 = 0;
      uint8_t m_vector = 0;
      uint8_t m_intMask = 0;
      uint8_t m_control = 0;
      
      std::function<uint8_t ()> m_readHandler;
    };

  protected:
    Port m_ports[2] = { 0, 1 };  
    std::function<void (uint8_t)> m_interruptHandler;
};

#endif // Z80_PIO_H
