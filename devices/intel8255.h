#ifndef INTEL_8255_H_
#define INTEL_8255_H_

#include <cstdint>

class Intel8255
{
  public:
    Intel8255();

    void Reset();

    virtual uint8_t Read(uint8_t reg);
    virtual void Write(uint8_t reg, uint8_t data);

    virtual void SetData(int port, uint8_t data);
    virtual uint8_t GetData(int port) const;

    virtual uint8_t ReadControl();
    virtual void WriteControl(uint8_t data);

    void SetReadHandler(int port, std::function<uint8_t ()> handler);

    struct Port
    {
      Port(int port);
      void Reset();

      uint8_t ReadData();
      void WriteData(uint8_t data);

      void SetData(uint8_t data);
      uint8_t GetData() const;

      int m_port;
      uint8_t m_data = 0;

      std::function<uint8_t ()> m_readHandler;
    };

  protected:  
    uint8_t m_control;
    Port m_ports[3] = { 0, 1, 2 };  
};


#endif // INTEL_8255_H_