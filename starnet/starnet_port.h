#ifndef STARNET_PORT_H_
#define STARNET_PORT_H_

#include <stdint.h>
#include <vector>

// The station talks to this, not to a socket and not to a PIO register.
// Read and Write move one Starnet wire frame. A UDP port wraps that frame
// in a shim the session never sees. A hardware port clocks the same bytes.
class StarnetPort
{
  public:
    virtual ~StarnetPort() = default;

    // Bytes of one wire frame. Zero means the peer has nothing more.
    virtual int Read(uint8_t * dst, int n) = 0;
    virtual int Write(const uint8_t * src, int n) = 0;

    // The next reply may be clocked out. UDP returns immediately because
    // each reply is its own datagram. A PIO waits until the client has
    // turned around to receive.
    virtual bool WaitForReceiver() = 0;
};

// One request already received. Reply wires are kept for the caller to send.
class PacketPort : public StarnetPort
{
  public:
    PacketPort(const uint8_t * request, int n)
      : m_request(request, request + (n > 0 ? n : 0))
    {
    }

    int Read(uint8_t * dst, int n) override
    {
      if (m_consumed || n <= 0)
        return 0;
      m_consumed = true;
      int count = (int)m_request.size();
      if (count > n)
        count = n;
      for (int i = 0; i < count; ++i)
        dst[i] = m_request[(size_t)i];
      return count;
    }

    int Write(const uint8_t * src, int n) override
    {
      if (n < 0)
        return -1;
      m_replies.emplace_back(src, src + n);
      return n;
    }

    bool WaitForReceiver() override
    {
      return true;
    }

    const std::vector<std::vector<uint8_t>> & Replies() const
    {
      return m_replies;
    }

  private:
    std::vector<uint8_t> m_request;
    bool m_consumed = false;
    std::vector<std::vector<uint8_t>> m_replies;
};

#endif // STARNET_PORT_H_
