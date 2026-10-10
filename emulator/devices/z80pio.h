#ifndef Z80_PIO_H
#define Z80_PIO_H

#include <functional>

#include "devices/device.h"

class Z80PIO  : public VirtualDevice
{
  public:
    enum class Mode {
      Output  = 0,
      Input   = 1,
      Bidir   = 2,
      Control = 3
    };
    Z80PIO();

    virtual void Reset() override;

    // used by CPU to read data from a PIO register
    virtual uint8_t Read(uint8_t reg);

    // used by CPU to write data to a PIO register
    virtual void Write(uint8_t reg, uint8_t data);

    // Latch a byte the CPU will read. Does not request an interrupt;
    // Strobe() does that.
    virtual void SetData(int port, uint8_t data);

    // Output latch last written by the CPU. Safe to call from a write handler.
    virtual uint8_t GetData(int port) const;

    Mode GetMode(int port) const;
    bool GetIE(int port) const;
    bool GetReady(int port) const;

    // ASTB on an output port, or the input strobe once SetData has latched
    // a byte. Mode 2 uses Strobe() for the output half and Strobe(port, data)
    // for the input half. The interrupt is requested through the handler and
    // is not taken inside the instruction that caused it.
    void Strobe(int port);
    void Strobe(int port, uint8_t data);

    void SetInterruptHandler(std::function<void (uint8_t)> handler);

    void SetReadHandler(int port, std::function<uint8_t ()> handler);
    void SetWriteHandler(int port, std::function<void (uint8_t, bool)> handler);

    struct Port
    {
      Port(int port);
      void Reset();

      // used by CPU to write to the PIO channel control register
      void WriteControl(uint8_t data);

      // used by CPU to read from from the PIO channel control register
      uint8_t ReadControl();

      // used by CPU to write data to the PIO channel data register
      void WriteData(uint8_t data);

      // used by CPU to read data from the PIO channel data register
      uint8_t ReadData();

      int m_port;
      int m_state;
      uint8_t m_outputLatch = 0;
      uint8_t m_data = 0;
      bool m_ie = false;
      bool m_ready = false;
      Mode m_mode;
      uint8_t m_inputMask = 0;   // 1 =input, 0 = output
      uint8_t m_vector = 0;
      uint8_t m_intMask = 0;
      uint8_t m_control = 0;
      
      std::function<uint8_t ()> m_readHandler;
      std::function<void (uint8_t, bool)> m_writeHandler;
    };

  protected:
    void RequestInterrupt(const Port & port);

    Port m_ports[2] = { 0, 1 };  
    std::function<void (uint8_t)> m_interruptHandler;
};

#endif // Z80_PIO_H
